#include "Client.hpp"
#include "Message.hpp"
#include "QueryType.hpp"
#include "Utility.hpp"

#include <iostream>
#include <string_view>
#include <functional> // std::bind

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

Client::Client(std::shared_ptr<asio::io_context> io):
    m_io { io },
    m_strand { *io },
    m_socket { *m_io }
{
}

Client::~Client() {
    if( m_state != State::CLOSED ) {
        this->Close();
    }
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
            std::bind(&Client::OnConnect, this->shared_from_this(), std::placeholders::_1)
        );
    }
}

void Client::Write(std::string && text ) {
    // using strand we prevent concurrent access to variables and 
    // concurrent writing to socket. 
    asio::post(m_strand, [text = std::move(text), self = this->shared_from_this()]() mutable {
        self->m_outbox.Enque(std::move(text));
        if(!self->m_isWriting) {
            self->Write();
        } 
    });
}

void Client::Close() {
    boost::system::error_code error;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, error);
    if(error) {
        m_logger.Write(LogType::error, 
            "Client's socket called shutdown with error: ", error.message(), '\n'
        );
        error.clear();
    }
    
    m_socket.close(error);
    if(error) {
        m_logger.Write(LogType::error, 
            "Client's socket is being closed with error: ", error.message(), '\n'
        );
    }
    m_state = State::CLOSED;
}

void Client::OnConnect(const boost::system::error_code& error) {
    if(error) {
        m_logger.Write(LogType::error, 
            "Client failed to connect with error: ", error.message(), "\n" 
        );
    } 
    else {
        m_logger.Write(LogType::info, "Client connected successfully!\n");
        // send SYN
        this->Synchronize();
        // start waiting incoming calls
        this->Read();
    }
}

void Client::Read() {
    asio::async_read_until(
        m_socket,
        m_inbox,
        Internal::MESSAGE_DELIMITER,
        asio::bind_executor(
            m_strand,
            std::bind(&Client::OnRead, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Client::OnRead(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        m_logger.Write(LogType::info, 
            "Client just recive: ", transferredBytes, " bytes.\n"
        );
        
        const auto data { m_inbox.data() }; // asio::streambuf::const_buffers_type
        std::string received {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes - Internal::MESSAGE_DELIMITER.size()
        };
        
        m_inbox.consume(transferredBytes);
        
        Internal::Response incomingResponse;
        incomingResponse.Read(received);
        
        boost::system::error_code error; 
        m_logger.Write( 
            LogType::info, 
            m_socket.remote_endpoint(error), ": ", received, '\n' 
        );

        this->HandleMessage(incomingResponse);
        this->Read();
    } 
    else {
        m_logger.Write(LogType::error, 
            "Client failed to read with error: ", error.message(), "\n"
        );
        this->Close();
    }
}

void Client::OnWrite(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    using namespace std::placeholders;

    if(!error) {
        m_logger.Write(LogType::info, 
            "Client just sent: ", transferredBytes, " bytes\n"
        );

        if(m_outbox.GetQueueSize()) {
            // we need to send other data
            asio::post(m_strand, [self = this->shared_from_this()](){
                self->Write();
            });
        } 
        else {
            m_isWriting = false;
        }
    } 
    else {
        m_isWriting = false;
        m_logger.Write(LogType::error, 
            "Client has error on writting: ", error.message(), '\n'
        );
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
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Client::Synchronize() {
    Internal::Request request;
    request.m_timeout = 30;
    request.m_timestamp = Utils::GetTimestamp();
    request.m_query = Internal::QueryType::SYN;
    
    rapidjson::Document d;
    const char* key = "234A$F(K(J@Jjsij2dk2k(@#KDfikwoik";
    d.SetObject().AddMember("key", rapidjson::Value(key, d.GetAllocator()), d.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    request.m_attachment = std::string(buffer.GetString(), buffer.GetSize());

    std::string serialized{};
    request.Write(serialized);
    this->Write(std::move(serialized));

    m_state = State::WAIT_ACK;
}

/// TODO: remove this temporary stuff
static bool Proccess(const char * key, const char* accepted) noexcept {
    return true;
}

void Client::HandleMessage(Internal::Response& response) {
    switch(m_state) {
        case State::WAIT_ACK: {
            bool isAckQuery { response.m_query == Internal::QueryType::ACK };
            bool hasValidStatus { response.m_status == 200 };
            bool hasValidKey { ::Proccess(nullptr, nullptr) };
            // confirm status
            // confirm whether it's our server (proccessing the key)
            if( isAckQuery && hasValidStatus && hasValidKey) { // it's temporary
                m_state = State::RECIEVE_ACK;
                m_gui.UpdateResponse(std::move(response));
            } 
            else {
                this->Close();
            }
        } break;
        case State::RECIEVE_ACK: {
            switch(response.m_query) {
                case Internal::QueryType::ACK: {
                    m_gui.UpdateResponse(std::move(response));
                } break;
                case Internal::QueryType::LIST_CHATROOM: {   
                    m_gui.UpdateResponse(std::move(response));
                } break;
                case Internal::QueryType::JOIN_CHATROOM: {
                    m_gui.UpdateResponse(std::move(response));
                } break;
                case Internal::QueryType::CREATE_CHATROOM: {
                    m_gui.UpdateResponse(std::move(response));
                } break;
                case Internal::QueryType::LEAVE_CHATROOM: {
                    m_gui.UpdateResponse(std::move(response));
                } break;
                case Internal::QueryType::CHAT_MESSAGE: {
                    m_gui.UpdateResponse(std::move(response));
                } break;
                default: break;
            }
        } break;
        case State::CLOSED: break;
        default: break;
    }
}
