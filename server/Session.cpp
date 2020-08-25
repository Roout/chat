#include "Session.hpp"
#include "Server.hpp"
#include "Message.hpp"
#include "Utility.hpp"
#include "Chatroom.hpp"

#include <functional>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

Session::Session( 
    asio::ip::tcp::socket && socket, 
    Server * const server 
) :
    m_socket { std::move(socket) },
    m_server { server },
    m_strand { *server->m_context },
    m_user {},
    m_timer { *server->m_context }
{
}

void Session::Write(std::string text) {
    asio::post(m_strand, [text = std::move(text), self = shared_from_this()]() mutable {
        self->m_outbox.Enque(std::move(text));
        if( !self->m_isWriting ) {
            self->Write();
        }
    });
}

void Session::Write() {
    // add all text that is queued for write operation to active buffer
    m_outbox.SwapBuffers();
    // initiate write operation
    m_isWriting = true;
    asio::async_write(
        m_socket,
        m_outbox.GetBufferSequence(),
        asio::bind_executor(
            m_strand,
            std::bind(&Session::WriteSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Session::Read() {
    asio::async_read_until(
        m_socket,
        m_inbox,
        Internal::MESSAGE_DELIMITER,
        asio::bind_executor(
            m_strand, 
            std::bind(&Session::ReadSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Session::ExpiredDeadlineHandler(
    const boost::system::error_code& error
) {
    const bool isCanceled { error == boost::asio::error::operation_aborted };
    if( !error || isCanceled ) {
        if( IsWaitingSyn() ) {
            m_server->Write(LogType::info, "Connection has been closed due to timeout.");
            Close();
        }
        // otherwise the session is either already acknowledged either closed/disconnected. 
    } 
    else {
        // TODO: some weird error
    }
}

void Session::WaitSynchronizationRequest() {
    // start reading requests
    this->Read();
    // set up deadline timer for the SYN request from the accepted connection
    m_timer.expires_from_now(boost::posix_time::milliseconds(m_waitSynTimeout));
    m_timer.async_wait(
        asio::bind_executor(
            m_strand, 
            std::bind(&Session::ExpiredDeadlineHandler, 
                this->shared_from_this(), 
                std::placeholders::_1
            )
        )
    );
}


void Session::Close() {
    /// TODO: get rid of the errors about normal shutdown
    boost::system::error_code ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if(ec) { 
        m_server->Write(LogType::error, 
            "Session's socket called shutdown with error: ", ec.message(), '\n'
        );
        ec.clear();
    }
    m_socket.close(ec);
    if(ec) {
        m_server->Write(LogType::error, 
            "Session's socket is being closed with error: ", ec.message(), '\n'
        );
    } 

    m_state = State::DISCONNECTED;
}

void Session::ReadSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        m_server->Write(LogType::info, 
            "Session just recive: ", transferredBytes, " bytes.\n"
        );
        // boost::asio::async_read_until calls commit by itself
        // m_inbox.commit(transferredBytes);
        
        const auto data { m_inbox.data() };
        std::string recieved {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes - Internal::MESSAGE_DELIMITER.size()
        };
        m_inbox.consume(transferredBytes);
        
        // Handle exceptions
        const auto incomingRequest = Internal::Read(recieved);

        boost::system::error_code ec; 
        m_server->Write(LogType::info, 
            m_socket.remote_endpoint(ec), ": ", recieved, '\n'
        );

        this->HandleMessage(*incomingRequest);
        this->Read();
    } 
    else {
        m_server->Write(LogType::error, 
            "Session trying to read invoked error: ", error.message(), '\n'
        );
    }
}

void Session::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        m_server->Write(LogType::info, 
            "Session sent: ", transferredBytes, " bytes.\n"
        );

        if( m_outbox.GetQueueSize() ) {
            // we need to Write other data
            m_server->Write(LogType::info, 
                "Session need to Write ", m_outbox.GetQueueSize(), " messages.\n"
            );
            asio::post(m_strand, [self = shared_from_this()](){
                self->Write();
            }); 
        } else {
            m_isWriting = false;
        }
    } 
    else /* if(error == boost::asio::error::eof) */ {
        // Connection was closed by the remote peer 
        // or any other error happened 
        m_server->Write(LogType::error, 
            "Session has error trying to write: ", error.message(), "\n"
        );
        this->Close();
    }
}

bool Session::AssignChatroom(std::size_t id) {
    const bool result = m_server->AssignChatroom(id, shared_from_this());
    if( result ) {
        m_user.m_chatroom = id;
    }
    return result;
}

bool Session::LeaveChatroom() {
    // leave current chatroom if exist
    if(m_user.m_chatroom != chat::Chatroom::NO_ROOM) {
        m_server->LeaveChatroom(m_user.m_chatroom, shared_from_this());
        m_user.m_chatroom = chat::Chatroom::NO_ROOM;
        return true;
    }
    return false;
}

void Session::HandleMessage(const Internal::Message& msg) {
    std::string reply {};
    const auto protocol { std::string(msg.GetProtocol()) };
    if( protocol == Internal::Request::PROTOCOL ) {
        const auto request = dynamic_cast< Internal::Request const* >(&msg);
        reply = HandleRequest(request);
        this->Write(reply);
    } 
    else if( protocol == Internal::Chat::PROTOCOL ) {
        const auto chat = dynamic_cast< Internal::Chat const* >(&msg);
        this->HandleChat(chat);
    }
}

template<class Encoding, class Allocator>
std::string Serialize(const rapidjson::GenericValue<Encoding, Allocator>& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return { buffer.GetString(), buffer.GetLength() };
}

Internal::Response Session::HandleSyncRequest(const Internal::Request& request) {
    // confirm timeout
    // confirm that key was sent

    Internal::Response reply;
    reply.m_status = 101;
    reply.m_type = Internal::QueryType::ACK;
    reply.m_timestamp = Utils::GetTimestamp();

    // add accept field

    rapidjson::Document reader;
    auto& alloc = reader.GetAllocator();
    reader.Parse(request.m_attachment.c_str());

    const std::string key { reader["key"].GetString() };

    rapidjson::Value writer(rapidjson::kObjectType);
    writer.AddMember("accept",  rapidjson::Value(key.c_str(), alloc), alloc);
    reply.m_attachment = ::Serialize(writer);
    return reply;
}

std::string Session::HandleRequest(const Internal::Request* request) {
    Internal::Response reply {};
    switch(m_state) {
        case State::WAIT_SYNCHRONIZE_REQUEST:
            if(request->m_type != Internal::QueryType::SYN) {
                // create response with error description
                reply.m_status = 404; // TODO: define apropriate statuses
                reply.m_error = "Expect SYN request";
                reply.m_timestamp = Utils::GetTimestamp();
                reply.m_type = Internal::QueryType::ACK;
            } else {
                reply = this->HandleSyncRequest(*request);
                // TODO: between this state update and actual writing to the socket exception can be raised! 
                // CHECK for the basic safety
                m_state = State::SENT_ACKNOWLEDGED_RESPONSE;
            }
            break;
        case State::SENT_ACKNOWLEDGED_RESPONSE: 
            { // NOTE: ASSUME FOR NOW THAT EVERYTHING WORKS FINE AND NO ERRORS GENERATED
                switch(request->m_type) {
                    case Internal::QueryType::LEAVE_CHATROOM:
                    { 
                        const auto leftRoom = this->LeaveChatroom();
                        reply.m_status = 200; 
                        reply.m_error.clear();
                        reply.m_timestamp = Utils::GetTimestamp();
                        reply.m_type = Internal::QueryType::LEAVE_CHATROOM;
                    } break;
                    case Internal::QueryType::JOIN_CHATROOM:
                    {
                        rapidjson::Document reader;
                        reader.Parse(request->m_attachment.c_str());
                        const auto roomId = reader["chatroom"]["id"].GetUint64();
                        const auto username = reader["user"]["name"].GetString();
                        m_user.m_username = username;
                        const auto joined = this->AssignChatroom(roomId); 

                        reply.m_status = 200; 
                        reply.m_error.clear();
                        reply.m_timestamp = Utils::GetTimestamp();
                        reply.m_type = Internal::QueryType::JOIN_CHATROOM; 
                    } break;
                    case Internal::QueryType::CREATE_CHATROOM:
                    {
                        rapidjson::Document doc;
                        auto& alloc = doc.GetAllocator();
                        doc.Parse(request->m_attachment.c_str());

                        const auto room = doc["chatroom"]["name"].GetString();
                        const auto roomId = m_server->CreateChatroom(room); 
                        if( roomId != chat::Chatroom::NO_ROOM ) {
                            m_user.m_username = doc["user"]["name"].GetString();
                            const auto joined = this->AssignChatroom(roomId); 

                            reply.m_status = joined? 200: 404; 
                            reply.m_error = joined? "":"Failed to join";
                            reply.m_timestamp = Utils::GetTimestamp();
                            reply.m_type = Internal::QueryType::CREATE_CHATROOM; 
                            if( joined ) {
                                rapidjson::Value value(rapidjson::kObjectType);
                                value.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType), alloc);
                                value["chatroom"].AddMember("id", roomId, alloc);
                                reply.m_attachment = ::Serialize(value);
                            }
                        } else {
                            reply.m_status = 404; 
                            reply.m_error = "Failed to create room";
                            reply.m_timestamp = Utils::GetTimestamp();
                            reply.m_type = Internal::QueryType::CREATE_CHATROOM; 
                        }
                    } break;
                    case Internal::QueryType::LIST_CHATROOM: {
                        reply.m_status = 200; 
                        reply.m_error.clear();
                        reply.m_timestamp = Utils::GetTimestamp();
                        reply.m_type = Internal::QueryType::LIST_CHATROOM;
                        auto list = m_server->GetChatroomList(); 
                        reply.m_attachment = "{\"chatrooms\":[";
                        for(auto&& obj: list) {
                            reply.m_attachment += std::move(obj);
                            reply.m_attachment += ",";
                        }
                        if(!list.empty()) {
                            reply.m_attachment.pop_back();
                        }
                        reply.m_attachment += "]}";

                    } break;
                }
            } break;
        case State::DISCONNECTED: break;
        default: break;
    };

    std::string replyStr {};
    reply.Write(replyStr);
    return replyStr;
}

void Session::HandleChat(const Internal::Chat* chat) {
    if(m_state != State::SENT_ACKNOWLEDGED_RESPONSE ) {
        throw "You have no rights to chat";
    }
    // confirm we're withing timeout limit
    // confirm that chat room exist
    // confirm we're in the chat room

    const auto roomId { m_user.m_chatroom };
    const auto userId { m_user.m_id };

    const auto it = m_server->m_chatrooms.find(roomId);
    it->second.Broadcast(chat->m_message, [this](const Session& session) {
        return this != &session;
    });
}
