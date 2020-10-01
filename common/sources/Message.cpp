#include "Message.hpp"
#include "QueryType.hpp"
#include <string>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace {

    std::string AsString(const Internal::QueryType ty) {
        static std::map<Internal::QueryType, std::string> mapped = {
            { Internal::QueryType::UNDEFINED,         "undefined" },
            { Internal::QueryType::LEAVE_CHATROOM,    "leave-chatroom" },
            { Internal::QueryType::JOIN_CHATROOM,     "join-chatroom" },
            { Internal::QueryType::CREATE_CHATROOM,   "create-chatroom" },
            { Internal::QueryType::LIST_CHATROOM,     "list-chatroom" },
            { Internal::QueryType::CHAT_MESSAGE,      "chat-message" },
            { Internal::QueryType::SYN,               "syn" },
            { Internal::QueryType::ACK,               "ack" }
        };
        return mapped.at(ty);
    }

    Internal::QueryType AsQueryType(const std::string& str) {
        static std::map<std::string, Internal::QueryType> mapped = {
            { "undefined",          Internal::QueryType::UNDEFINED },
            { "join-chatroom",      Internal::QueryType::JOIN_CHATROOM },
            { "leave-chatroom",     Internal::QueryType::LEAVE_CHATROOM },
            { "create-chatroom",    Internal::QueryType::CREATE_CHATROOM },
            { "list-chatroom",      Internal::QueryType::LIST_CHATROOM },
            { "chat-message",       Internal::QueryType::CHAT_MESSAGE },
            { "syn",                Internal::QueryType::SYN },
            { "ack",                Internal::QueryType::ACK }
        };
        return mapped.at(str);
    }    
}

namespace Internal {

    void Request::Read(const std::string& json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());

        m_query = ::AsQueryType(doc["query"].GetString());
        m_timestamp = doc["timestamp"].GetInt64();
        m_timeout = doc["timeout"].GetUint64();

        if( doc.HasMember("attachment") ) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc["attachment"].Accept(writer);
            m_attachment = std::string(buffer.GetString(), buffer.GetSize());
        } 
    }

    void Request::Write(std::string& json) const {
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.SetObject();

        rapidjson::Value value;
     
        const auto query = ::AsString(m_query);
        value.SetString(query.c_str(), alloc);
        doc.AddMember("query", value, alloc);

        value.SetInt64(m_timestamp);
        doc.AddMember("timestamp", value, alloc);

        value.SetUint64(m_timeout);
        doc.AddMember("timeout", value, alloc);

        // read to string:
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        json = std::string(buffer.GetString(), buffer.GetSize());
        if( !m_attachment.empty() ) {
            json.pop_back(); // remove '}'
            json += ",\"attachment\":";
            json += m_attachment;
            json += "}";
        }
        json += MESSAGE_DELIMITER;
    }
    
    void Response::Read(const std::string & json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());

        m_query = ::AsQueryType(doc["query"].GetString());
        m_timestamp = doc["timestamp"].GetInt64();
        m_status = doc["status"].GetInt();
        
        if(doc.HasMember("error")) {
            m_error = doc["error"].GetString();
        } else {
            m_error.clear();
        }
        
        if( doc.HasMember("attachment") ) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc["attachment"].Accept(writer);
            m_attachment = std::string(buffer.GetString(), buffer.GetSize());
        } 
    }

    void Response::Write(std::string& json) const {
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.SetObject();

        rapidjson::Value value;
    
        const auto query = ::AsString(m_query);
        value.SetString(query.c_str(), alloc);
        doc.AddMember("query", value, alloc);

        value.SetInt64(m_timestamp);
        doc.AddMember("timestamp", value, alloc);

        value.SetInt(m_status);
        doc.AddMember("status", value, alloc);

        if( !m_error.empty() ) {
            value.SetString(m_error.c_str(), alloc);
            doc.AddMember("error", value, alloc);
        }

        // read to string:
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        json = std::string(buffer.GetString(), buffer.GetSize());
        if( !m_attachment.empty() ) {
            json.pop_back(); // remove '}'
            json += ",\"attachment\":";
            json += m_attachment;
            json += "}";
        }
        json += MESSAGE_DELIMITER;
    }
    
};