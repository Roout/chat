#include "Server.hpp"
#include "Session.hpp"

Server::Server(std::shared_ptr<asio::io_context> context, std::uint16_t port) :
    m_context { context },
    m_acceptor { *m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port) },
    m_hall { "unautherized users" }
{
    m_chatrooms.reserve(40);
}

void Server::Start() {
    m_socket.emplace(*m_context);
    m_acceptor.set_option(
        asio::ip::tcp::acceptor::reuse_address(false)
    );

    /// Q: Am I right to not use strand here? 
    /// A: As far as I can see I need strand here for ostream object safety...
    m_acceptor.async_accept( *m_socket, [&](const boost::system::error_code& code ) {
        if( !code ) {
            boost::system::error_code err; 
            this->Write(LogType::info, 
                "Server accepted connection on endpoint: ", m_socket->remote_endpoint(err), "\n"
            );
            
            const auto newSession { std::make_shared<Session>(std::move(*m_socket), this) };
            if(!m_hall.AddSession(newSession)) {
                // TODO: failed to add new session most likely due to connection limit 
            }
            newSession->Read();
            // wait for the new connections again
            this->Start();
        }
    });
}

void Server::Shutdown() {
    boost::system::error_code ec;
    m_acceptor.close(ec);
    if(ec) {
        this->Write(LogType::error, 
            "Server closed acceptor with error: ", ec.message(), "\n"
        );
    }

    m_hall.Close();
    for(auto& [id, chat]: m_chatrooms) chat.Close();
}

bool Server::AssignChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session) {
    const auto isRemoved = m_hall.RemoveSession(session.get());
    if(!isRemoved) {
        // TODO: can't find session in chatroom for unAuth
    }
    // find chatroom with required id
    const auto chat = m_chatrooms.find(chatroomId);
    // if chatroom is found then try to assign session to chatroom
    if( chat != m_chatrooms.end() ) {
        // if chatroom was assigned successfully return true otherwise false
        if( chat->second.AddSession(session) ) {
            return true;
        } 
        else { // failed to join chatroom, go back to hall
            const auto result = m_hall.AddSession(session);
            if( !result ) {
                // some weird error!
            }
        }
    }
    return false;
}

void Server::LeaveChatroom(std::size_t chatroomId, const std::shared_ptr<Session>& session) {
    // Check whether it's a hall chatroom
    if( chatroomId == m_hall.GetId()) {
        /// TODO: user can't leave hall!
        return;
    } 
    
    const auto chat = m_chatrooms.find(chatroomId);

    if( chat != m_chatrooms.end() ) {
        if( chat->second.RemoveSession(session.get()) ) {
            if(this->IsEmpty(chatroomId)) {
                this->RemoveChatroom(chatroomId, false);
            } 
            const auto isInHall = m_hall.AddSession(session);
            if( !isInHall ) {
                /// TODO: some weird error
            }
        }
    }
}

std::vector<std::string> Server::GetChatroomList() const noexcept {
    std::vector<std::string> list;
    list.reserve(m_chatrooms.size());
    for(auto& [id, chatroom] : m_chatrooms) {
        list.emplace_back(chatroom.AsJSON());
    }
    return list;
}

std::size_t Server::CreateChatroom(std::string name) {
    chat::Chatroom room { std::move(name) };
    const std::size_t id { room.GetId() };
    m_chatrooms.emplace(id, std::move(room));
    return id;
}

std::size_t Server::GetChatroom(const Session*const session) const noexcept {
    // check the hall
    if( m_hall.Contains(session) ) {
        return m_hall.GetId();
    }
    // check user's chatrooms
    for(const auto& [id, room]: m_chatrooms) {
        if( room.Contains(session) ) {
            return id;
        }
    }

    return chat::Chatroom::NO_ROOM;
}


bool Server::ExistChatroom(std::size_t id) const noexcept {
    return m_chatrooms.find(id) != m_chatrooms.cend();
}

void Server::RemoveChatroom(std::size_t chatroomId, bool mustClose) {
    const auto it = m_chatrooms.find(chatroomId);
    if( it != m_chatrooms.end() ) {
        if( mustClose ) it->second.Close();
        m_chatrooms.erase(it);
    }
}

bool Server::IsEmpty(std::size_t chatroomId) const noexcept {
    const auto it = m_chatrooms.find(chatroomId);
    return (it == m_chatrooms.cend() || it->second.IsEmpty());
}