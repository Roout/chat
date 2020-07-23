#include "../include/Request.hpp"
#include "../include/RequestType.hpp"
#include "../include/Utility.hpp"
#include <string>

namespace Requests {

    struct Request::Impl {
        /**
         * Type of request. 
         */
        RequestType m_type { RequestType::UNDEFINED };
        /**
         * Id of the chatroom which will be joined.
         * Used on JOIN_CHATROOM request.
         */
        size_t m_chatroomId { 0 };
        /**
         * Unique (for server) non-empty name.
         * Used on JOIN_SERVER request.
         */
        std::string m_name {};
        /**
         * Request body.
         * It's either text of chat message either information about chatroom. 
         */
        std::string m_body {};
    };

    Request::Request() : 
        m_impl{ std::make_unique<Impl>() }
    {};
        
    Request::~Request() = default;

    void Request::Reset() {
        m_impl->m_type = RequestType::UNDEFINED;
        m_impl->m_chatroomId = 0;
        m_impl->m_name.clear();
        m_impl->m_body.clear();
    }

    void Request::Parse(const std::string& frame) {
        size_t start { 0 }, end { 0 };
        const size_t skip { DELIMETER.size() };
        // type
        end = frame.find_first_of(DELIMETER);
        const auto type { frame.substr(start, end - start) };
        m_impl->m_type = Util::EnumCast<RequestType>(std::stoi(type));
        // room id
        start = end + skip;
        end = frame.find_first_of(DELIMETER, start);
        const auto room { frame.substr(start, end - start) };
        m_impl->m_chatroomId = std::stoi(room);
        // username
        start = end + skip;
        end = frame.find_first_of(DELIMETER, start);
        m_impl->m_name = frame.substr(start, end - start);
        // body
        start = end + skip;
        end = frame.size() - REQUEST_DELIMETER.size();
        m_impl->m_body = frame.substr(start, end - start);
    }

    std::string Request::Serialize() const {
        std::string result {};
        result += std::to_string(static_cast<size_t>(m_impl->m_type));
        result += DELIMETER;
        result += std::to_string(m_impl->m_chatroomId);
        result += DELIMETER;
        result += m_impl->m_name;
        result += DELIMETER;
        result += m_impl->m_body;
        // end request
        result += REQUEST_DELIMETER;
        return result;
    }

    void Request::SetType(RequestType type) {
        m_impl->m_type = type;
    }

    void Request::SetChatroom(size_t chatroomId) {
        m_impl->m_chatroomId = chatroomId;
    }

    void Request::SetName(const std::string& name) {
        m_impl->m_name = name;
    }

    void Request::SetBody(const std::string& body) {
        m_impl->m_body = body;
    }

    RequestType Request::GetType() const noexcept {
        return m_impl->m_type;
    }

    size_t Request::GetChatroom() const noexcept {
        return m_impl->m_chatroomId;
    }

    const std::string& Request::GetName() const noexcept {
        return m_impl->m_name;
    }

    const std::string& Request::GetBody() const noexcept {
        return m_impl->m_body;
    }
}