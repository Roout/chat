#include "Server.hpp"
#include "RoomService.hpp"
#include "Session.hpp"

Server::Server(std::shared_ptr<asio::io_context> context, std::uint16_t port) :
    m_context { context },
    m_acceptor { *m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port) },
    m_service { std::make_shared<chat::RoomService>() }
{
}

void Server::Start() {
    m_socket.emplace(*m_context);
    m_acceptor.set_option(
        // To avoid exception compiling with github actions:
        // C++ exception with description "bind: Address already in use"
        asio::ip::tcp::acceptor::reuse_address(true)
    );
    m_acceptor.async_accept( *m_socket, [this](const boost::system::error_code& code ) {
        if( !code ) {
            boost::system::error_code err; 
            this->Write(LogType::info, 
                "Server accepted connection on endpoint: ", m_socket->remote_endpoint(err), "\n"
            ); 
            // Session won't live more than room service cuz service was destroyed or closed
            // when all sessions had been closed.
            const auto session { std::make_shared<Session>(std::move(*m_socket), m_service, m_context) };
            session->Subscribe();
            session->Read();
            if(!m_service->AddSession(session)) {
                // TODO: failed to add new session most likely due to connection limit 
            }
            // wait for the new connections again
            this->Start();
        }
    });
}

void Server::Shutdown() {
    boost::system::error_code ec;
    m_acceptor.close(ec);
    if(ec) {
        this->Write(LogType::error, 
            "Server closed acceptor with error: ", ec.message(), "\n"
        );
    }
    m_service->Close();
}