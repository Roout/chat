#include "Client.hpp"
#include "Connection.hpp"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <iostream>

Client::Client(
    std::shared_ptr<boost::asio::io_context> io
    , std::shared_ptr<boost::asio::ssl::context> sslContext
) 
    : m_io { io }
    , m_sslContext { sslContext }
    , m_connection { nullptr }
{
    boost::system::error_code error;
	m_sslContext->load_verify_file("settings/server.crt", error);
	if (error) {
		std::cerr << "[ERROR] Can't find <settings/server.crt> file.\n";
		exit(1);
	}
}

Client::~Client() {
    if(m_state != State::CLOSED) {
        this->CloseConnection();
    }
}

void Client::Connect(std::string_view path, std::string_view port) {
    m_connection = std::make_shared<client::Connection_t>(this->weak_from_this(), m_io, m_sslContext);
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

Internal::Response Client::GetLastResponse() const noexcept {
    std::lock_guard<std::mutex> lock { m_mutex };
    return m_response;
}

void Client::HandleMessage(Internal::Response&& response) {
    m_response = std::move(response);
    std::lock_guard<std::mutex> lock{ m_mutex };
    switch(m_state) {
        case State::WAIT_ACK: {
        } break;
        case State::RECEIVE_ACK: {
            // TODO: send to gui or smth
            switch(m_response.m_query) {
                case Internal::QueryType::ACK:
                case Internal::QueryType::LIST_CHATROOM:
                case Internal::QueryType::JOIN_CHATROOM:
                case Internal::QueryType::CREATE_CHATROOM:
                case Internal::QueryType::LEAVE_CHATROOM:
                case Internal::QueryType::CHAT_MESSAGE:;
                default: break;
            }
        } break;
        case State::CLOSED: 
        case State::CONNECTED: break;
        default: break;
    }
}
