#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>   // std::shared_ptr
#include <string>   
#include <optional>
#include <tuple>
#include <mutex>
#include <unordered_map>
#include <boost/asio.hpp>
#include "DoubleBuffer.hpp"
#include "Chatroom.hpp"
#include "Log.hpp"

class Session;

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
     * Get list of all available chatrooms user can join.
     * @return 
     *  Return the list of serialized chatrooms in json format.
     * @note
     *  Thread-safety: safe
     */
    std::vector<std::string> GetChatroomList() const noexcept;

    /**
     * Get chatroom information by it's ID.
     * @param id
     *  This is ID of the chatroom being looked up.
     * @return 
     *  Optional type. It wraps either nothing (can't find chatroom)
     * either tuple:
     *  - chatroom id;
     *  - session count, i.e. number of users;
     *  - chatroom name.
     * @note
     *  Thread-safety: safe
     */
    auto GetChatroomData(std::size_t id) const noexcept 
        -> std::optional<std::tuple<std::size_t, std::size_t, std::string>>
    {
        std::lock_guard<std::mutex> lock (m_mutex);
        if( auto it = m_chatrooms.find(id); it != m_chatrooms.end() ) {
            auto tuple = std::make_tuple(
                it->second.GetId(), 
                it->second.GetSessionCount(), 
                it->second.GetName()
            );
            return std::make_optional(tuple);
        }
        return std::nullopt;
    }

    /**
     * Create chatroom with the given name
     * @param name
     *  This is the name of the future chat room. It may not be unique.
     * @return 
     *  Return ID of the created chat room on success, 
     *  0 otherwise 
     * @note
     *  Thread-safety: safe
     */
    std::size_t CreateChatroom(std::string name);

    /// Methods
private:
    friend class Session;

    /**
     * Move session from hall chatroom to the chatroom with required id. 
     * @param chatroomId
     *  This is ID of the chatroom needed to be joined.
     * @param session
     *  This is session that is going to join to the chatroom
     * @return 
     *  The indication whether the session has joined chatroom successfully or not.
     *
     * @note 
     *  Session can be only in the one chatroom at the same time
     *  Thread-safety: safe
     */
    bool AssignChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session);

    /**
     * Move session from the current chatroom to the hall chatroom 
     * @param chatroomId
     *  This is ID of the chatroom needed to be left.
     * @param session
     *  This is session that is going to leave to the chatroom
     * @note 
     *  Session can be only in the one chatroom at the same time.
     *  Thread-safety: safe
     */
    void LeaveChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session);
    
    /**
     * Get ID of the chatroom this session belong to.
     * @param session
     *  This is session needed to be checked for joined chatroom.
     * @return 
     *  The ID of the joined chatroom. If it belongs to the HALL room,
     * returns chat::Chatroom::NO_ROOM constant
     * @note
     *  Thread-safety : safe
     */
    std::size_t GetChatroom(const Session*const session) const noexcept;

    /**
     * Check whether a chatroom with the given ID exist.
     * @param chatroomId
     *  The ID of the room needed to be checked.
     * @return 
     *  The indication whether the room with the given ID exist or not.
     * @note
     *  Thread-safety : safe
     */
    bool ExistChatroom(std::size_t chatroomId) const noexcept;

    /**
     * Remove chatroom from the chatroom pool.
     * @param chatroomId
     *  This is ID of the chatroom to be removed.
     * @param mustClose
     *  This is indication whether all active sessions (users) must be closed or not.
     * @note
     *  Thread-safety: NOT-safe
     */   
    void RemoveChatroom(std::size_t chatroomId, bool mustClose);

    /**
     * Check whether a chatroom with the given ID is empty (without users).
     * @param chatroomId
     *  The ID of the room needed to be checked.
     * @return 
     *  The indication whether the room with the given ID empty or not.
     * @note
     *  Thread-safety: NOT-safe
     */
    bool IsEmpty(std::size_t chatroomId) const noexcept;

    /**
     * Logging the custom message
     */
    template<class ...Args>
    void Write(const LogType ty, Args&& ...args);

    /// Properties
private:  

    /**
     * Protects resources from the data race 
     */
    mutable std::mutex m_mutex;

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
    
    /**
     * Virtual chatroom which is just a hall to keep all connections
     * which hasn't joined any chatroom yet.
     * No chat interface provided in this room (it's virtual, i.e. container). 
     */
    chat::Chatroom m_hall;

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