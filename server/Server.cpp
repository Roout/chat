#include "Server.hpp"
#include "RoomService.hpp"
#include "Session.hpp"

#include <cassert>
#include <exception>
#include <fstream>

void Server::Config::LoadConfig() {
    std::ifstream in(Config::PATH);
    if (!in.is_open()) {
        assert(false && "TODO: Handle situation when file is absent!");
        exit(1);
    }
    std::string line;
    while (std::getline(in, line)) {
        line = line.substr(line.find('"'));
        line.pop_back();
        if (line == "password") {
            password = std::move(line);
        }
        else if (line == "certificate_chain_file") {
            certificate_chain_file = std::move(line);
        }
        else if (line == "private_key_file") {
            private_key_file = std::move(line);
        }
        else if (line == "tmp_dh_file") {
            tmp_dh_file = std::move(line);
        }
        else {
            assert(false && "LoadConfig: unreachable");
            exit(1);
        }
    }
}

Server::Server(
    std::shared_ptr<asio::io_context> context, 
    std::uint16_t port
) :
    m_context { context },
    m_sslContext { std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23)  },
    m_acceptor { *m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port) },
    m_service { std::make_shared<chat::RoomService>() }
{
    this->SetupSSL();
}

void Server::SetupSSL() {
    m_sslContext->set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use
    );
    m_sslContext->set_password_callback(std::bind(
        &Server::PasswordCallback
        , this
        , std::placeholders::_1
        , std::placeholders::_2)
    );

    try {
        m_config.LoadConfig();
        m_sslContext->use_certificate_chain_file(m_config.certificate_chain_file);
        m_sslContext->use_private_key_file(m_config.private_key_file, boost::asio::ssl::context::pem);
        m_sslContext->use_tmp_dh_file(m_config.tmp_dh_file);
    }
    catch (std::exception const & e) {
        this->Write(LogType::error, "SetupSSL failed:", e.what(), '\n'); 
        exit(1);
    }
}

std::string Server::PasswordCallback(
    std::size_t max_length,  // The maximum size for a password.
    boost::asio::ssl::context::password_purpose purpose // Whether password is for reading or writing.
) {
    return m_config.password;
}

void Server::Start() {
    m_socket.emplace(*m_context);
    m_acceptor.set_option(
        // To avoid exception compiling with github actions:
        // C++ exception with description "bind: Address already in use"
        asio::ip::tcp::acceptor::reuse_address(true)
    );

    m_acceptor.async_accept(*m_socket, [this](const boost::system::error_code& code) {
        if (!code) {
            boost::system::error_code err; 
            this->Write(LogType::info, 
                "Server accepted connection on endpoint:", m_socket->remote_endpoint(err), '\n'
            ); 
            // Session won't live more than room service cuz service was destroyed or closed
            // when all sessions had been closed.
            const auto session { std::make_shared<Session>(
                std::move(*m_socket)
                , m_service
                , m_context
                , m_sslContext) };
            session->Subscribe();
            session->Handshake();
            // TODO: need to add session only after successfull handshake
            if (!m_service->AddSession(session)) {
                // TODO: failed to add new session most likely due to connection limit 
            }
            // wait for the new connections again
            this->Start();
        }
    });
}

void Server::Shutdown() {
    boost::system::error_code error;
    m_acceptor.close(error);
    if (error) {
        this->Write(LogType::error, 
            "Server closed acceptor with error:", error.message(), '\n'
        );
    }
    m_service->Close();
}