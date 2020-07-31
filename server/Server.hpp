#pragma once
#include <memory>
#include <string>
#include <optional>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Log.hpp"

class Session;

class Server final {
public:
    
    Server(std::shared_ptr<asio::io_context>, std::uint16_t port);

    void Start();

    void Shutdown();
    
private:
    friend class Session;

    void Broadcast(const std::string& text);

    void RemoveSession(const Session * s);
    
    void BroadcastEveryoneExcept(const std::string& text, std::shared_ptr<const Session>);

    template<class ...Args>
    void Write(const LogType ty, Args&& ...args) {
        m_logger.Write(ty, std::forward<Args>(args)...);
    }
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

    Log m_logger{"server_log.txt"};
};

