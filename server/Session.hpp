#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "InteractionStage.hpp"

namespace asio = boost::asio;

class Server;
namespace chat {
    class Chatroom;
}
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
        if( !m_isClosed ) 
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

    bool AssignChatroom(size_t id);

    bool LeaveChatroom();
    
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

    /**
     * The authorization must firstly be verified. 
     */
    [[nodiscard]] bool ValidateAuth(const Requests::Request& request) const noexcept;

    /// data members
private:
    /**
     * It's a socket connected to the server. 
     */
    asio::ip::tcp::socket m_socket;

    Server * const m_server { nullptr };

    size_t m_chatroom;

    asio::io_context::strand m_strand;

    Buffers m_outbox;

    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_inbox;

    bool m_isClosed { false };

    bool m_isWriting { false };

    IStage::State m_state { IStage::State::UNAUTHORIZED };

};

#endif // SESSION_HPP