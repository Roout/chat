#include "server.hpp"
#include "client.hpp"
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

Session::Session( 
    asio::ip::tcp::socket && socket, 
    Server * server 
) :
    m_socket { std::move(socket) },
    m_server { server },
    m_strand { *server->m_context }
{
}

void Session::Send(std::string text) {
    m_buffer = std::move(text);
    m_socket.async_write_some(
        asio::buffer(m_buffer), 
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
    auto mutableBuffer = m_streamBuffer.prepare(1024);
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
        const char* p = asio::buffer_cast<const char*>(m_streamBuffer.data());
        std::string recieved;
        recieved.resize(transferredBytes);

        std::copy(p, p + transferredBytes, recieved.begin()); 
        // for(int i = 0; i < transferredBytes; i++) recieved[i] = p[i];

        boost::system::error_code error; 

        std::stringstream ss;
        ss << m_socket.remote_endpoint(error) << ": " << recieved << '\n' ;
        std::string data { ss.rdbuf()->str() };

        m_streamBuffer.consume(transferredBytes);
        
        if(m_server) {
            asio::post(std::bind(&Server::BroadcastEveryoneExcept, m_server, data, this));
        }
        
        this->Read();
    }
}

void Session::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        std::cout << "Just sent: " << transferredBytes << " bytes\n";
        m_transferred += transferredBytes;
        std::cout << "Already sent: " << m_transferred << " bytes\n";

        if(m_transferred < m_buffer.size() ) {
            // we need to send other data
            std::cout << " Trying to send: "<< m_buffer.size() - m_transferred << " bytes\n";
            m_socket.async_write_some(
                asio::buffer(&m_buffer[m_transferred], m_buffer.size() - m_transferred), 
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
        else if( m_transferred == m_buffer.size()) {
            m_transferred = 0;
        }
    } 
    else if(error == boost::asio::error::eof) {
        // Connection was closed by the remote peer
        std::cerr << error.message() << "\n";
        this->Close();
    }
    else {
        // Something went wrong
        std::cerr << error.message() << "\n";
        this->Close();
    }
}

Server::Server(std::shared_ptr<asio::io_context> context, std::uint16_t port) :
    m_context { context },
    m_acceptor { *m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port) }
{
}

void Server::Start() {
    m_socket.emplace(*m_context);
    m_acceptor.set_option(
        asio::ip::tcp::acceptor::reuse_address(false)
    );

    /// Q: Am I right to not use strand here?
    m_acceptor.async_accept( *m_socket, [&](const boost::system::error_code& code ) {
        if( !code ) {
            boost::system::error_code msg; 
            std::cerr << "Accepted connection on endpoint: " << m_socket->remote_endpoint(msg) << "\n";
            
            std::stringstream ss;
            ss << "Welcome to my server, user #" << m_socket->remote_endpoint(msg);
            std::string welcomeMessage = ss.rdbuf()->str();

            m_sessions.emplace_back(std::make_shared<Session>(std::move(*m_socket), this));
            // welcome new user
            m_sessions.back()->Send(welcomeMessage);
            m_sessions.back()->Read();
            // wait for the new connections again
            this->Start();
        }
    });
}

void Server::Shutdown() {
    boost::system::error_code ec;
    m_acceptor.close(ec);
    if(ec) {
        std::cerr<< ec.message() << "\n";
    }

    for(auto& s: m_sessions) s->Close();
    m_sessions.clear();
}

void Server::RemoveSession(const Session * s) {
    m_sessions.erase(
        std::remove_if(m_sessions.begin(), m_sessions.end(), [s](const auto& session){
            return session.get() == s;
        }),
        m_sessions.end()
    );
}

void Server::Broadcast(const std::string& text) {
    // remove closed sessions
     m_sessions.erase(
        std::remove_if(m_sessions.begin(), m_sessions.end(), [](const auto& session){
            return session->IsClosed();
        }),
        m_sessions.end()
    );

    for(const auto& session: m_sessions) {
        session->Send(text);
    }
}

void Server::BroadcastEveryoneExcept(const std::string& text, const Session* exception) {
    // remove closed sessions
     m_sessions.erase(
        std::remove_if(m_sessions.begin(), m_sessions.end(), [](const auto& session){
            return session->IsClosed();
        }),
        m_sessions.end()
    );

    for(const auto& session: m_sessions) {
        if( exception != session.get()) {
            session->Send(text);
        }
    }
}