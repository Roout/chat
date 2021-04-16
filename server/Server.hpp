#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>   // std::shared_ptr
#include <optional>
#include <cstdint>

#include <boost/asio.hpp>

#include "Log.hpp"

namespace asio = boost::asio;

namespace chat {
    class RoomService;
}

class Server final {
public:
    
    /**
     * Server's constructor with external io context;
     * @param io
     *  Pointer to external io_context instance.
     * @param port
     *  This is a port the server will listen to.
     */
    Server(std::shared_ptr<asio::io_context> io, std::uint16_t port);

    /**
     * Start accepting connections on the given port
     */
    void Start();

    /**
     * Shutdown the server closing all connections beforehand 
     */
    void Shutdown();

    /**
     * Used for testing 
     */
    std::shared_ptr<chat::RoomService> GetRoomService() const;

private:

    /**
     * Logging the custom message
     */
    template<class ...Args>
    void Write(const LogType ty, Args&& ...args);

    /// Properties

    /**
     * Log all server activities for debug purpose.
     */
    Log m_logger{"server_log.txt"};

    /**
     * Context is being passed by a pointer because we don't need to know
     * where and how it's being run. We just ditch this responsibility to someone else.
     */
    std::shared_ptr<asio::io_context> m_context;
    
    asio::ip::tcp::acceptor m_acceptor;

    /**
     * Optionality gives us a mechanism to create socket inplace after moving it 
     * to accepter's handler.  
     */
    std::optional<asio::ip::tcp::socket> m_socket;

    std::shared_ptr<chat::RoomService> m_service { nullptr };
};

template<class ...Args>
void Server::Write(const LogType ty, Args&& ...args) {
    m_logger.Write(ty, std::forward<Args>(args)...);
}

inline std::shared_ptr<chat::RoomService> Server::GetRoomService() const {
    return m_service;
}

#endif // SERVER_HPP