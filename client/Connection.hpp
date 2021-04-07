#ifndef CLIENT_CONNECTION_HPP__
#define CLIENT_CONNECTION_HPP__

#include <memory>
#include <string_view>
#include <cstdint>
#include <cstddef>
#include <functional>

#include "DoubleBuffer.hpp"
#include "Message.hpp"
#include "Log.hpp"
#include "Client.hpp"

#include <boost/asio.hpp>


/** TODO: implement these functions for Stream
 * - [ ] close
 * - [ ] shutdown
 * - [ ] async_connect
 * - [ ] remote_endpoint
 * - [ ] constructor
 * - [x] async_write_some
 * - [x] async_read_some
*/

namespace client {

// TODO: add constrains for Stream type
template<class Stream>
class Connection 
    : public std::enable_shared_from_this<Connection<Stream>> {
public:

    Connection(std::weak_ptr<Client> client
        , std::shared_ptr<boost::asio::io_context> context
    );

    /**
     * Close connection if it's haven't been terminated yet. 
     */
    ~Connection();

    /**
     * Trying to connect to the remote host
     * 
     * @param path
     *  It's ip address of the remote host
     * @param port
     *  It's a port on which a remote host is listening  
     */
    void Connect(std::string_view path, std::string_view port);

    void Write(std::string text);

    void Close();

private:
    /**
     * This method initialite handshake with server `on connection` event.
     * I.e. it sends a request with SYN query type to server.
     */
    void Synchronize();

    void OnConnect(const boost::system::error_code& err);

    void Read();
    
    /**
     * Completion handler. 
     * Called when the client read something from the remote endpoint
     */
    void OnRead(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );

    /**
     * Send everything from the passive buffers to remote peer.  
     * It is called from the function wrapped in strand to prevent concurrent 
     * access to outgoing buffer and other variables. 
     */
    void Write();

    /**
     * Completion handler; called when the client write something to the remote endpoint
     */
    void OnWrite(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );

private:
    Log m_logger { "client_log.txt" };
    
    std::weak_ptr<Client> m_client;
    std::shared_ptr<boost::asio::io_context> m_io;
    boost::asio::io_context::strand m_strand;
    Stream m_stream;
    
    bool m_isWriting { false };
    boost::asio::streambuf m_inbox;
    Buffers m_outbox;
};

template<class Stream>
Connection<Stream>::Connection(std::weak_ptr<Client> client
    , std::shared_ptr<boost::asio::io_context> context
) 
    : m_client { client }
    , m_io { context }
    , m_strand { *context }
    , m_stream { *context }
{}

template<class Stream>
Connection<Stream>::~Connection() {}

template<class Stream>
void Connection<Stream>::Connect(std::string_view path, std::string_view port) {
    asio::ip::tcp::resolver resolver(*m_io);
    const auto endpoints = resolver.resolve(path, port);
    if (endpoints.empty()) {
        this->Close();
    }
    else {
        const auto endpoint { endpoints.cbegin()->endpoint() };
        m_stream.async_connect(
            endpoint, 
            std::bind(&Connection::OnConnect, this->shared_from_this(), std::placeholders::_1)
        );
    }
}

template<class Stream>
void Connection<Stream>::OnConnect(const boost::system::error_code& error) {
    if (error) {
        m_logger.Write(LogType::error,
            "Connection failed to connect with error: ", error.message(), "\n"
        );
    } 
    else {
        m_logger.Write(LogType::info, "Connection connected successfully!\n");
        if(auto model = m_client.lock(); model) {
            // update state
            model->SetState(Client::State::WAIT_ACK);
        }
        // send SYN
        this->Synchronize();
        // start waiting incoming calls
        this->Read();
    }
}

template<class Stream>
void Connection<Stream>::Close() {
    boost::asio::post(m_strand, [self = this->shared_from_this()]() {
        boost::system::error_code error;
        self->m_stream.shutdown(asio::ip::tcp::socket::shutdown_both, error);
        if (error) {
            self->m_logger.Write(LogType::error, 
                "Connection's socket called shutdown with error: ", error.message(), '\n'
            );
            error.clear();
        }
        
        self->m_stream.close(error);
        if (error) {
            self->m_logger.Write(LogType::error, 
                "Connection's socket is being closed with error: ", error.message(), '\n'
            );
        }
        self->m_isWriting = false;

        if(auto model = self->m_client.lock(); model) {
            model->SetState(Client::State::CLOSED);
        }
    });
}

template<class Stream>
void Connection<Stream>::Read() {
    boost::asio::async_read_until(
        m_stream,
        m_inbox,
        Internal::MESSAGE_DELIMITER,
        boost::asio::bind_executor(
            m_strand,
            std::bind(&Connection::OnRead, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

template<class Stream>
void Connection<Stream>::Write() {
    m_isWriting = true;
    m_outbox.SwapBuffers();
    boost::asio::async_write(
        m_stream,
        m_outbox.GetBufferSequence(),
        boost::asio::bind_executor(
            m_strand,
            std::bind(&Connection::OnWrite, 
                this->shared_from_this(), 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        )
    );
}

template<class Stream>
void Connection<Stream>::Write(std::string text) {
    // using strand we prevent concurrent access to variables and 
    // concurrent writing to socket. 
    boost::asio::post(m_strand, [text = std::move(text), self = this->shared_from_this()]() mutable {
        self->m_outbox.Enque(std::move(text));
        if (!self->m_isWriting) {
            self->Write();
        } 
    });
}

template<class Stream>
void Connection<Stream>::OnRead(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if (!error) {
        m_logger.Write(LogType::info, "Connection just recive:", transferredBytes, "bytes.\n");
        
        const auto data { m_inbox.data() }; // asio::streambuf::const_buffers_type
        std::string received {
            asio::buffers_begin(data), 
            asio::buffers_begin(data) + transferredBytes - Internal::MESSAGE_DELIMITER.size()
        };
        m_inbox.consume(transferredBytes);
        
        Internal::Response incomingResponse;
        incomingResponse.Read(received);
        
        boost::system::error_code error; 
        m_logger.Write(LogType::info, m_stream.remote_endpoint(error), ':', received, '\n');

        if(auto model = m_client.lock(); model) {
            /* if it's ACK, we resolve the State in synchronous way, otherwise resolve asyncronously */
            model->HandleMessage(incomingResponse);
        }
        this->Read();
    } 
    else {
        m_logger.Write(LogType::error, "Connection failed to read with error:", error.message(), "\n");
        this->Close();
    }
}

template<class Stream>
void Connection<Stream>::OnWrite(
    const boost::system::error_code& error, 
    size_t transferredBytes
) {
    if (!error) {
        m_logger.Write(LogType::info, "Connection just sent:", transferredBytes, "bytes\n");
        if (m_outbox.GetQueueSize()) {
            // we need to send other data
            this->Write();
        } 
        else {
            m_isWriting = false;
        }
    } 
    else {
        m_isWriting = false;
        m_logger.Write(LogType::error, "Connection has error on writting:", error.message(), '\n');
        this->Close();
    }
} 

template<class Stream>
void Connection<Stream>::Synchronize() {
    std::string serialized{};
    // Internal::Request request = CreateSynchronizeRequest();
    // request.Write(serialized);
    if(auto model = m_client.lock(); model) {
        model->SetState(Client::State::WAIT_ACK);
    }
    this->Write(std::move(serialized));
}

} // namespace client

#endif // CLIENT_CONNECTION_HPP__