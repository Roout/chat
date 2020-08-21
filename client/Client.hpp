#pragma once
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Message.hpp"
#include "Log.hpp"
#include "GUI.hpp"

namespace asio = boost::asio;

class Client final {
public:
    
    Client(std::shared_ptr<asio::io_context> io);

    ~Client();

    void Connect(const std::string& path, std::uint16_t port);
    
    void Write(std::string && text );

    bool IsWaitingAck() const noexcept {
        return m_state == State::WAIT_ACK;
    }

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
     * Completion handle. 
     * Called when the client read something from the remote endpoint
     */
    void OnRead(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    /**
     * Completion handle; called when the client write something to the remote endpoint
     */
    void OnWrite(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    /**
     * This function handle incoming messages
     */
    void HandleMessage(Internal::Message&);

    void SendSynRequest();

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
         * has already recieved ACK from the 
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
