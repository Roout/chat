#include "Server.hpp"
#include "Session.hpp"
#include "Request.hpp"
#include "InteractionStage.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

Server::Server(std::shared_ptr<asio::io_context> context, std::uint16_t port) :
    m_context { context },
    m_acceptor { *m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port) },
    m_hall { "unautherized users" }
{
    m_chatrooms.reserve(10);
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
            
            std::stringstream ss;
            ss << "Welcome to my server, user #" << m_socket->remote_endpoint(err) << '\n';
            ss << "Please complete authorization to gain some access.";
            std::string body = ss.rdbuf()->str();

            Requests::Request request {};
            request.SetType(Requests::RequestType::POST);
            request.SetStage(IStage::State::UNAUTHORIZED);
            request.SetCode(Requests::ErrorCode::SUCCESS);
            request.SetBody(body);

            auto newSession { std::make_shared<Session>(std::move(*m_socket), this) };
            if(!m_hall.AddSession(newSession)) {
                // TODO: failed to add new session most likely due to connection limit 
            }
            // welcome new user
            newSession->Write(request.Serialize());
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
    for(auto& chat: m_chatrooms) chat.Close();
}

bool Server::AssignChatroom(size_t chatroomId, const std::shared_ptr<Session>& session) {
    const auto isRemoved = m_hall.RemoveSession(session.get());
    if(!isRemoved) {
        // TODO: can't find session in chatroom for unAuth
    }
    // find chatroom with required id
    const auto chat = std::find_if(m_chatrooms.begin(), m_chatrooms.end(), [chatroomId](const auto& chatroom){
        return chatroom.GetId() == chatroomId;
    });
    // if chatroom is found then try to assign session to chatroom
    if( chat != m_chatrooms.end() ) {
        // if chatroom was assigned successfully return true otherwise false
        if( chat->AddSession(session) ) {
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

void Server::LeaveChatroom(size_t chatroomId, const std::shared_ptr<Session>& session) {
    // Check whether it's a hall chatroom
    if( chatroomId == m_hall.GetId()) {
        /// TODO: user can't leave hall!
        return;
    } 
    
    const auto chat = std::find_if(m_chatrooms.begin(), m_chatrooms.end(), [chatroomId](const auto& chatroom){
        return chatroom.GetId() == chatroomId;
    });

    if( chat != m_chatrooms.end() ) {
        if( chat->RemoveSession(session.get()) ) {
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
    for(auto& chatroom : m_chatrooms) {
        list.emplace_back(chatroom.GetRepresentation());
    }
    return list;
}

size_t Server::CreateChatroom(std::string name) {
    m_chatrooms.emplace_back(std::move(name));
    return m_chatrooms.back().GetId();
}

size_t Server::GetChatroom(const Session*const session) const noexcept {
    // check the hall
    if( m_hall.Contains(session) ) {
        return m_hall.GetId();
    }
    // check user's chatrooms
    for(const auto& room: m_chatrooms) {
        if( room.Contains(session) ) {
            return room.GetId();
        }
    }

    return chat::Chatroom::NO_ROOM;
}


bool Server::ExistChatroom(size_t id) const noexcept {
    return (std::find_if(m_chatrooms.cbegin(), m_chatrooms.cend(), [id](const chat::Chatroom& rhs){
        return rhs.GetId() == id;
    }) != m_chatrooms.cend());
}

void Server::RemoveChatroom(size_t chatroomId, bool mustClose) {
    const auto it = std::find_if(m_chatrooms.begin(), m_chatrooms.end(), [chatroomId](const chat::Chatroom& rhs){
        return rhs.GetId() == chatroomId;
    });
    if( it != m_chatrooms.end() ) {
        if( mustClose ) it->Close();
        *it = std::move(m_chatrooms.back());
        m_chatrooms.pop_back();
    }
}

bool Server::IsEmpty(size_t chatroomId) const noexcept {
    const auto it = std::find_if(m_chatrooms.cbegin(), m_chatrooms.cend(), [chatroomId](const chat::Chatroom& rhs) {
        return rhs.GetId() == chatroomId;
    });
    return (it == m_chatrooms.cend() || it->IsEmpty());
}