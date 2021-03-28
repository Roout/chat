#include "Connection.hpp"

#include <utility>
#include <sstream>
#include <string>

#include "Message.hpp"
#include "RequestQueue.hpp"

#include "Session.hpp"

namespace net {

Connection::Connection( 
    std::size_t id
    , asio::ip::tcp::socket && socket
    , asio::io_context * const context
    , std::shared_ptr<rt::RequestQueue> incommingRequests
)   
    : m_socket { std::move(socket) }
    , m_strand { *context }
    , m_timer { *context }
    , m_incommingRequests { incommingRequests }
{ 
    std::stringstream ss;
    ss << "connection_" << m_id << "_log.txt";
    m_logger = std::make_shared<Log>(ss.str().c_str());
}

Connection::~Connection() {
    this->Close();
}

void Connection::Publish() {
    if (auto ptr = m_subscriber.lock()) {
        ptr->AcquireRequests();
    }
}

void Connection::AddSubscriber(Session* session) {
    m_subscriber = session->weak_from_this();
}

void Connection::Close() {
    if (m_state != State::CLOSED) {
        boost::system::error_code ec;
        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) { 
            this->AddLog(LogType::error, 
                "Connection's socket called shutdown with error: "
                , ec.message()
                , '\n'
            );
            ec.clear();
        }
        m_socket.close(ec);
        if (ec) {
            this->AddLog(LogType::error, 
                "Connection's socket is being closed with error: "
                , ec.message()
                , '\n'
            );
        } 
        m_state = State::CLOSED;
    }
}

void Connection::Read(std::size_t ms, TimerCallback&& callback) {
    // start reading requests
    this->Read();
    // set up deadline timer for the SYN request from the accepted connection
    m_timer.expires_from_now(boost::posix_time::milliseconds(ms));
    m_timer.async_wait(asio::bind_executor(m_strand, std::move(callback)));
}


void Connection::Write(std::string&& text) {
    asio::post(m_strand, [text = std::move(text), self = shared_from_this()]() mutable {
        self->m_outbox.Enque(std::move(text));
        if (self->m_state != State::WRITING) {
            self->Write();
        }
    });
}

void Connection::Read() {
    asio::async_read_until(
        m_socket,
        m_inbox,
        Internal::MESSAGE_DELIMITER,
        asio::bind_executor(
            m_strand, 
            std::bind(&Connection::ReadSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Connection::Write() {
    // add all text that is queued for write operation to active buffer
    m_outbox.SwapBuffers();
    // initiate write operation
    m_state = State::WRITING;
    asio::async_write(
        m_socket,
        m_outbox.GetBufferSequence(),
        asio::bind_executor(
            m_strand,
            std::bind(&Connection::WriteSomeHandler, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

void Connection::WriteSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if (!error) {
        this->AddLog(LogType::info, 
            "Connection sent: ", transferredBytes, " bytes.\n"
        );
        if (m_outbox.GetQueueSize()) {
            // we need to Write other data
            this->AddLog(LogType::info, 
                "Connection need to Write ", m_outbox.GetQueueSize(), " messages.\n"
            );
            this->Write();
        } 
        else {
            m_state = State::DEFAULT;
        }
    } 
    else /* if (error == boost::asio::error::eof) */ {
        // Connection was closed by the remote peer 
        // or any other error happened 
        this->AddLog(LogType::error, 
            "Connection has error trying to write: ", error.message(), '\n'
        );
        this->Close();
    }
}

void Connection::ReadSomeHandler(
    const boost::system::error_code& error, 
    std::size_t transferredBytes
) {
    if (!error) {
        this->AddLog(LogType::info, 
            "Connection just recive: ", transferredBytes, " bytes.\n"
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
        Internal::Request request{};
        request.Read(received);
        // TODO: it can be added via `asio::post` and return immediatly
        // or not read the socket until request will be pushed to the queue
        // (as queue is shared resource it can be blocked)
        m_incommingRequests->Push(std::move(request));
        this->Publish();
        this->Read();
    } 
    else {
        this->AddLog(LogType::error, 
            "Connection trying to read invoked error: ", error.message(), '\n'
        );
    }
}


} // net