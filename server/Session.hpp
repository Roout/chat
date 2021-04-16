#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <memory>
#include <cstddef>
#include <functional>

#include <boost/asio.hpp>

#include "DoubleBuffer.hpp"
#include "Message.hpp"
#include "User.hpp"
#include "Log.hpp"

namespace asio = boost::asio;

namespace chat {
    class Chatroom;
    class RoomService;
}
namespace net {
    class Connection;
}
namespace rt {
    class RequestQueue;
}

// TODO: try to remove the inheritance
class Session final : public std::enable_shared_from_this<Session> {
public:

    Session( 
        asio::ip::tcp::socket && socket, 
        std::shared_ptr<chat::RoomService> service,
        std::shared_ptr<asio::io_context> context
    );

    ~Session() {
        if (m_connection != nullptr) {
            this->Close();
        }
    };

    /**
     * Initiate read operation for the connection 
     */
    void Read();

    /**
     * queue text for writing through connection
     */
    void Write(std::string text);

    /**
     * Close connection  
     */
    void Close();

    void Subscribe();

    /**
     * Initiate a handler which
     * acquire all already existing in queue requests
     * and proccessed them one by one
     */
    void AcquireRequests();

    const Internal::User& GetUser() const noexcept {
        return m_user;
    }

    bool IsClosed() const noexcept {
        return m_state == State::CLOSED;
    }
    
    bool IsWaitingSyn() const noexcept {
        return m_state == State::WAIT_SYN;
    }

    bool IsAcknowleged() const noexcept {
        return m_state == State::ACKNOWLEDGED;
    }
    

    /// Interface used by response handlers
    void AcknowledgeClient() noexcept {
        m_state = State::ACKNOWLEDGED;
    }

    void UpdateUsername(std::string name) {
        m_user.m_username = std::move(name);
    }

    bool AssignChatroom(std::size_t id);

    bool LeaveChatroom();

    std::size_t CreateChatroom(const std::string& chatroomName);

    std::vector<std::string> GetChatroomList() const noexcept;

    void BroadcastOnly(
        const std::string& message, 
        std::function<bool(const Session&)>&& condition
    );
    
private:
    enum class State: std::uint8_t {
        /**
         * This is a state when a session was just connectied
         * and initiate deadline timer waiting for the SYN 
         * request. 
         */
        WAIT_SYN,
        /**
         * This is a state when a session has already received
         * the SYN request, no problems have been occured
         * with the request and ACK response was sent. 
         */
        ACKNOWLEDGED,
        CLOSED
    };

    /**
     * Build reply base on incoming request
     * 
     * @param request
     *  This is request which came from the remote peer. 
     */
    void HandleRequest(Internal::Request&& request);

    /// Properties
private:
    /**
     * It's a user assosiated with remote connection 
     */
    Internal::User m_user {};
    
    std::shared_ptr<chat::RoomService> m_service { nullptr };

    std::shared_ptr<rt::RequestQueue> m_incommingRequests { nullptr };

    std::shared_ptr<asio::io_context> m_context { nullptr };

    std::shared_ptr<net::Connection> m_connection { nullptr };

    State m_state { State::CLOSED };

    /**
     * Time in milliseconds the session is ready to wait for the SYN request 
     */
    const std::size_t m_timeout { 256 };
};


#endif // SESSION_HPP