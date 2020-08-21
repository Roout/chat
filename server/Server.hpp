#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Chatroom.hpp"
#include "Log.hpp"

class Session;

class Server final {
public:
    
    Server(std::shared_ptr<asio::io_context>, std::uint16_t port);

    /**
     * Start accepting connections on the given port
     */
    void Start();

    /**
     * Shutdown the server closing all connections beforehand 
     */
    void Shutdown();
    
    /**
     * Get list of all available chatrooms user can join.
     * @return 
     *  Return std::vector of std::string which is 
     *  std::vector of serialized json object.
     */
    std::vector<std::string> GetChatroomList() const noexcept;

    const chat::Chatroom * GetChatroom(std::size_t id) const noexcept {
        if( auto it = m_chatrooms.find(id); it != m_chatrooms.end() ) {
            return &it->second;
        }
        return nullptr;
    }

    /**
     * Create chatroom with the given name
     * @param name
     *  This is the name of the future chat room. It may not be unique.
     * @return 
     *  Return ID of the created chat room on success, 
     *  0 otherwise 
     */
    std::size_t CreateChatroom(std::string name);

    /// Methods
private:
    friend class Session;

    /**
     * Move session from chatroom for unauthorized users to chatroom with required id. 
     */
    bool AssignChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session);

    void LeaveChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session);
        
    void RemoveChatroom(std::size_t chatroomId, bool mustClose);
    
    std::size_t GetChatroom(const Session*const session) const noexcept;

    bool ExistChatroom(std::size_t chatroomId) const noexcept;

    bool IsEmpty(std::size_t chatroomId) const noexcept;

    template<class ...Args>
    void Write(const LogType ty, Args&& ...args);

    /// Properties
private:  
    /// TODO: add mutex as there are resourses which will be used by several threads.

    /**
     * Log all server activities for debug purpose 
     */
    Log m_logger{"server_log.txt"};
    /**
     * Context is being passed by a pointer because we don't need to know
     * where and how it's being run. We just ditch this responsibility to someone else.
     */
    std::shared_ptr<asio::io_context>       m_context;
    
    asio::ip::tcp::acceptor                 m_acceptor;

    std::optional<asio::ip::tcp::socket>    m_socket;
    
    /**
     * Virtual chatroom which is just a hall to keep all connections
     * which hasn't joined any chatroom yet.
     * No chat interface provided in this room (it's virtual, i.e. container). 
     */
    chat::Chatroom                          m_hall;

    /**
     * Keep active chatrooms which can be accessed by their ID as key. 
     */
    std::unordered_map<std::size_t, chat::Chatroom> m_chatrooms;

};

template<class ...Args>
void Server::Write(const LogType ty, Args&& ...args) {
    m_logger.Write(ty, std::forward<Args>(args)...);
}

#endif // SERVER_HPP