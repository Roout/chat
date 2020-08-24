#include "Message.hpp"
#include "QueryType.hpp"
#include <string>
#include <map>

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

namespace {

    std::string AsString(const Internal::QueryType ty) {
        static std::map<Internal::QueryType, std::string> mapped = {
            { Internal::QueryType::UNDEFINED,         "undefined" },
            { Internal::QueryType::LEAVE_CHATROOM,    "leave-chatroom" },
            { Internal::QueryType::JOIN_CHATROOM,     "join-chatroom" },
            { Internal::QueryType::CREATE_CHATROOM,   "create-chatroom" },
            { Internal::QueryType::LIST_CHATROOM,     "list-chatroom" },
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
            { "syn",                Internal::QueryType::SYN },
            { "ack",                Internal::QueryType::ACK }
        };
        return mapped.at(str);
    }    
}

namespace Internal {

    void Request::Read(rapidjson::Document& doc) {
        m_type = ::AsQueryType(doc["type"].GetString());
        m_timestamp = doc["timestamp"].GetInt64();
        m_timeout = doc["timeout"].GetUint64();

        if( doc.HasMember("body") ) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc["body"].Accept(writer);
            m_attachment = std::string(buffer.GetString(), buffer.GetLength());
        } 
    }

    void Request::Write(std::string& json) {
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.SetObject();

        rapidjson::Value value;
        
        value.SetString(PROTOCOL, alloc);
        doc.AddMember("protocol", value, alloc);

        value.SetString(TAG, alloc);
        doc.AddMember("tag", value, alloc);

        const auto type = ::AsString(m_type);
        value.SetString(type.c_str(), alloc);
        doc.AddMember("type", value, alloc);

        value.SetInt64(m_timestamp);
        doc.AddMember("timestamp", value, alloc);

        value.SetUint64(m_timeout);
        doc.AddMember("timeout", value, alloc);

        // read to string:
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        json = std::string(buffer.GetString(), buffer.GetLength());
        if( !m_attachment.empty() ) {
            json.pop_back(); // remove '}'
            json += ",\"body\":";
            json += m_attachment;
            json += "}";
        }
        json += MESSAGE_DELIMITER;
    }
    
    void Response::Read(rapidjson::Document& doc) {
        m_type = ::AsQueryType(doc["type"].GetString());
        m_timestamp = doc["timestamp"].GetInt64();
        m_status = doc["status"].GetInt();
        
        if(doc.HasMember("error")) {
            m_error = doc["error"].GetString();
        } else {
            m_error.clear();
        }
        
        if( doc.HasMember("body") ) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc["body"].Accept(writer);
            m_attachment = std::string(buffer.GetString(), buffer.GetLength());
        } 
    }

    void Response::Write(std::string& json) {
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.SetObject();

        rapidjson::Value value;
        
        value.SetString(PROTOCOL, alloc);
        doc.AddMember("protocol", value, alloc);

        value.SetString(TAG, alloc);
        doc.AddMember("tag", value, alloc);

        const auto type = ::AsString(m_type);
        value.SetString(type.c_str(), alloc);
        doc.AddMember("type", value, alloc);

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

        json = std::string(buffer.GetString(), buffer.GetLength());
        if( !m_attachment.empty() ) {
            json.pop_back(); // remove '}'
            json += ",\"body\":";
            json += m_attachment;
            json += "}";
        }
        json += MESSAGE_DELIMITER;
    }

    void Chat::Read(rapidjson::Document& doc) {
        m_timestamp = doc["timestamp"].GetInt64();
        m_timeout = doc["timeout"].GetUint64();
        m_message = doc["message"].GetString();
    }

    void Chat::Write(std::string& json) {
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        doc.SetObject();

        rapidjson::Value value;
        
        value.SetString(PROTOCOL, alloc);
        doc.AddMember("protocol", value, alloc);

        value.SetString(TAG, alloc);
        doc.AddMember("tag", value, alloc);

        value.SetInt64(m_timestamp);
        doc.AddMember("timestamp", value, alloc);

        value.SetUint64(m_timeout);
        doc.AddMember("timeout", value, alloc);

        value.SetString(m_message.c_str(), alloc);
        doc.AddMember("message", value, alloc);

        // read to string:
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        json = std::string(buffer.GetString(), buffer.GetLength());
        json += MESSAGE_DELIMITER;
    }

    std::unique_ptr<Message> Read(const std::string& json) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        const std::string tag = doc["tag"].GetString();
        // dispatch base on tag
        std::unique_ptr<Message> message {};
        if( tag == Response::TAG ) {
            message = std::make_unique<Response>();
        } else if( tag == Request::TAG ) {
            message = std::make_unique<Request>();
        } else if( tag == Chat::TAG ) {
            message = std::make_unique<Chat>();
        }
        message->Read(doc);
        return message;
    }

};