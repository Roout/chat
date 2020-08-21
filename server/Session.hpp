#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "User.hpp"
#include "Message.hpp"
#include <cstddef>

namespace asio = boost::asio;

class Server;
namespace chat {
    class Chatroom;
}

class Session final : public std::enable_shared_from_this<Session> {
public:

    Session( 
        asio::ip::tcp::socket && socket, 
        Server * const server 
    );

    ~Session() {
        if( m_state != State::DISCONNECTED ) {
            this->Close();
        }
    };

    /**
     * Write @text to remote connection.
     * Invoke private Write() overload via asio::post() through strand
     */
    void Write(std::string text);

    /**
     * Read data from the remote connection.
     * At first invoked on accept completion handler and then only
     * on read completion handler. This prevent concurrent execution 
     * of read operation on the same socket.
     */
    void Read();

    bool IsClosed() const noexcept {
        return m_state == State::DISCONNECTED;
    }
    
    const Internal::User& GetUser() const noexcept {
        return m_user;
    }

    bool IsWaitingSyn() const noexcept{
        return m_state == State::WAIT_SYNCHRONIZE_REQUEST;
    }

    bool IsAcknowleged() const noexcept {
        return m_state == State::SENT_ACKNOWLEDGED_RESPONSE;
    }
    /**
     * Shutdown Session and close the socket  
     */
    void Close();

    bool AssignChatroom(std::size_t id);

    bool LeaveChatroom();
    
private:
    
    void WriteSomeHandler(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    void ReadSomeHandler(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );
    
    void Write();

    /**
     * Process incoming message base on protocol.
     * @param msg
     *  This is incoming message.
     */
    void HandleMessage(const Internal::Message& msg);
    
    /**
     * Build reply base on incoming request
     * 
     * @param request
     *  This is request which came from the remote peer. 
     * @return 
     *  Return serialized reply(response format) build base on @request
     */
    std::string HandleRequest(const Internal::Request* request);

    /**
     * Build reply base on incoming request
     * 
     * @param chat
     *  This is chat message which came from the remote peer. 
     * @return 
     *  Return serialized reply(chat format) build base on @chat
     */
    void HandleChat(const Internal::Chat* chat);

    /// Minor Helper functions
    Internal::Response HandleSyncRequest(const Internal::Request& request);
    
    /// Properties
private:
    enum class State {
        /**
         * This is a state when a session was just connected
         * and initiate deadline timer waiting for the SYN 
         * request. 
         */
        WAIT_SYNCHRONIZE_REQUEST,
        /**
         * This is a state when a session has already recieved
         * the SYN request, no problems have been occured
         * with the request and ACK response was sent. 
         */
        SENT_ACKNOWLEDGED_RESPONSE,
        /**
         * This is a state when a connection between peers 
         * was terminated/closed or hasn't even started. 
         * Reason isn't important.
         */
        DISCONNECTED
    };

    /**
     * It's a socket connected to the server. 
     */
    asio::ip::tcp::socket m_socket;

    Server * const m_server { nullptr };

    Internal::User m_user;

    asio::io_context::strand m_strand;

    Buffers m_outbox;

    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_inbox;

    bool m_isWriting { false };

    State m_state { State::WAIT_SYNCHRONIZE_REQUEST };

    /// TODO: add deadline timer to keep track for timeouts
};


#endif // SESSION_HPP