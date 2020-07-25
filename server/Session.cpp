#include "Session.hpp"
#include "Server.hpp"
#include "Request.hpp"
#include "RequestType.hpp"
#include "Utility.hpp"

#include <functional>
#include <iostream>
#include <sstream>

Session::Session( 
    asio::ip::tcp::socket && socket, 
    Server * server 
) :
    m_socket { std::move(socket) },
    m_server { server },
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
    // TODO: is it limit which we can read? If it's true, 
    // then confirm that the situation with full buffer has been processed.
    m_inbox.prepare(1024);

    asio::async_read_until(
        m_socket,
        m_inbox,
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
    m_isClosed = true;

    boost::system::error_code ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if(ec) { 
        std::cerr<< "Session's socket called shutdown with error: " << ec.message() << '\n';
    }
    ec.clear();
    
    m_socket.close(ec);
    if(ec) {
        std::cerr<< "Session's socket is being closed with error: " << ec.message() << '\n';
    } 
}

void Session::ReadSomeHandler(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if(!error) {
        std::cout << "Session just recive: " << transferredBytes << " bytes.\n";
        
        m_inbox.commit(transferredBytes);
        
        const auto data { m_inbox.data() }; // asio::streambuf::const_buffers_type
        std::string recieved {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes
        };
        
        boost::system::error_code error; 
        std::cerr << m_socket.remote_endpoint(error) << ": " << recieved << '\n' ;

        m_inbox.consume(transferredBytes);
        
        // TODO: request may not come fully in this operation
        // so solve this situation too.
        Requests::Request incomingRequest {};
        const auto result = incomingRequest.Parse(recieved);
        if( !result ) {
            this->Write(this->SolveRequest(incomingRequest));
        } else {
            std::cerr << "Parsing request: {" 
                << recieved 
                << "} failed with error code: " 
                << result << '\n';
        }
                
        this->Read();
    } 
    else {
        std::cerr << "Session trying to read invoked error: " << error.message() << "\n";
    }
}

void Session::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if(!error) {
        std::cout << "Session sent: " << transferredBytes << " bytes\n";

        if( m_outbox.GetQueueSize() ) {
            // we need to Write other data
            std::cout << "Session need to Write " << m_outbox.GetQueueSize() << " messages.\n";
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
        std::cerr << "Session has error trying to write: " << error.message() << "\n";
        this->Close();
    }
}


std::string Session::SolveRequest(const Requests::Request& request) {
    Requests::Request reply {};
    switch(m_state) {
        case State::ACCEPTED :
            {
                reply.SetType(Requests::RequestType::POST);
                // check if it's an auth request; 
                const auto isAuthRequest { request.GetType() == Requests::RequestType::AUTHORIZE };
                if(isAuthRequest) { // send confirm post request
                    const std::string body {
                        "Welcome, " + request.GetName() + "!\n"
                        "Thank you for using my chat application!\n"
                    };
                    reply.SetBody(body);
                    m_state = State::AUTHORIZED;
                } else { // send warning about wrong expected request 
                    const std::string body {
                        "Welcome! Is your name '" + request.GetName() + "'?\n"
                        "Sorry, but you send wrong request type.\n"
                        "Authorization expected!\n"
                    };
                    reply.SetBody(body);
                }
            } break;
        case State::AUTHORIZED : 
            { // process request
                reply.SetType(Requests::RequestType::POST);
                const auto expectedRequests {
                    Utils::CreateMask(
                        Requests::RequestType::LIST_CHATROOM,
                        Requests::RequestType::JOIN_CHATROOM,
                        Requests::RequestType::ABOUT_CHATROOM,
                        Requests::RequestType::CREATE_CHATROOM
                    )
                };
                if(Utils::EnumCast(request.GetType()) & expectedRequests) {
                    /// TODO: solve base on request type
                    if(request.GetType() == Requests::RequestType::LIST_CHATROOM) {
                        const std::string body {
                            "Here goes list of avaible chatrooms:\n"
                            "Test chatroom #1\n"
                            "Test chatroom #2345\n"
                        };
                        reply.SetBody(body);    
                    }
                } else {
                    const std::string body {
                        "Welcome! Is your name '" + request.GetName() + "'?\n"
                        "Sorry, but you send wrong request type.\n"
                        "Try to submit one of those request:\n"
                        "--list-chatroom\n"
                        "--join-chatroom\n"
                        "--about-chatroom\n"
                        "--create-chatroom"
                    };
                    reply.SetBody(body);    
                }
            }
            break;
        case State::BUSY : 
            // asio::post(m_strand, std::bind(&Server::BroadcastEveryoneExcept, m_server, msg, this->shared_from_this()));
            break;
        default: break;
    };
    return reply.Serialize();
}
