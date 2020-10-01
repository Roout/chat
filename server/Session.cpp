#include "Session.hpp"
#include "RoomService.hpp"
#include "classes/Message.hpp"
#include "classes/Utility.hpp"
#include "Chatroom.hpp"
#include "RequestHandlers.hpp"

#include <functional>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>

Session::Session( 
    asio::ip::tcp::socket && socket, 
    chat::RoomService * const service,
    asio::io_context * const context
) :
    m_socket { std::move(socket) },
    m_service { service },
    m_strand { *context },
    m_timer { *context }
{
    m_user.m_chatroom = chat::Chatroom::NO_ROOM;
    std::stringstream ss;
    ss << "session_" << m_user.m_id << "_log.txt";
    m_logger = std::make_shared<Log>(ss.str().c_str());
}

void Session::Write(std::string text) {
    asio::post(m_strand, [text = std::move(text), self = shared_from_this()]() mutable {
        self->m_outbox.Enque(std::move(text));
        if( !self->m_isWriting ) {
            self->Write();
        }
    });
}

void Session::Write() {
    // add all text that is queued for write operation to active buffer
    m_outbox.SwapBuffers();
    // initiate write operation
    m_isWriting = true;
    asio::async_write(
        m_socket,
        m_outbox.GetBufferSequence(),
        asio::bind_executor(
            m_strand,
            std::bind(&Session::WriteSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Session::Read() {
    asio::async_read_until(
        m_socket,
        m_inbox,
        Internal::MESSAGE_DELIMITER,
        asio::bind_executor(
            m_strand, 
            std::bind(&Session::ReadSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Session::ExpiredDeadlineHandler(
    const boost::system::error_code& error
) {
    bool isCanceled { error == boost::asio::error::operation_aborted };
    bool endWithoutError = !error || isCanceled;
    if( endWithoutError && this->IsWaitingSyn() ) {
        this->AddLog(LogType::info, "Connection has been closed due to timeout.");
        this->Close();
    } 
    else {
        // TODO: some weird error
    }
}

void Session::WaitSynchronizationRequest() {
    // start reading requests
    this->Read();
    // set up deadline timer for the SYN request from the accepted connection
    m_timer.expires_from_now(boost::posix_time::milliseconds(m_synTime));
    m_timer.async_wait(
        asio::bind_executor(
            m_strand, 
            std::bind(&Session::ExpiredDeadlineHandler, 
                this->shared_from_this(), 
                std::placeholders::_1
            )
        )
    );
}


void Session::Close() {
    /// TODO: get rid of the errors about normal shutdown
    boost::system::error_code ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if(ec) { 
        this->AddLog(LogType::error, 
            "Session's socket called shutdown with error: ", ec.message(), '\n'
        );
        ec.clear();
    }
    m_socket.close(ec);
    if(ec) {
        this->AddLog(LogType::error, 
            "Session's socket is being closed with error: ", ec.message(), '\n'
        );
    } 
    m_state = State::DISCONNECTED;
}

void Session::ReadSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        this->AddLog(LogType::info, 
            "Session just recive: ", transferredBytes, " bytes.\n"
        );
        const auto data { m_inbox.data() };
        std::string received {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes - Internal::MESSAGE_DELIMITER.size()
        };
        m_inbox.consume(transferredBytes);
        
        boost::system::error_code ec; 
        this->AddLog(LogType::info, 
            m_socket.remote_endpoint(ec), ": ", received, '\n'
        );

        // Handle exceptions
        Internal::Request incomingRequest{};
        incomingRequest.Read(received);
        this->HandleRequest(incomingRequest);
        this->Read();
    } 
    else {
        this->AddLog(LogType::error, 
            "Session trying to read invoked error: ", error.message(), '\n'
        );
    }
}

void Session::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        this->AddLog(LogType::info, 
            "Session sent: ", transferredBytes, " bytes.\n"
        );
        if(m_outbox.GetQueueSize()) {
            // we need to Write other data
            this->AddLog(LogType::info, 
                "Session need to Write ", m_outbox.GetQueueSize(), " messages.\n"
            );
            asio::post(m_strand, [self = shared_from_this()](){
                self->Write();
            }); 
        } 
        else {
            m_isWriting = false;
        }
    } 
    else /* if(error == boost::asio::error::eof) */ {
        // Connection was closed by the remote peer 
        // or any other error happened 
        this->AddLog(LogType::error, 
            "Session has error trying to write: ", error.message(), "\n"
        );
        this->Close();
    }
}

bool Session::AssignChatroom(std::size_t id) {
    if(m_service->AssignChatroom(id, shared_from_this())) {
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
    if(m_user.m_chatroom != chat::Chatroom::NO_ROOM) {
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

void Session::HandleRequest(const Internal::Request& request) {
    switch(request.m_query) {
        case Internal::QueryType::SYN: { 
            CreateExecutor<Internal::QueryType::SYN>(&request, this)->Run();
        } break;
        case Internal::QueryType::LEAVE_CHATROOM: { 
            CreateExecutor<Internal::QueryType::LEAVE_CHATROOM>(&request, this)->Run();
        } break;
        case Internal::QueryType::JOIN_CHATROOM: {
            CreateExecutor<Internal::QueryType::JOIN_CHATROOM>(&request, this)->Run();
        } break;
        case Internal::QueryType::CREATE_CHATROOM: {
            CreateExecutor<Internal::QueryType::CREATE_CHATROOM>(&request, this)->Run();
        } break;
        case Internal::QueryType::LIST_CHATROOM: {
            CreateExecutor<Internal::QueryType::LIST_CHATROOM>(&request, this)->Run();
        } break;
        case Internal::QueryType::CHAT_MESSAGE: {
            CreateExecutor<Internal::QueryType::CHAT_MESSAGE>(&request, this)->Run();
        } break;
    }
    /// TODO: handle unexpected request
}
