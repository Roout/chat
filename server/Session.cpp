#include "Session.hpp"
#include "Server.hpp"
#include "Message.hpp"
#include "Utility.hpp"
#include "Chatroom.hpp"
#include "RequestHandlers.hpp"

#include <functional>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

Session::Session( 
    asio::ip::tcp::socket && socket, 
    Server * const server 
) :
    m_socket { std::move(socket) },
    m_server { server },
    m_strand { *server->m_context },
    m_timer { *server->m_context }
{
    m_user.m_chatroom = chat::Chatroom::NO_ROOM;
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
        m_server->Write(LogType::info, "Connection has been closed due to timeout.");
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
        m_server->Write(LogType::error, 
            "Session's socket called shutdown with error: ", ec.message(), '\n'
        );
        ec.clear();
    }
    m_socket.close(ec);
    if(ec) {
        m_server->Write(LogType::error, 
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
        m_server->Write(LogType::info, 
            "Session just recive: ", transferredBytes, " bytes.\n"
        );
        const auto data { m_inbox.data() };
        std::string received {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes - Internal::MESSAGE_DELIMITER.size()
        };
        m_inbox.consume(transferredBytes);
        
        boost::system::error_code ec; 
        m_server->Write(LogType::info, 
            m_socket.remote_endpoint(ec), ": ", received, '\n'
        );

        // Handle exceptions
        Internal::Request incomingRequest{};
        incomingRequest.Read(received);
        this->HandleRequest(incomingRequest);
        this->Read();
    } 
    else {
        m_server->Write(LogType::error, 
            "Session trying to read invoked error: ", error.message(), '\n'
        );
    }
}

void Session::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        m_server->Write(LogType::info, 
            "Session sent: ", transferredBytes, " bytes.\n"
        );

        if(m_outbox.GetQueueSize()) {
            // we need to Write other data
            m_server->Write(LogType::info, 
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
        m_server->Write(LogType::error, 
            "Session has error trying to write: ", error.message(), "\n"
        );
        this->Close();
    }
}

bool Session::AssignChatroom(std::size_t id) {
    bool isAssigned = m_server->AssignChatroom(id, shared_from_this());
    if(isAssigned) {
        m_user.m_chatroom = id;
    }
    return isAssigned;
}

bool Session::LeaveChatroom() {
    // leave current chatroom if exist
    if(m_user.m_chatroom != chat::Chatroom::NO_ROOM) {
        m_server->LeaveChatroom(m_user.m_chatroom, shared_from_this());
        m_user.m_chatroom = chat::Chatroom::NO_ROOM;
        return true;
    }
    return false;
}

std::size_t Session::CreateChatroom(const std::string& chatroomName) {
    const auto roomId = m_server->CreateChatroom(chatroomName); 
    return roomId;
}

std::vector<std::string> Session::GetChatroomList() const noexcept {
    return m_server->GetChatroomList();
}

template<class Encoding, class Allocator>
std::string Serialize(const rapidjson::GenericValue<Encoding, Allocator>& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return { buffer.GetString(), buffer.GetLength() };
}


void Session::HandleRequest(const Internal::Request& request) {
    switch(request.m_query) {
        case Internal::QueryType::SYN: { 
            auto executor = CreateExecutor<Internal::QueryType::SYN>(&request, this);
            executor->Run();
        } break;
        case Internal::QueryType::LEAVE_CHATROOM: { 
            auto executor = CreateExecutor<Internal::QueryType::LEAVE_CHATROOM>(&request, this);
            executor->Run();
        } break;
        case Internal::QueryType::JOIN_CHATROOM: {
            auto executor = CreateExecutor<Internal::QueryType::JOIN_CHATROOM>(&request, this);
            executor->Run();
        } break;
        case Internal::QueryType::CREATE_CHATROOM: {
            auto executor = CreateExecutor<Internal::QueryType::CREATE_CHATROOM>(&request, this);
            executor->Run();
        } break;
        case Internal::QueryType::LIST_CHATROOM: {
            auto executor = CreateExecutor<Internal::QueryType::LIST_CHATROOM>(&request, this);
            executor->Run();
        } break;
    }
    /// TODO: handle unexpected request
}
