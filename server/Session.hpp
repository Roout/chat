#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <cstddef>
#include <functional>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Message.hpp"
#include "User.hpp"

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

    void WaitSynchronizationRequest();

    /**
     * Write @text to remote connection.
     * @note
     *  Invoke private Write() overload via asio::post() through strand
     */
    void Write(std::string text);

    const Internal::User& GetUser() const noexcept {
        return m_user;
    }

    bool IsClosed() const noexcept {
        return m_state == State::DISCONNECTED;
    }
    
    bool IsWaitingSyn() const noexcept {
        return m_state == State::WAIT_SYNCHRONIZE_REQUEST;
    }

    bool IsAcknowleged() const noexcept {
        return m_state == State::SENT_ACKNOWLEDGED_RESPONSE;
    }
    
    /**
     * Shutdown Session and close the socket  
     */
    void Close();
    
private:

    bool AssignChatroom(std::size_t id);

    bool LeaveChatroom();
    
    
    /**
     * Read data from the remote connection.
     * At first it's invoked at server's `on accept` completion handler.
     * Otherwise it can be invoked within `on read` completion handler. 
     * This prevents a concurrent execution of the read operation on the same socket.
     */
    void Read();
    
    /**
     * This method calls async I/O write operation.
     */
    void Write();

    void WriteSomeHandler(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    void ReadSomeHandler(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    void ExpiredDeadlineHandler(
        const boost::system::error_code& error
    );

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
     * Handle incoming chat message
     * 
     * @param chat
     *  This is chat message which came from the remote peer. 
     */
    void HandleChat(const Internal::Chat* chat);

    /// Minor Helper functions

    /**
     * This is a method which builds response only for incoming SYNCHRONIZATION requests. 
     * @param request
     *  This is incoming (from the remote peer) request: first step of the handshake.
     * @return 
     *  Return respose base on processed request. If everything is fine, 
     *  respose will have an ACKNOWLEDGEMENT.
     */
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
     * It's a socket connected to the remote peer. 
     */
    asio::ip::tcp::socket m_socket;

    Server * const m_server { nullptr };

    /**
     * It's a user assosiated with remote connection 
     */
    Internal::User m_user;

    asio::io_context::strand m_strand;

    Buffers m_outbox;

    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_inbox;

    /**
     * This is an indication whether the socket writing operation 
     * is ongoing or not. 
     */
    bool m_isWriting { false };

    /**
     * This is indication of the current state of this session,
     * i.e. stage of communication.  
     */
    State m_state { State::WAIT_SYNCHRONIZE_REQUEST };

    /**
     * This is a timer used to set up deadline for the client
     * and invoke handler for the expired request.
     * Now it's used to wait for the synchronization request.  
     */
    asio::deadline_timer m_timer;

    /**
     * Define hom long the session can wait for the SYN request 
     * from the client. Time is in milliseconds.
     */
    std::size_t m_waitSynTimeout { 128 };
};


#endif // SESSION_HPP