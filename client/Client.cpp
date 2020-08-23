#include "Client.hpp"
#include "Message.hpp"
#include "QueryType.hpp"
#include "Utility.hpp"

#include <iostream>
#include <string_view>
#include <functional> // std::bind

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

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
    m_state = State::CLOSED;
}

void Client::OnConnect(const boost::system::error_code& err) {
    if(err) {
        m_logger.Write( 
            LogType::error, 
            "Client failed to connect with error: ", err.message(), "\n" 
        );
    } 
    else {
        m_logger.Write( 
            LogType::info, 
            "Client connected successfully!\n"
        );
        // send SYN
        this->SendSynRequest();
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
            std::bind(&Client::OnRead, this, std::placeholders::_1, std::placeholders::_2)
        )
    );
}

void Client::OnRead(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if( !error ) {
        m_logger.Write( 
            LogType::info, 
            "Client just recive: ", transferredBytes, " bytes.\n"
        );
        
        // boost::asio::async_read_until calls commit by itself 
        // m_inbox.commit(transferredBytes);
        
        const auto data { m_inbox.data() }; // asio::streambuf::const_buffers_type
        std::string recieved {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes - Internal::MESSAGE_DELIMITER.size()
        };
        
        m_inbox.consume(transferredBytes);
        
        Internal::Response incomingResponse {};
        incomingResponse.Read(recieved);
        
        boost::system::error_code error; 
        m_logger.Write( 
            LogType::info, 
            m_socket.remote_endpoint(error), ": ", recieved, '\n' 
        );

        this->HandleMessage(incomingResponse);
        this->Read();
    } 
    else {
        m_logger.Write( 
            LogType::error, 
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

    if( !error ) {
        m_logger.Write( 
            LogType::info, 
            "Client just sent: ", transferredBytes, " bytes\n"
        );

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
        m_logger.Write( 
            LogType::error, 
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
                this, 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

static bool Proccess(const char * key, const char* accepted) noexcept {
    return true;
}

void Client::SendSynRequest() {
    Internal::Request request;
    request.m_timeout = 30;
    request.m_timestamp = Utils::GetTimestamp();
    request.m_type = Internal::QueryType::SYN;
    
    rapidjson::Document d;
    const char* key = "234A$F(K(J@Jjsij2dk2k(@#KDfikwoik";
    d.SetObject().AddMember("key", rapidjson::Value(key, d.GetAllocator()), d.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    request.m_attachment = std::string(buffer.GetString(), buffer.GetLength());

    std::string serialized{};
    request.Write(serialized);
    this->Write(std::move(serialized));

    m_state = State::WAIT_ACK;
}

void Client::HandleMessage(Internal::Message& msg) {
    const auto protocol { std::string(msg.GetProtocol()) };
    if( protocol == Internal::Chat::PROTOCOL) {
        const auto chat = dynamic_cast<Internal::Chat *>(&msg);
        if( m_state == State::RECIEVE_ACK ) {
            // confirm we're at room
            m_gui.UpdateChat(std::move(*chat));
        }
    }
    else if( protocol == Internal::Response::PROTOCOL) {
        const auto response = dynamic_cast<Internal::Response *>(&msg);
        switch(m_state) {
            case State::WAIT_ACK: 
            {
                if(response->m_type == Internal::QueryType::ACK) {
                    // confirm status
                    // confirm whether it's our server (proccessing the key)
                    if( ::Proccess(nullptr, nullptr) ) { // it's temporary
                        m_state = State::RECIEVE_ACK;
                        m_gui.UpdateResponse(std::move(*response));
                    }
                }
                else {
                    this->Close();
                }
            } break;
            case State::RECIEVE_ACK: 
            {
                switch(response->m_type) {
                    case Internal::QueryType::ACK: {
                        // error
                        m_gui.UpdateResponse(std::move(*response));
                    } break;
                    case Internal::QueryType::LIST_CHATROOM: {   
                        m_gui.UpdateResponse(std::move(*response));
                    } break;
                    case Internal::QueryType::JOIN_CHATROOM: {
                        m_gui.UpdateResponse(std::move(*response));
                    } break;
                    case Internal::QueryType::CREATE_CHATROOM: {
                        m_gui.UpdateResponse(std::move(*response));
                    } break;
                    case Internal::QueryType::LEAVE_CHATROOM: {
                        m_gui.UpdateResponse(std::move(*response));
                    } break;
                    default: break;
                }
            } break;
            case State::CLOSED: break;
            default: break;
        }
    }
    else {
        // error
    }
}
