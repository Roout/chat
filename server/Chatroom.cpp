#include "Chatroom.hpp"
#include "Session.hpp"

namespace chat {
    
    struct Chatroom::Impl {

        Impl();

        Impl(const std::string& name );

        const size_t m_id { 0 };
        std::string m_name {};

        static constexpr size_t MAX_CONNECTIONS { 256 };
        std::array<std::shared_ptr<Session>, MAX_CONNECTIONS> m_sessions;
        
    private:
        inline static size_t m_instances { Chatroom::NO_ROOM };
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


size_t chat::Chatroom::GetId() const noexcept {
    return m_impl->m_id;
} 

size_t chat::Chatroom::GetSessionCount() const noexcept {
    size_t count { 0 };
    for(auto& s: m_impl->m_sessions) if (s) count++;
    return count;
}

/**
 * @return 
 *      The indication whether the session was insert successfully or not. 
 */
bool chat::Chatroom::AddSession(const std::shared_ptr<Session>& session) {
    for(auto& s: m_impl->m_sessions) {
        if( !s ) {
            s = session;
            return true;
        }
    }
    return false;
}

bool chat::Chatroom::RemoveSession(const Session * const session) {
    for(auto& s: m_impl->m_sessions) {
        if( s.get() == session ) {
            s.reset();
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


std::string chat::Chatroom::GetRepresentation() const {
    std::string view = "{ \"id\": " + std::to_string(m_impl->m_id)
        + ", \"name\": \"" + m_impl->m_name + "\""
        + ", \"users\": " + std::to_string(this->GetSessionCount()) 
        + " }";
    return view;
} 

void chat::Chatroom::Rename(const std::string& name) {
    m_impl->m_name = name;
}

void chat::Chatroom::Broadcast(const std::string& text) {
    // remove closed sessions
    for(auto& session: m_impl->m_sessions) {
        if(session && session->IsClosed()) session.reset();
    }

    for(const auto& session: m_impl->m_sessions) {
        session->Write(text);
    }
}

void chat::Chatroom::Broadcast(
    const std::string& text, 
    std::function<bool(const Session&)> predicate
) {
    // remove closed sessions
    for(auto& session: m_impl->m_sessions) {
        if(session && session->IsClosed()) session.reset();
    };

    for(const auto& session: m_impl->m_sessions) {
        if( std::invoke(predicate, *session) ) {
            session->Write(text);
        }
    }
}