#include "server.hpp"
#include "Session.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

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
    /// A: As far as I can see I need strand here for ostream object safety...
    m_acceptor.async_accept( *m_socket, [&](const boost::system::error_code& code ) {
        if( !code ) {
            boost::system::error_code msg; 
            std::cerr << "Accepted connection on endpoint: " << m_socket->remote_endpoint(msg) << "\n";
            
            std::stringstream ss;
            ss << "Welcome to my server, user #" << m_socket->remote_endpoint(msg) << '\n';
            ss << "Please, login!";
            std::string welcomeMessage = ss.rdbuf()->str();

            m_sessions.emplace_back(std::make_shared<Session>(std::move(*m_socket), this));
            // welcome new user
            m_sessions.back()->Write(welcomeMessage);
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
        session->Write(text);
    }
}

void Server::BroadcastEveryoneExcept(const std::string& text, std::shared_ptr<const Session> exception) {
    // remove closed sessions
     m_sessions.erase(
        std::remove_if(m_sessions.begin(), m_sessions.end(), [](const auto& session){
            return session->IsClosed();
        }),
        m_sessions.end()
    );

    for(const auto& session: m_sessions) {
        if( exception.get() != session.get()) {
            session->Write(text);
        }
    }
}