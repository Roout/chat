#pragma once
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "InteractionStage.hpp"
#include "Log.hpp"
#include "GUI.hpp"

namespace asio = boost::asio;

namespace Requests{
    class Request;
}

class Client final {
public:
    
    Client(std::shared_ptr<asio::io_context> io);

    ~Client();

    void Connect(const std::string& path, std::uint16_t port);
    
    void Write(std::string && text );

    /**
     * This function indicate stage which interaction between client and 
     * server has already reached. 
     */
    IStage::State GetStage() const noexcept;

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
        size_t transferredBytes
    );

    /**
     * Completion handle; called when the client write something to the remote endpoint
     */
    void OnWrite(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );

    /**
     * This function handle incoming requests and 
     * base on it's type, status and interaction stage 
     * changes own state. 
     */
    void HandleRequest(Requests::Request&&);

private:
    
    Log             m_logger { "client_log.txt" };
    
    std::shared_ptr<asio::io_context>   m_io;
    asio::io_context::strand            m_strand;
    asio::ip::tcp::socket               m_socket;
    
    bool            m_isClosed { false };
    bool            m_isWriting { false };
    asio::streambuf m_inbox;
    Buffers         m_outbox;
    IStage::State   m_stage { IStage::State::DISCONNECTED };

    GUI             m_gui;
};

inline const GUI& Client::GetGUI() const noexcept {
    return m_gui;
}
