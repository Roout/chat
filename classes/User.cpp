#include "User.hpp"

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"


Internal::User::User() : 
    m_id { m_instance++ } 
{ // default ctor
}

Internal::User::User(std::size_t id) :
    m_id { id }
{ // construct with existing id
}

std::string Internal::User::AsJSON() const {
    rapidjson::Document doc;
    rapidjson::Value value;
    auto& allocator = doc.GetAllocator();
    doc.SetObject();
    
    value.SetUint64(m_id);
    doc.AddMember("id", value, allocator);

    value.SetObject();
    value.AddMember("id", rapidjson::Value{ m_chatroom }, allocator);
    doc.AddMember("chatroom", value, allocator);

    value.SetString(m_username.c_str(), allocator);
    doc.AddMember("name", value, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

Internal::User Internal::User::FromJSON(const std::string& json) {
    rapidjson::Document doc;
    doc.Parse(json.c_str());

    User user { static_cast<std::size_t>(doc["id"].GetUint64()) };
    user.m_chatroom = doc["chatroom"]["id"].GetUint64();
    user.m_username = doc["name"].GetString();

    return user;
}