#include "Chatroom.hpp"
#include "Session.hpp"

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

namespace chat {
    
    struct Chatroom::Impl {
        /// Lifetime management
        Impl();

        Impl(const std::string& name );

    public:
        /// Data members
        const std::size_t m_id { 0 };

        std::string m_name {};

        std::size_t m_users { 0 };

        static constexpr std::size_t MAX_CONNECTIONS { 256 };
        std::array<std::shared_ptr<Session>, MAX_CONNECTIONS> m_sessions;
        
    private:
        inline static std::size_t m_instances { Chatroom::NO_ROOM };
    };

    Chatroom::Impl::Impl() : 
        m_id { m_instances++ },
        m_name { "default-chatroom" }    
    { // default ctor
    }

    Chatroom::Impl::Impl(const std::string& name ): 
        m_id { m_instances++ },
        m_name { name }
    {}

}

chat::Chatroom::Chatroom() :
    m_impl { std::make_unique<Impl>() }
{ // default ctor
}

chat::Chatroom::Chatroom(const std::string & name) :
    m_impl { std::make_unique<Impl>(name) }
{
}

chat::Chatroom::Chatroom(chat::Chatroom&&rhs) = default;

chat::Chatroom & chat::Chatroom::operator=(Chatroom&&rhs) = default;

chat::Chatroom::~Chatroom() = default;

void chat::Chatroom::Close() {
    for(auto& s: m_impl->m_sessions) {
        if(s) s->Close();
    };
}


std::size_t chat::Chatroom::GetId() const noexcept {
    return m_impl->m_id;
} 

std::size_t chat::Chatroom::GetSessionCount() const noexcept {
    return m_impl->m_users;
}

const std::string& chat::Chatroom::GetName() const noexcept {
    return m_impl->m_name;
}


/**
 * @return 
 *      The indication whether the session was insert successfully or not. 
 */
bool chat::Chatroom::AddSession(const std::shared_ptr<Session>& session) {
    for(auto& s: m_impl->m_sessions) {
        if( !s ) {
            s = session;
            m_impl->m_users++;
            return true;
        }
    }
    return false;
}

bool chat::Chatroom::RemoveSession(const Session * const session) {
    for(auto& s: m_impl->m_sessions) {
        if( s.get() == session ) {
            s.reset();
            m_impl->m_users--;
            return true;
        }
    }
    return false;
}

bool chat::Chatroom::Contains(const Session * const session) const noexcept {
    for(const auto& s: m_impl->m_sessions) {
        if( s.get() == session ) {
            return true;
        }
    }
    return false;
}

bool chat::Chatroom::IsEmpty() const noexcept {
    return !this->GetSessionCount();
}

std::string chat::Chatroom::AsJSON() const {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("id", m_impl->m_id, allocator);

    rapidjson::Value value;
    value.SetString(m_impl->m_name.c_str(), allocator);
    doc.AddMember("name", value, allocator);

    doc.AddMember("users", this->GetSessionCount(), allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
} 

void chat::Chatroom::Rename(const std::string& name) {
    m_impl->m_name = name;
}

void chat::Chatroom::Broadcast(const std::string& text) {
     for(auto& session: m_impl->m_sessions) {
        if(session && session->IsClosed()) {
            session.reset();
        }
        else if (session) {
            session->Write(text);
        }
    }
}

void chat::Chatroom::Broadcast(
    const std::string& text, 
    std::function<bool(const Session&)> predicate
) {
    for(auto& session: m_impl->m_sessions) {
        if(session && session->IsClosed()) {
            session.reset();
        }
        else if ( session && std::invoke(predicate, *session) ) {
            session->Write(text);
        }
    }
}