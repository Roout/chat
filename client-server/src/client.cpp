#include "client.hpp"
#include <iostream>
#include <string_view>
#include <functional> // std::bind

Client::Client(std::shared_ptr<asio::io_context> io):
    m_io { io },
    m_strand { *io },
    m_socket{ *m_io }
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
    m_isWriting = true;
    m_outcomingBuffer = std::move(text);
    m_socket.async_write_some(
        asio::buffer(m_outcomingBuffer),
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
        std::cerr << err.message() << "\n";
    }
    else {
        std::cerr << "Connected successfully!\n";
        // start waiting incoming calls
        this->Read();
    }
}

void Client::Read() {
    std::cout << "Start reading!\n";
    m_incomingBuffer.resize(1024);
    m_socket.async_read_some(
        asio::buffer(m_incomingBuffer),
        asio::bind_executor(
            m_strand,
            std::bind(&Client::OnRead, this, std::placeholders::_1, std::placeholders::_2)
        )
    );
}

/**
 * Completion handle. 
 * Called when the client read something from the remote endpoint
 */
void Client::OnRead(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if(transferredBytes) {
        std::string_view sv(m_incomingBuffer);
        const auto suffixSize { static_cast<int>(m_incomingBuffer.size()) - transferredBytes };
        sv.remove_suffix(suffixSize);
        std::cout << m_socket.remote_endpoint() << ": "<< sv << '\n'; 
    }

    if( !error ) {
        this->Read();
    } 
    // else if( error == boost::asio::error::eof) {
    //     // peer has closed this connection
    //     // handle all recieved data
    //     this->Close();
    // } 
    else {
        std::cerr << error.message() << "\n";
        this->Close();
    }
}

/**
 * Completion handle; called when the client write something to the remote endpoint
 */
void Client::OnWrite(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    using namespace std::placeholders;

    if( !error ){
        std::cout << "Just sent: " << transferredBytes << " bytes\n";
        m_transferred += transferredBytes;
        std::cout << "Already sent: " << m_transferred << " bytes\n";
        
        if(m_transferred < m_outcomingBuffer.size() ) {
            // we need to send other data
            std::cout << " Trying to send: "<< m_outcomingBuffer.size() - m_transferred << " bytes\n";
            m_socket.async_write_some(
                asio::buffer(&m_outcomingBuffer[m_transferred], m_outcomingBuffer.size() - m_transferred), 
                asio::bind_executor(m_strand, std::bind(&Client::OnWrite, this, _1, _2))
            );
        }
        else if( m_transferred == m_outcomingBuffer.size()) {
            m_transferred = 0;
            m_isWriting = false;
        }
    }
    else {
        m_isWriting = false;
    }
}