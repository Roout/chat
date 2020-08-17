#include "Session.hpp"
#include "Server.hpp"
#include "Request.hpp"
#include "RequestType.hpp"
#include "InteractionStage.hpp"
#include "Utility.hpp"
#include "Chatroom.hpp"

#include <functional>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>

Session::Session( 
    asio::ip::tcp::socket && socket, 
    Server * const server 
) :
    m_socket { std::move(socket) },
    m_server { server },
    m_chatroom { chat::Chatroom::NO_ROOM },
    m_strand { *server->m_context }
{
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
        m_inbox, // What is limit of asio::streambuf
        Requests::REQUEST_DELIMETER,
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

void Session::Close() {
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

    m_isClosed = true;
}

void Session::ReadSomeHandler(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if(!error) {
        m_server->Write(LogType::info, 
            "Session just recive: ", transferredBytes, " bytes.\n"
        );
        // boost::asio::async_read_until calls commit by itself
        // m_inbox.commit(transferredBytes);
        
        const auto data { m_inbox.data() }; // asio::streambuf::const_buffers_type
        std::string recieved {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes
        };
    
        m_inbox.consume(transferredBytes);
        
        // TODO: request may not come fully in this operation
        // so solve this situation too.
        Requests::Request incomingRequest {};
        const auto result = incomingRequest.Parse(recieved);

        boost::system::error_code ec; 
        m_server->Write(LogType::info, 
            m_socket.remote_endpoint(ec), ": ", incomingRequest.AsString(), '\n'
        );

        if( !result ) {
            const auto reply { this->SolveRequest(incomingRequest) };
            this->Write(reply);
        } else {
            m_server->Write(LogType::error, 
                "Parsing request: {\n", incomingRequest.AsString(), "} failed with error code: ", result, '\n'
            );
        }
                
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

        if( m_outbox.GetQueueSize() ) {
            // we need to Write other data
            m_server->Write(LogType::info, 
                "Session need to Write ", m_outbox.GetQueueSize(), " messages.\n"
            );
            asio::post(m_strand, [self = shared_from_this()](){
                self->Write();
            }); 
        } else {
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

bool Session::AssignChatroom(size_t id) {
    const bool result = m_server->AssignChatroom(id, shared_from_this());
    if( result ) {
        m_chatroom = id;
    }
    return result;
}

bool Session::LeaveChatroom() {
    // leave current chatroom if exist
    if(m_chatroom != chat::Chatroom::NO_ROOM) {
        m_server->LeaveChatroom(m_chatroom, shared_from_this());
        m_chatroom = chat::Chatroom::NO_ROOM;
        return true;
    }
    return false;
}

bool Session::ValidateAuth(const Requests::Request& request) const noexcept {
    // for now it's just a dummy without logic
    return request.GetType() == Requests::RequestType::AUTHORIZE;
}

std::string Session::SolveRequest(const Requests::Request& request) {
    Requests::Request reply {};
    switch(m_state) {
        case IStage::State::UNAUTHORIZED:
            {
                // check if it's an auth request; 
                if(this->ValidateAuth(request)) { // send confirm AUTHORIZE request
                    reply.SetType(Requests::RequestType::AUTHORIZE);
                    reply.SetStage(IStage::State::AUTHORIZED);
                    reply.SetCode(Requests::ErrorCode::SUCCESS);
                    const std::string body {
                        "Welcome, " + request.GetName() + "!\n"
                        "Thank you for using my chat application!"
                    };
                    reply.SetBody(body);
                    m_state = IStage::State::AUTHORIZED;
                } else { // send warning about wrong expected request 
                    reply.SetType(request.GetType());
                    reply.SetStage(IStage::State::UNAUTHORIZED);
                    reply.SetCode(Requests::ErrorCode::FAILURE);
                    const std::string body {
                        "Welcome! Is your name '" + request.GetName() + "'?\n"
                        "Sorry, but you send wrong request type.\n"
                        "Authorization expected!"
                    };
                    reply.SetBody(body);
                }
            } break;
        case IStage::State::AUTHORIZED : 
            { // process request
                if(request.GetType() == Requests::RequestType::UNDEFINED) {
                    reply.SetType(Requests::RequestType::POST);
                    reply.SetStage(IStage::State::AUTHORIZED);
                    const std::string body {
                        "Welcome! Is your name '" + request.GetName() + "'?\n"
                        "Sorry, but you send wrong request type.\n"
                        "Try to submit one of those request:\n"
                        "--list-chatroom\n"
                        "--join-chatroom\n"
                        "--create-chatroom\n"
                        "--about-chatroom\n"
                        "--chat-message\n"
                        "--leave-chatroom"
                    };
                    reply.SetBody(body);
                    break;  
                }

                const auto requestType { request.GetType() };

                reply.SetType(requestType);
                reply.SetStage(IStage::State::AUTHORIZED);
                if(requestType == Requests::RequestType::LIST_CHATROOM) {
                    reply.SetCode(Requests::ErrorCode::SUCCESS);
                    std::string body {};
                    for(auto& str: m_server->GetChatroomList()) {
                        body += std::move(str);
                        body.push_back('\n');
                    }
                    body.pop_back(); // last '\n'
                    reply.SetBody(body);   
                } 
                else if(requestType == Requests::RequestType::JOIN_CHATROOM ) {
                    std::array<std::function<bool()>, 4> testTable;
                    testTable[0] = [&request]() { // Was id included to request?
                        return (request.GetChatroom() != 0U);
                    };
                    testTable[1] = [this]() { // Is the user in the `hall` chatroom?
                        return m_server->m_hall.Contains(this);
                    };
                    testTable[2] = [this, &request]() { // Did a chatroom with given id exist?
                        return m_server->ExistChatroom(request.GetChatroom());
                    };
                    testTable[3] = [this, &request]() { // Are there any free space for new user?
                        // return true, if chatroom was joined successfully 
                        // otherwise return false. Which means that the chatroom is most likely full.
                        return this->AssignChatroom(request.GetChatroom());
                    };

                    size_t errorCode { 0 };
                    for(size_t test = 0; test < testTable.size(); test++) {
                        if( !std::invoke(testTable[test]) ) {
                            errorCode = test + 1;
                            break;
                        }
                    }
                    std::string errorMessage {};
                    switch(errorCode) {
                        case 0: errorMessage = "You've successfully joined the chatroom"; break;
                        case 1: errorMessage = "ID wasn't included to the request"; break; 
                        case 2: errorMessage = "User is already in the chatroom"; break;
                        case 3: errorMessage = "Can't find room with given id"; break; 
                        case 4: errorMessage = "No free space in the room"; break;
                        default: errorMessage = "Some weird error, sorry!"; break;
                    }
                    if( !errorCode ) {
                        reply.SetChatroom(request.GetChatroom());
                        reply.SetName(request.GetName());
                    }
                    reply.SetCode(errorCode? Requests::ErrorCode::FAILURE: Requests::ErrorCode::SUCCESS);
                    reply.SetBody(errorMessage);
                } 
                else if(requestType == Requests::RequestType::CREATE_CHATROOM ) {
                    size_t errorCode { 0 };
                    if(!m_server->m_hall.Contains(this)) {
                        errorCode = 1;
                    } 
                    else if (request.GetName().empty()) {
                        errorCode = 2;
                    }
                    std::string errorMessage {};
                    switch(errorCode) {
                        case 0: errorMessage = "You've successfully create and joined the chatroom"; break;
                        case 1: errorMessage = "User is already in the chatroom"; break;
                        case 2: errorMessage = "Room's name is empty!"; break;
                        default: errorMessage = "Some weird error, sorry!"; break;
                    }
                    if( !errorCode ) {
                        // create room
                        const auto id = m_server->CreateChatroom(request.GetName());
                        // add to this room
                        const auto result = this->AssignChatroom(id);
                        if (!result) {
                            // Some weird error, need to check is it even possible.
                            // I think it should only throw exceptions as it's impossible to fail adding
                            // user to empty room.
                        }
                        reply.SetChatroom(id);
                        reply.SetName(request.GetName());
                    }
                    reply.SetCode(errorCode? Requests::ErrorCode::FAILURE: Requests::ErrorCode::SUCCESS);
                    reply.SetBody(errorMessage);
                }
                else if(requestType == Requests::RequestType::LEAVE_CHATROOM ) {
                    std::array<std::function<bool()>, 2> testTable;
                    testTable[0] = [this]() {  // Is this session in the chatroom? 
                        return (m_chatroom != chat::Chatroom::NO_ROOM);
                    };
                    testTable[1] = [this, &request]() { // Does this chatroom exist?
                        return m_server->ExistChatroom(m_chatroom);
                    };
                    
                    size_t errorCode { 0 };
                    for(size_t test = 0; test < testTable.size(); test++) {
                        if( !std::invoke(testTable[test]) ) {
                            errorCode = test + 1;
                            break;
                        }
                    }
                    std::string errorMessage {};
                    switch(errorCode) {
                        case 0: errorMessage = "You've successfully leave the chatroom"; break;
                        case 1: errorMessage = "You're not in the chatroom"; break; 
                        case 2: errorMessage = "Chatroom this session belongs no longer exist"; break;
                        default: errorMessage = "Some weird error, sorry!"; break;
                    }
                    if( !errorCode ) {
                        // leave chatroom & join hall!
                        const auto chatroom = m_chatroom;
                        this->LeaveChatroom();
                        // if chatroom is empty -> remove it
                        if( m_server->IsEmpty(chatroom) ) {
                            const bool closeConnections { false };
                            m_server->RemoveChatroom(chatroom, closeConnections);
                        }
                    }
                    reply.SetCode(errorCode? Requests::ErrorCode::FAILURE: Requests::ErrorCode::SUCCESS);
                    reply.SetBody(errorMessage);
                }
                else if(requestType == Requests::RequestType::ABOUT_CHATROOM ) {
                    size_t errorCode { 0 };
                    const auto chatroomId = request.GetChatroom();

                    if( chatroomId == chat::Chatroom::NO_ROOM) {
                        // Request has room's id
                        errorCode = 1;
                    } 
                    else if( !m_server->ExistChatroom(chatroomId) ) {
                        // Chatroom doesn't exist
                        errorCode = 2;
                    }

                    std::string errorMessage {};
                    switch(errorCode) {
                        case 0: errorMessage = m_server->m_chatrooms.at(chatroomId).GetRepresentation(); break;
                        case 1: errorMessage = "User is already in the chatroom"; break;
                        case 2: errorMessage = "Chatroom doesn't exist"; break;
                        default: errorMessage = "Some weird error, sorry!"; break;
                    }
                    reply.SetCode(errorCode? Requests::ErrorCode::FAILURE: Requests::ErrorCode::SUCCESS);
                    reply.SetBody(errorMessage);
                } 
                else { // Requests::RequestType::CHAT_MESSAGE
                    size_t errorCode { 0 };
                    // 
                }
            }
            break;
        default: break;
    };
    return reply.Serialize();
}
