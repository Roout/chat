#include "server.hpp"
#include "client.hpp"
#include <functional>
#include <iostream>

void Session::Send(const std::string& text) {
    m_buffer = text;
    m_isWriting = true;
    m_socket.async_write_some(
        asio::buffer(m_buffer), 
        std::bind(&Session::WriteSomeHandler, 
            this->shared_from_this(), 
            std::placeholders::_1, 
            std::placeholders::_2
        )
    );
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
                std::bind(&Session::WriteSomeHandler, 
                    this->shared_from_this(), 
                    std::placeholders::_1, 
                    std::placeholders::_2
                )
            );
        }
    } 
    else if(error == boost::asio::error::eof) {
        // Connection was closed by the remote peer
        std::cerr<< error.message() << "\n";
        m_isWriting = false;
    }
    else {
        // Something went wrong
        std::cerr<< error.message() << "\n";
        m_isWriting = false;
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
    m_acceptor.async_accept( *m_socket, [&](const boost::system::error_code& code ) {
        if( !code ) {
            boost::system::error_code msg; 
            std::cerr << "Accepted connection on endpoint: " << m_socket->remote_endpoint(msg) << "\n";

            m_sessions.emplace_back(std::make_shared<Session>(std::move(*m_socket)));
            // welcome new user
            this->Broadcast("Everybody, welcome new user to my server!");
            // wait for the new connections again
            this->Start();
        }
    });
}

void Server::Shutdown() {
    boost::system::error_code ec;
    
    m_socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if(ec) {
        std::cerr<< ec.message() << "\n";
    }
    ec.clear();
    
    m_socket->close(ec);
    if(ec) {
        std::cerr<< ec.message() << "\n";
    }
    ec.clear();

    m_acceptor.close(ec);
    if(ec) {
        std::cerr<< ec.message() << "\n";
    }
}

void Server::Broadcast(const std::string& text) {
    for(const auto& session: m_sessions) {
        session->Send(text);
    }
}