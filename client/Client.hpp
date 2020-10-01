#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Message.hpp"
#include "Log.hpp"
#include "GUI.hpp"

namespace asio = boost::asio;

class Client final : public std::enable_shared_from_this<Client> {
public:
    
    /**
     * Create an instance with external I/O context 
     */
    Client(std::shared_ptr<asio::io_context> io);

    /**
     * Close connection if it's haven't been terminated yet. 
     */
    ~Client();

    /**
     * Trying to connect to the remote host
     * 
     * @param path
     *  It's ip address of the remote host
     * @param port
     *  It's a port on which a remote host is listening  
     */
    void Connect(const std::string& path, std::uint16_t port);
    
    /**
     * Write message to the remote peer
     * @param text
     *  Message in string format 
     */
    void Write(std::string && text );

    /**
     * Check whether a client waiting for acknowledge response or not.
     * @return 
     *  The indication that client is waiting for acknowledge response from the server.  
     */
    bool IsWaitingAck() const noexcept {
        return m_state == State::WAIT_ACK;
    }

    /**
     * Check whether a client is already acknoledged or not.
     * @return 
     *  The indication that client has already received an acknowledgement from the server.  
     */
    bool IsAcknowleged() const noexcept {
        return m_state == State::RECIEVE_ACK;
    }

    inline const GUI& GetGUI() const noexcept; 
    
private:

    void Close();

    void OnConnect(const boost::system::error_code& err);

    void Read();
    
    /**
     * Send everything from the passive buffers to remote peer.  
     * It is called from the function wrapped in strand to prevent concurrent 
     * access to outgoing buffer and other variables. 
     */
    void Write();

    /**
     * Completion handler. 
     * Called when the client read something from the remote endpoint
     */
    void OnRead(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    /**
     * Completion handler; called when the client write something to the remote endpoint
     */
    void OnWrite(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    /**
     * This function handle incoming response
     */
    void HandleMessage(Internal::Response&);

    /**
     * This method initialite handshake with server `on connection` event.
     * I.e. it sends a request with SYN query type to server.
     */
    void Synchronize();

private:
    enum class State {
        /**
         * This state indicates that 
         * client is closed or disconnected.  
         */
        CLOSED,
        /**
         * This state indicated that 
         * client has sent SYN handshake to the 
         * server and is waiting ACK response.
         */
        WAIT_ACK,
        /**
         * This state indicates that the client
         * has already received ACK from the 
         * server and server was acknowledged
         * by the client.
         */
        RECIEVE_ACK
    };

    Log             m_logger { "client_log.txt" };
    
    std::shared_ptr<asio::io_context>   m_io;
    asio::io_context::strand            m_strand;
    asio::ip::tcp::socket               m_socket;
    
    bool            m_isWriting { false };
    asio::streambuf m_inbox;
    Buffers         m_outbox;
    State           m_state { State::CLOSED };

    GUI             m_gui;
};

inline const GUI& Client::GetGUI() const noexcept {
    return m_gui;
}

#endif // CHAT_CLIENT_HPP