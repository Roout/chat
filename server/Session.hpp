#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"

namespace asio = boost::asio;

class Server;
namespace Requests {
    class Request;
}

class Session final : public std::enable_shared_from_this<Session> {
public:

    Session( 
        asio::ip::tcp::socket && socket, 
        Server * const server 
    );

    ~Session() {
        this->Close();
    };

    /**
     * Write @text to remote connection.
     * Invoke private Write() overload via asio::post() through strand
     */
    void Write(std::string text);

    /**
     * Read data from the remote connection.
     * At first invoked on accept completion handler and then only
     * on read completion handler. This prevent concurrent execution of read
     * operation on socket.
     */
    void Read();

    bool IsClosed() const noexcept {
        return m_isClosed;
    }
    /**
     * Shutdown Session and close the socket  
     */
    void Close();

private:
    
    void WriteSomeHandler(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );

    void ReadSomeHandler(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );
    
    void Write();
    
    /**
     * Build reply base on incoming request
     * 
     * @param request
     *      This is request which comes from the remote peer. 
     */
    std::string SolveRequest(const Requests::Request& request);

    /// data members
private:
    enum class State {
        /**
         * In this state session will only wait for AUTHORIZE request.
         * Any other requests will be ignored.
         */
        ACCEPTED,
        /**
         * In this state session expects any request except CHAT_MESSAGE and AUTHORIZE.
         * The user is already authorized and have access to several functions.
         */
        AUTHORIZED,
        /**
         *  In this state user is busy chatting in the chatroom
         */
        BUSY
    };

    /**
     * It's a socket connected to the server. 
     */
    asio::ip::tcp::socket m_socket;

    /// Session should communicate with server via protocol and predefined commands
    Server * const m_server { nullptr };

    asio::io_context::strand m_strand;

    Buffers m_outbox;

    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_inbox;

    bool m_isClosed { false };

    bool m_isWriting { false };

    State m_state { State::ACCEPTED };


};

#endif // SESSION_HPP