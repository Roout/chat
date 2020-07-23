#include "Session.hpp"
#include "server.hpp"

#include <functional>
#include <iostream>
#include <sstream>

Session::Session( 
    asio::ip::tcp::socket && socket, 
    Server * server 
) :
    m_socket { std::move(socket) },
    m_server { server },
    m_strand { *server->m_context }
{
}

void Session::Write(std::string text) {
    asio::post(m_strand, [text = std::move(text), self = shared_from_this()]() mutable {
        self->m_outbox.Enque(std::move(text));
        if( !self->m_isWriting ) {
            self->Write();
        }
    });
}

void Session::Write() {
    // add all text that is queued for write operation to active buffer
    m_outbox.SwapBuffers();
    // initiate write operation
    m_isWriting = true;

    asio::async_write(
        m_socket,
        m_outbox.GetBufferSequence(),
        asio::bind_executor(
            m_strand,
            std::bind(&Session::WriteSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Session::Read() {
    auto mutableBuffer = m_inbox.prepare(1024);
    m_socket.async_read_some(
        mutableBuffer,
        asio::bind_executor(
            m_strand, 
            std::bind(&Session::ReadSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Session::Close() {
    m_isClosed = true;

    boost::system::error_code ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if(ec) {

    }
    ec.clear();
    
    m_socket.close(ec);
    if(ec) {

    } 
}

void Session::ReadSomeHandler(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if(!error) {
        std::cout << "Just recive: " << transferredBytes << " bytes.\n";
        
        m_inbox.commit(transferredBytes);
        
        const auto data { m_inbox.data() }; // asio::streambuf::const_buffers_type
        std::string recieved {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes
        };
        
        boost::system::error_code error; 

        std::stringstream ss;
        ss << m_socket.remote_endpoint(error) << ": " << recieved << '\n' ;
        std::string msg { ss.rdbuf()->str() };

        m_inbox.consume(transferredBytes);
        
        asio::post(std::bind(&Server::BroadcastEveryoneExcept, m_server, msg, this->shared_from_this()));
        
        this->Read();
    } 
    else {
        std::cerr << error.message() << "\n";
    }
}

void Session::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        std::cout << "Sent: " << transferredBytes << " bytes\n";

        if( m_outbox.GetQueueSize() ) {
            // we need to Write other data
            std::cout << "Need to Write " << m_outbox.GetQueueSize() << " messages.\n";
            asio::post(m_strand, [self = shared_from_this()](){
                self->Write();
            }); 
        } else {
            m_isWriting = false;
        }
    } 
    else /* if(error == boost::asio::error::eof) */ {
        // Connection was closed by the remote peer 
        // or any other error happened 
        std::cerr << error.message() << "\n";
        this->Close();
    }
}

