#include "Session.hpp"

#include "Message.hpp"
#include "RequestQueue.hpp"

#include "RoomService.hpp"
#include "Chatroom.hpp"
#include "RequestHandlers.hpp"
#include "Connection.hpp"

Session::Session( 
    asio::ip::tcp::socket && socket, 
    std::shared_ptr<chat::RoomService> service,
    std::shared_ptr<asio::io_context> context
) 
    : m_service { service }
    , m_incommingRequests { std::make_shared<rt::RequestQueue>() }
    , m_context { context }
    , m_connection { std::make_shared<net::Connection>(
        m_user.m_id
        , std::move(socket)
        , m_context.get()
        , m_incommingRequests
    )}
{
    m_user.m_chatroom = chat::Chatroom::NO_ROOM;
}

void Session::Close() {
    m_connection->Close();
}

/**
 * 1. Acquire all data from the extern queue (move it to our queue)
 * 2. Handle requests one by one from the own queue
 * 3. If extern queue is not empty -> go to step 1; otherwise stop
 */
void Session::AcquireRequests() {
    asio::post(*m_context, [self = this->shared_from_this()]() {
        rt::RequestQueue buffer{};
        self->m_incommingRequests->Swap(buffer);
        while (!buffer.IsEmpty()) {
            self->HandleRequest(buffer.Extract());
        }
        if (!self->m_incommingRequests->IsEmpty()) {
            self->AcquireRequests();
        }
    });
}

void Session::Subscribe() {
    m_connection->AddSubscriber(this);
}

void Session::Read() {
    m_connection->Read(m_timeout, 
        [self = this->shared_from_this()](const boost::system::error_code& error) {
            const bool isCanceled { error == boost::asio::error::operation_aborted };
            const bool endWithoutError { !error || isCanceled };
            const bool isAcknowledged { self->m_state == State::ACKNOWLEDGED };
            if (endWithoutError && !isAcknowledged) {
                // self->AddLog(LogType::info, "Connection has been closed due to timeout.");
                self->Close();
            } 
        }
    );
    m_state = State::WAIT_SYN;
}

void Session::Write(std::string text) {
    m_connection->Write(std::move(text));
}

bool Session::AssignChatroom(std::size_t id) {
    if (m_service->AssignChatroom(id, this->shared_from_this())) {
        m_user.m_chatroom = id;
        return true;
    }
    return false;
}

void Session::BroadcastOnly(
    const std::string& message, 
    std::function<bool(const Session&)>&& condition
) {
    m_service->BroadcastOnly(this, message, std::move(condition));
}

bool Session::LeaveChatroom() {
    // leave current chatroom if exist
    if (m_user.m_chatroom != chat::Chatroom::NO_ROOM) {
        m_service->LeaveChatroom(m_user.m_chatroom, shared_from_this());
        m_user.m_chatroom = chat::Chatroom::NO_ROOM;
        return true;
    }
    return false;
}

std::size_t Session::CreateChatroom(const std::string& chatroomName) {
    const auto roomId = m_service->CreateChatroom(chatroomName); 
    return roomId;
}

std::vector<std::string> Session::GetChatroomList() const noexcept {
    return m_service->GetChatroomList();
}

void Session::HandleRequest(Internal::Request&& request) {
    using QueryType = Internal::QueryType;

    switch (request.m_query) {
        case QueryType::SYN: { 
            CreateExecutor<QueryType::SYN>(&request, this)->Run();
        } break;
        case QueryType::LEAVE_CHATROOM: { 
            CreateExecutor<QueryType::LEAVE_CHATROOM>(&request, this)->Run();
        } break;
        case QueryType::JOIN_CHATROOM: {
            CreateExecutor<QueryType::JOIN_CHATROOM>(&request, this)->Run();
        } break;
        case QueryType::CREATE_CHATROOM: {
            CreateExecutor<QueryType::CREATE_CHATROOM>(&request, this)->Run();
        } break;
        case QueryType::LIST_CHATROOM: {
            CreateExecutor<QueryType::LIST_CHATROOM>(&request, this)->Run();
        } break;
        case QueryType::CHAT_MESSAGE: {
            CreateExecutor<QueryType::CHAT_MESSAGE>(&request, this)->Run();
        } break;
    }
    /// TODO: handle unexpected request
}