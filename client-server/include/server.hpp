﻿#pragma once
#include <memory>
#include <string>
#include <optional>
#include <queue>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"

namespace asio = boost::asio;

class Server;

/// TODO: Prevent several async_write/read in/from one direction!
///
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
     * Write @text to remote connection 
     */
    void Write(std::string text);

    /**
     * Read @text from the remote connection
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
private:

    /**
     * It's a socket connected to the server. 
     */
    asio::ip::tcp::socket m_socket;

    /// TODO: remove server pointer. 
    /// Session should communicate with server via protocol and preefined commands
    Server * const m_server { nullptr };

    asio::io_context::strand m_strand;

    Buffers m_outbox;

    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_inbox;

    bool m_isClosed { false };

    bool m_isWriting { false };


};

class Server final {
public:
    
    Server(std::shared_ptr<asio::io_context>, std::uint16_t port);

    void Start();

    void Shutdown();

    void Broadcast(const std::string& text);
    
    void BroadcastEveryoneExcept(const std::string& text, std::shared_ptr<const Session>);

    void RemoveSession(const Session * s);

private:
    friend class Session;
    /**
     * Context is being passed by a pointer because we don't need to know
     * where and how it's being run. We just ditch this responsibility to someone else.
     */
    std::shared_ptr<asio::io_context>       m_context;
    asio::ip::tcp::acceptor                 m_acceptor;
    /**
     * Socket  
     */
    std::optional<asio::ip::tcp::socket>    m_socket;
    /**
     * Use shared_ptr instead of simple instance because 
     * Session is derived from std::enamble_shared_from_this so
     * we need to have allocated control block for ::shared_from_this
     */
    std::vector<std::shared_ptr<Session>>   m_sessions;
};

