#ifndef CHAT_HALL_HPP
#define CHAT_HALL_HPP

#include <mutex>
#include <memory>
#include <unordered_map>
#include <optional>
#include <tuple>
#include <string>
#include <vector>
#include <cstddef>

#include "Log.hpp"
#include "Chatroom.hpp"

class Session;

namespace chat {

class RoomService final {
public:
    
    RoomService();

    ~RoomService();

    void Close();

    bool AddSession(const std::shared_ptr<Session>& session) noexcept;

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
        if(auto it = m_chatrooms.find(id); it != m_chatrooms.end()) {
            auto room = it->second;
            auto tuple = std::make_tuple(
                room->GetId(), 
                room->GetSessionCount(), 
                room->GetName()
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
     *  Thread-safety: safe
     */
    std::size_t GetChatroom(const Session* const session) const noexcept;

    /**
     * Conditional broadcast message to users in the chatroom which @source belongs.
     * 
     * @param source
     *  Session which initiate a broadcast in chatroom it belongs
     * @param message
     *  Message which will be broadcasted
     * @param condition
     *  Conditional function which on execution return indication
     * whether a message will be sent to the given user on not.
     */
    void BroadcastOnly(
        const Session* source, 
        const std::string& message, 
        std::function<bool(const Session&)>&& condition
    );

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
     * @note
     *  Thread-safety: NOT-safe
     */   
    void RemoveChatroom(std::size_t chatroomId);

    /**
     * Check whether a chatroom with the given ID is empty (without users).
     * @param chatroomId
     *  The ID of the room needed to be checked.
     * @return 
     *  The indication whether the room with the given ID empty or not.
     * @note
     *  Thread-safety: safe
     */
    bool IsEmpty(std::size_t chatroomId) const noexcept;

private:

    mutable std::mutex m_mutex;

    /**
     * Log all hall activities for debug purpose.
     */
    Log m_logger{"hall_log.txt"};

    /**
     * Keep active chatrooms which can be accessed by their ID as key. 
     */
    std::unordered_map<std::size_t, std::shared_ptr<chat::Chatroom>> m_chatrooms;

    /**
     * Virtual chatroom which is just a hall to keep all connections
     * which hasn't joined any chatroom yet.
     * No chat interface provided in this room (it's virtual, i.e. container). 
     */
    std::shared_ptr<chat::Chatroom> m_hall { nullptr };
};

} // namespace chat

#endif // CHAT_HALL_HPP