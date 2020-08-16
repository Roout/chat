#pragma once
#include <memory>
#include <string>
#include <optional>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Chatroom.hpp"
#include "Log.hpp"

class Session;

class Server final {
public:
    
    Server(std::shared_ptr<asio::io_context>, std::uint16_t port);

    void Start();

    void Shutdown();
    
    std::vector<std::string> GetChatroomList() const noexcept;

    size_t CreateChatroom(std::string name);
    
private:
    friend class Session;

    /**
     * Move session from chatroom for unauthorized users to chatroom with required id. 
     */
    bool AssignChatroom(size_t chatroomId, const std::shared_ptr<Session>& session);

    void LeaveChatroom(size_t chatroomId, const std::shared_ptr<Session>& session);

    size_t GetChatroom(const Session*const session) const noexcept;

    bool ExistChatroom(size_t id) const noexcept;

    template<class ...Args>
    void Write(const LogType ty, Args&& ...args);

    /// data mambers:
private:  
    Log m_logger{"server_log.txt"};
    /**
     * Context is being passed by a pointer because we don't need to know
     * where and how it's being run. We just ditch this responsibility to someone else.
     */
    std::shared_ptr<asio::io_context>       m_context;
    
    asio::ip::tcp::acceptor                 m_acceptor;

    std::optional<asio::ip::tcp::socket>    m_socket;
    
    chat::Chatroom                          m_hall;

    std::vector<chat::Chatroom>             m_chatrooms;

};

template<class ...Args>
void Server::Write(const LogType ty, Args&& ...args) {
    m_logger.Write(ty, std::forward<Args>(args)...);
}