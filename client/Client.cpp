#include "Client.hpp"
#include <iostream>
#include <string_view>
#include <functional> // std::bind

Client::Client(std::shared_ptr<asio::io_context> io):
    m_io { io },
    m_strand { *io },
    m_socket { *m_io }
{
}

Client::~Client() {
    this->Close();
}

void Client::Connect(const std::string& path, std::uint16_t port) {
    asio::ip::tcp::resolver resolver(*m_io);
    const auto endpoints = resolver.resolve(path, std::to_string(port));
    if( endpoints.empty() ) {
        this->Close();
    }
    else {
        const auto endpoint { endpoints.cbegin()->endpoint() };
        m_socket.async_connect(
            endpoint, 
            std::bind(&Client::OnConnect, this, std::placeholders::_1)  // implicit strand.
        );
    }
}

void Client::Write(std::string && text ) {
    // using strand we prevent concurrent access to variables and 
    // concurrent writing to socket. 
    /// TODO: make sure that pointer @this is valid.
    asio::post(m_strand, [text = std::move(text), this]() mutable {
        m_outbox.Enque(std::move(text));
        if(!m_isWriting) {
            this->Write();
        } 
    });
}

void Client::Close() {
    boost::system::error_code ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if(ec) {

    }
    ec.clear();
    
    m_socket.close(ec);
    if(ec) {

    }
}

void Client::OnConnect(const boost::system::error_code& err) {
    if(err) {
        std::cerr << "Client failed to connect with error: " << err.message() << "\n";
    } 
    else {
        std::cerr << "Client connected successfully!\n";
        // start waiting incoming calls
        this->Read();
    }
}

void Client::Read() {
    std::cout << "Client start reading!\n";
    m_inbox.resize(1024);
    m_socket.async_read_some(
        asio::buffer(m_inbox),
        asio::bind_executor(
            m_strand,
            std::bind(&Client::OnRead, this, std::placeholders::_1, std::placeholders::_2)
        )
    );
}

void Client::OnRead(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if(transferredBytes) {
        std::string_view sv(m_inbox);
        const auto suffixSize { static_cast<int>(m_inbox.size()) - transferredBytes };
        sv.remove_suffix(suffixSize);
        std::cout << "Server says: " << sv << '\n'; 
    }

    if( !error ) {
        this->Read();
    } 
    else /*if( error == boost::asio::error::eof) */ {
        std::cerr << "Client failed to read with error: " << error.message() << "\n";
        /// TODO: Is it safe to close already closed socket
        this->Close();
    }
}

void Client::OnWrite(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    using namespace std::placeholders;

    if( !error ) {
        std::cout << "Client just sent: " << transferredBytes << " bytes\n";

        if(m_outbox.GetQueueSize()) {
            // we need to send other data
            asio::post(m_strand, [this](){
                this->Write();
            });
        } 
        else {
            m_isWriting = false;
        }
    } 
    else {
        m_isWriting = false;
        std::cerr << "Client has error on writting: " << error.message() << '\n';
        this->Close();
    }
}

void Client::Write() {
    m_isWriting = true;
    m_outbox.SwapBuffers();
    asio::async_write(
        m_socket,
        m_outbox.GetBufferSequence(),
        asio::bind_executor(
            m_strand,
            std::bind(&Client::OnWrite, 
                this, 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}