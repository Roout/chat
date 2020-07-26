#pragma once
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "InteractionStage.hpp"

namespace asio = boost::asio;

class Client final {
public:
    
    Client(std::shared_ptr<asio::io_context> io);

    ~Client();

    void Connect(const std::string& path, std::uint16_t port);
    
    void Write(std::string && text );

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
        size_t transferredBytes
    );

    /**
     * Completion handle; called when the client write something to the remote endpoint
     */
    void OnWrite(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );

private:
    
    std::shared_ptr<asio::io_context>   m_io;
    asio::io_context::strand            m_strand;
    asio::ip::tcp::socket               m_socket;
    
    bool            m_isWriting { false };
    std::string     m_inbox;
    Buffers         m_outbox;
};