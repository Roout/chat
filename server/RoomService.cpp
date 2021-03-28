#include "RoomService.hpp"

#include "Session.hpp"

namespace chat {

RoomService::RoomService() :
    m_hall { std::make_shared<Chatroom>( "Hall" ) }
{
    m_chatrooms.reserve(100);
}

void RoomService::Close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, room]: m_chatrooms) {
        room->Close();
    }
    m_chatrooms.clear();
    if (!m_hall->IsEmpty()) {
        m_hall->Close();
    }
}

RoomService::~RoomService() {
    this->Close();
}

bool RoomService::AddSession(const std::shared_ptr<Session>& session) noexcept {
    return m_hall->AddSession(session);
}

bool RoomService::AssignChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session) {
    // Remove session from the hall
    const auto isRemoved = m_hall->RemoveSession(session.get());
    if (!isRemoved) {
        // TODO: can't find session in chatroom for unAuth
    }

    std::shared_ptr<Chatroom> room { nullptr };
    { // Block while looking for the desired chatroom
        std::lock_guard<std::mutex> lock(m_mutex);
        // Find chatroom with required id
        // If chatroom is found then try to assign session to chatroom
        if (const auto it = m_chatrooms.find(chatroomId); it != m_chatrooms.end()) {
            room = it->second;
        }
    } // Pointer to chatroom was aquired so release mutex

    // if chatroom was assigned successfully return true otherwise false
    if (room && room->AddSession(session)) {
        return true;
    } 
    else { // failed to join chatroom or room doens't exist
        // assign session back to hall
        (void) m_hall->AddSession(session);
        return false;
    }
}

void RoomService::BroadcastOnly(
    const Session* source, 
    const std::string& message, 
    std::function<bool(const Session&)>&& condition
) {
    const auto id = source->GetUser().m_chatroom;

    std::shared_ptr<Chatroom> room { nullptr };
    { // block until the room was found
        std::lock_guard<std::mutex> lock{ m_mutex };
        if (auto it = m_chatrooms.find(id); it != m_chatrooms.end()) {
            room = it->second;
        }
    } // release mutex
    room->Broadcast(message, condition);
}

void RoomService::LeaveChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session) {
    // Check whether it's a hall chatroom
    if (chatroomId == m_hall->GetId()) {
        /// TODO: user can't leave hall!
        return;
    }     
    std::shared_ptr<Chatroom> room{ nullptr };
    { // Block
        std::lock_guard<std::mutex> lock{ m_mutex };
        if (auto it = m_chatrooms.find(chatroomId); it != m_chatrooms.end()) {
            room = it->second;
        }
    } // Release

    if (room && room->RemoveSession(session.get())) {
        if (room->IsEmpty()) {
            this->RemoveChatroom(chatroomId);
        } 
        (void) m_hall->AddSession(session);
    }
}

std::vector<std::string> RoomService::GetChatroomList() const noexcept {
    std::vector<std::string> list;
    list.reserve(m_chatrooms.size());

    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    for (const auto& [id, chatroom] : m_chatrooms) {
        list.emplace_back(chatroom->AsJSON());
    }
    lock.unlock();
    return list;
}

std::size_t RoomService::CreateChatroom(std::string name) {
    auto room { std::make_shared<Chatroom>(name) };
    const std::size_t id { room->GetId() };
    { // Block
        std::lock_guard<std::mutex> lock(m_mutex);
        m_chatrooms.emplace(id, std::move(room));
    } // Release
    return id;
}

std::size_t RoomService::GetChatroom(const Session* const session) const noexcept {
    // check the hall
    if (m_hall->Contains(session)) {
        return m_hall->GetId();
    }
    // check user's chatrooms
    { // Blocks
        std::lock_guard<std::mutex> lock{ m_mutex };
        for (const auto& [id, room]: m_chatrooms) {
            if (room->Contains(session)) {
                return id;
            }
        }
    } // Release
    return chat::Chatroom::NO_ROOM;
}

bool RoomService::ExistChatroom(std::size_t id) const noexcept {
    std::lock_guard<std::mutex> lock{ m_mutex };  
    return (m_chatrooms.find(id) != m_chatrooms.cend());
}

void RoomService::RemoveChatroom(std::size_t chatroomId) {
    std::shared_ptr<Chatroom> room { nullptr };
    { // Block
        std::lock_guard<std::mutex> lock{ m_mutex };  
        if (auto it = m_chatrooms.find(chatroomId); it != m_chatrooms.end()) {
            room = it->second;
            m_chatrooms.erase(it);
        }
    } // Release
    if (room) {
        room->Close();
    }
}

bool RoomService::IsEmpty(std::size_t chatroomId) const noexcept {
    std::lock_guard<std::mutex> lock{ m_mutex };  
    const auto it = m_chatrooms.find(chatroomId);
    return (it == m_chatrooms.cend() || it->second->IsEmpty());
}

} // namespace chat;