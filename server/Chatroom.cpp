#include "Chatroom.hpp"
#include "Session.hpp"

#include <mutex>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace chat {
    
    struct Chatroom::Impl {
        /// Lifetime management
        Impl();

        Impl(const std::string& name);

    public:
        /// Data members

        static constexpr std::size_t MAX_CONNECTIONS { 256 };

        const std::size_t m_id { 0 };

        mutable std::mutex m_mutex;

        std::string m_name {};

        std::size_t m_users { 0 };

        std::array<std::shared_ptr<Session>, MAX_CONNECTIONS> m_sessions;
        
    private:
        inline static std::size_t m_instances { Chatroom::NO_ROOM + 1U };
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

    Chatroom::Chatroom() :
        m_impl { std::make_unique<Impl>() }
    { // default ctor
    }

    Chatroom::Chatroom(const std::string & name) :
        m_impl { std::make_unique<Impl>(name) }
    {
    }

    Chatroom::Chatroom(Chatroom&&rhs) = default;

    Chatroom & Chatroom::operator=(Chatroom&&rhs) = default;

    Chatroom::~Chatroom() {
        bool hasUsers { false };
        {
            std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
            hasUsers = m_impl->m_users > 0U;
        }
        if (!hasUsers) this->Close();
    };

    void Chatroom::Close() {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };

        for (auto& session: m_impl->m_sessions) {
            if (session) { 
                session->Close();
                session.reset();
            }
        };
        m_impl->m_users = 0U;
    }

    std::size_t Chatroom::GetId() const noexcept {
        return m_impl->m_id;
    } 

    std::size_t Chatroom::GetSessionCount() const noexcept {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
        return m_impl->m_users;
    }

    const std::string& Chatroom::GetName() const noexcept {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
        return m_impl->m_name;
    }

    bool Chatroom::AddSession(const std::shared_ptr<Session>& session) {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };

        if (m_impl->m_users >= Chatroom::Impl::MAX_CONNECTIONS) {
            return false;
        }

        for (auto& s: m_impl->m_sessions) {
            if (s == nullptr) {
                s = session;
                m_impl->m_users++;
                break;
            }
        }
        return true;
    }

    bool Chatroom::RemoveSession(const Session * const session) {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };

        for (auto& s: m_impl->m_sessions) {
            if (s.get() == session) {
                s.reset();
                m_impl->m_users--;
                return true;
            }
        }
        return false;
    }

    bool Chatroom::Contains(const Session * const session) const noexcept {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
        for (const auto& s: m_impl->m_sessions) {
            if (s.get() == session) {
                return true;
            }
        }
        return false;
    }

    bool Chatroom::IsEmpty() const noexcept {
        return this->GetSessionCount() == 0ULL;
    }

    std::string Chatroom::AsJSON() const {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("id", m_impl->m_id, allocator);
        {
            std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
            rapidjson::Value value;
            value.SetString(m_impl->m_name.c_str(), allocator);
            doc.AddMember("name", value, allocator);
            doc.AddMember("users", m_impl->m_users, allocator);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();
    } 

    void Chatroom::Rename(const std::string& name) {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
        m_impl->m_name = name;
    }

    void Chatroom::Broadcast(const std::string& text) {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
        for (auto& session: m_impl->m_sessions) {
            if (session && session->IsClosed()) {
                session.reset();
            }
            else if (session) {
                session->Write(text);
            }
        }
    }

    void Chatroom::Broadcast(
        const std::string& text, 
        std::function<bool(const Session&)> predicate
    ) {
        std::lock_guard<std::mutex> lock{ m_impl->m_mutex };
        for (auto& session: m_impl->m_sessions) {
            if (session && session->IsClosed()) {
                session.reset();
            }
            else if (session && std::invoke(predicate, *session)) {
                session->Write(text);
            }
        }
    }

} // namespace chat
