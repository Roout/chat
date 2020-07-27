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
         * State of communication between client adn server
         */
        IStage::State m_stage { IStage::State::DISCONNECTED };
        /**
         * This is code used for RequestType::CONFIRM.
         */
        ErrorCode m_code { ErrorCode::SUCCESS };
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
        m_impl->m_stage = IStage::State::DISCONNECTED;
        m_impl->m_code = ErrorCode::SUCCESS;
        m_impl->m_chatroomId = 0;
        m_impl->m_name.clear();
        m_impl->m_body.clear();
    }

    int Request::Parse(const std::string& frame) {
        this->Reset();
        
        size_t start { 0 }, end { 0 };
        const size_t skip { DELIMETER.size() };
        // type
        end = frame.find(DELIMETER, start);
        if( end == std::string::npos) {
            return 1;
        }
        const auto type { frame.substr(start, end - start) };
        m_impl->m_type = Utils::EnumCast<RequestType>(std::stoi(type));
        // stage
        start = end + skip;
        end = frame.find(DELIMETER, start);
        if( end == std::string::npos) {
            return 2;
        }
        const auto stage { frame.substr(start, end - start) };
        m_impl->m_stage = Utils::EnumCast<IStage::State>(std::stoi(stage));
        // error code
        start = end + skip;
        end = frame.find(DELIMETER, start);
        if( end == std::string::npos) {
            return 3;
        }
        const auto code { frame.substr(start, end - start) };
        m_impl->m_code = Utils::EnumCast<ErrorCode>(std::stoi(code));
        // room id
        start = end + skip;
        end = frame.find(DELIMETER, start);
        if( end == std::string::npos) {
            return 4;
        }
        const auto room { frame.substr(start, end - start) };
        m_impl->m_chatroomId = std::stoi(room);
        // username
        start = end + skip;
        end = frame.find(DELIMETER, start);
        if( end == std::string::npos) {
            return 5;
        }
        m_impl->m_name = frame.substr(start, end - start);
        // body 
        start = end + skip;
        end = frame.find(REQUEST_DELIMETER, start);
        if( end == std::string::npos) {
            return 6;
        }
        m_impl->m_body = frame.substr(start, end - start);

        return 0;
    }

    std::string Request::Serialize() const {
        std::string result {};
        result += std::to_string(static_cast<size_t>(m_impl->m_type));
        result += DELIMETER;
        result += std::to_string(static_cast<size_t>(m_impl->m_stage));
        result += DELIMETER;
        result += std::to_string(static_cast<size_t>(m_impl->m_code));
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

    void Request::SetStage(IStage::State stage) {
        m_impl->m_stage = stage;
    }

    void Request::SetCode(ErrorCode code) {
        m_impl->m_code = code;
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
    
    IStage::State Request::GetStage() const noexcept {
        return m_impl->m_stage;
    }
    
    ErrorCode Request::GetCode() const noexcept {
        return m_impl->m_code;
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