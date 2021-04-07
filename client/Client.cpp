#include "Client.hpp"
#include "Connection.hpp"

Client::Client(std::shared_ptr<boost::asio::io_context> io) 
    : m_io { io }
    , m_connection { std::make_shared<client::Connection_t>(this->weak_from_this(), io) }
{
}

Client::~Client() {
    if(m_state != State::CLOSED) {
        this->CloseConnection();
    }
}

void Client::Connect(std::string_view path, std::string_view port) {
    m_connection->Connect(path, port);
}

void Client::Write(std::string text) {
    m_connection->Write(std::move(text));
}

void Client::SetState(State state) noexcept {
    std::lock_guard<std::mutex> lock{ m_mutex };
    m_state = state;
}

Client::State Client::GetState() const noexcept {
    std::lock_guard<std::mutex> lock{ m_mutex };
    return m_state;
}

void Client::CloseConnection() {
    m_connection->Close();
}

void Client::HandleMessage(Internal::Response&) {

}
