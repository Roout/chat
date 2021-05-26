#include "RequestHandlers.hpp"
#include "Session.hpp"
#include "Chatroom.hpp"

#include "Utility.hpp"

#include <string>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

template<class Encoding, class Allocator>
std::string Serialize(const rapidjson::GenericValue<Encoding, Allocator>& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return { buffer.GetString(), buffer.GetSize() };
}

/// Implementation  
namespace RequestHandlers {
    
    Executor::Executor(const Request *request, Session *service) :
        m_request { request },
        m_service { service }
    {}

    void Executor::Run() {
        if (this->IsValidRequest()) {
            this->ExecuteRequest();
        }
        this->SendResponse();
    }

    void Executor::SendResponse() {
        // make timestamp
        m_reply.m_timestamp = Utils::GetTimestamp();        
        // send
        std::string m_replyStr {};
        m_reply.Write(m_replyStr);
        // que m_reply for send operation!
        m_service->Write(m_replyStr);
    }

    bool LeaveChatroom::IsValidRequest() {
        m_reply.m_query = QueryType::LEAVE_CHATROOM;
        // - only acknowledged client can join/leave the room
        if (m_service->IsAcknowleged()) {
            m_reply.m_status = 200;
        }
        else {
            m_reply.m_status = 424; // Failed Dependency
            m_reply.m_error = "Require acknowledgement";
        }
        return m_reply.m_status == 200;
    };

    void LeaveChatroom::ExecuteRequest() {
        const auto leftRoom = m_service->LeaveChatroom();
        if (leftRoom == chat::Chatroom::NO_ROOM) {
            m_reply.m_status = 424;  // Failed Dependency
            m_reply.m_error = "Must belong to chatroom";
        }
    };

    bool JoinChatroom::IsValidRequest() {
        m_reply.m_query = QueryType::JOIN_CHATROOM;

        if (m_service->IsAcknowleged() == false) {
            m_reply.m_status = 424; // Failed Dependency
            m_reply.m_error = "Require acknowledgement";
        }
        else if (m_service->GetUser().m_chatroom == chat::Chatroom::NO_ROOM) {
            // is in hall
            m_reply.m_status = 200;
        }
        else {
            m_reply.m_status = 405; //  Method Not Allowed;
            m_reply.m_error = "Already in the chatroom.";
        }

        return m_reply.m_status == 200;
    };

    void JoinChatroom::ExecuteRequest() {
        rapidjson::Document reader;
        reader.Parse(m_request->m_attachment.c_str());
        const auto roomId = reader["chatroom"]["id"].GetUint64();

        if (m_service->AssignChatroom(static_cast<size_t>(roomId))) {
            m_service->UpdateUsername(reader["user"]["name"].GetString());
        }
        else {
            m_reply.m_status = 400;
            m_reply.m_error = "Can't join this room yet";
        }
    };

    bool CreateChatroom::IsValidRequest() {
        m_reply.m_query = QueryType::CREATE_CHATROOM;
        
        if (m_service->IsAcknowleged() == false) {
            m_reply.m_status = 424; // Failed Dependency
            m_reply.m_error = "Require acknowledgement";
        }
        else if (m_service->GetUser().m_chatroom == chat::Chatroom::NO_ROOM) {
            // is in hall
            m_reply.m_status = 200;
        }
        else {
            m_reply.m_status = 405; //  Method Not Allowed;
            m_reply.m_error = "Already in the chatroom.";
        }

        return m_reply.m_status == 200;
    };

    void CreateChatroom::ExecuteRequest() {

        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.Parse(m_request->m_attachment.c_str());

        const std::string roomName = doc["chatroom"]["name"].GetString();
        const auto roomId = m_service->CreateChatroom(roomName); 
        if (m_service->AssignChatroom(roomId)) {
            m_service->UpdateUsername(doc["user"]["name"].GetString());

            rapidjson::Value value(rapidjson::kObjectType);
            value.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType), alloc);
            value["chatroom"].AddMember("id", roomId, alloc);
            m_reply.m_attachment = ::Serialize(value);
        }
        else {
            m_reply.m_status = 500;
            m_reply.m_error = "Server error";
        }
    };

    bool ListChatroom::IsValidRequest() {
        m_reply.m_query = QueryType::LIST_CHATROOM;

        if (m_service->IsAcknowleged()) {
            m_reply.m_status = 200;
        }
        else {
            m_reply.m_status = 424; // Failed Dependency
            m_reply.m_error = "Require acknowledgement";
        }

        return m_reply.m_status == 200;
    };

    void ListChatroom::ExecuteRequest() {
        auto list = m_service->GetChatroomList(); 
        m_reply.m_attachment = "{\"chatrooms\":[";
        for(auto&& obj: list) {
            m_reply.m_attachment += std::move(obj);
            m_reply.m_attachment += ",";
        }
        if (!list.empty()) {
            m_reply.m_attachment.pop_back();
        }
        m_reply.m_attachment += "]}";

    };

    bool ChatMessage::IsValidRequest() {
        m_reply.m_query = QueryType::CHAT_MESSAGE;
        
        if (m_service->IsAcknowleged() == false) {
            m_reply.m_status = 424; // Failed Dependency
            m_reply.m_error = "Require acknowledgement";
        }
        else if (m_service->GetUser().m_chatroom != chat::Chatroom::NO_ROOM) {
            m_reply.m_status = 200;
        }
        else {
            m_reply.m_status = 405; //  Method Not Allowed;
            m_reply.m_error = "Already in the chatroom.";
        }

        return m_reply.m_status == 200;
    };

    void ChatMessage::ExecuteRequest() {
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.Parse(m_request->m_attachment.c_str());

        const std::string message = doc["message"].GetString();
        // build chat message for the other users
        Response chatMessage {};
        chatMessage.m_query = QueryType::CHAT_MESSAGE;
        chatMessage.m_status = 200;
        chatMessage.m_timestamp = Utils::GetTimestamp();
        chatMessage.m_attachment = "{\"message\":\""+ message +"\"}";

        std::string serialized {};
        chatMessage.Write(serialized);
        // broadcast to every user in the chatroom
        m_service->BroadcastOnly(serialized, [session = m_service](const Session& s){
            return session != &s;
        });
    };
}