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
#include <boost/asio/ssl.hpp>


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
        , std::shared_ptr<boost::asio::ssl::context> sslContext
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

    void Handshake();

    void OnConnect(
        const boost::system::error_code& err, 
        const boost::asio::ip::tcp::endpoint& endpoint
    );

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
    std::shared_ptr<boost::asio::ssl::context> m_sslContext;

    boost::asio::io_context::strand m_strand;
    boost::asio::ssl::stream<Stream> m_stream;
    
    bool m_isWriting { false };
    boost::asio::streambuf m_inbox;
    Buffers m_outbox;
};

template<class Stream>
Connection<Stream>::Connection(std::weak_ptr<Client> client
    , std::shared_ptr<boost::asio::io_context> context
    , std::shared_ptr<boost::asio::ssl::context> sslContext
) 
    : m_client { client }
    , m_io { context }
    , m_sslContext { sslContext }
    , m_strand { *context }
    , m_stream { *context, *sslContext }
{
}

template<class Stream>
Connection<Stream>::~Connection() {
    m_logger.Write(LogType::info, "Connection closed through destructor\n");
}

template<class Stream>
void Connection<Stream>::Connect(std::string_view path, std::string_view port) {
    using std::placeholders::_1;
    using std::placeholders::_2;
    
    m_stream.set_verify_mode(boost::asio::ssl::verify_peer
        | boost::asio::ssl::verify_fail_if_no_peer_cert
    );
    // m_stream.set_verify_callback(std::bind(&Connection::VerifyCertificate, this->shared_from_this(), _1, _2));
    m_stream.set_verify_callback([weak = this->weak_from_this()](
        bool preverified
        , boost::asio::ssl::verify_context& ctx) 
    {
        // In this example we will simply print the certificate's subject name.
        char name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), name, 256);
        
        if (auto shared = weak.lock(); shared) {
            shared->m_logger.Write(LogType::info, "Verifying", name, "\n");
        }

        return preverified;
    });

    asio::ip::tcp::resolver resolver(*m_io);
    const auto endpoints = resolver.resolve(path, port);
    if (endpoints.empty()) {
        this->Close();
    }
    else {
        boost::asio::async_connect(
            m_stream.lowest_layer(),
            endpoints, 
            std::bind(&Connection::OnConnect, this->shared_from_this(), _1, _2)
        );
    }
}

template<class Stream>
void Connection<Stream>::Handshake() {
    m_stream.async_handshake(boost::asio::ssl::stream_base::client,
        [self = this->shared_from_this()] (const boost::system::error_code& error) {
            if (!error) {
                if(auto model = self->m_client.lock(); model) {
                    // update state
                    model->SetState(Client::State::RECEIVE_ACK);
                }
                // start waiting incoming calls
                self->Read();
            }
            else {
                self->m_logger.Write(LogType::error, "Handshake failed:", error.message(), "\n");
            }
        }
    );
}

template<class Stream>
void Connection<Stream>::OnConnect(
    const boost::system::error_code& error, 
    const boost::asio::ip::tcp::endpoint& endpoint
) {
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
        this->Handshake();
    }
}

template<class Stream>
void Connection<Stream>::Close() {
    boost::asio::post(m_strand, [self = this->shared_from_this()]() {
        boost::system::error_code error;
        self->m_stream.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, error);
        if (error) {
            self->m_logger.Write(LogType::error, 
                "Connection's socket called shutdown with error: ", error.message(), '\n'
            );
            error.clear();
        }
        
        self->m_stream.lowest_layer().close(error);
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
        m_logger.Write(LogType::info, m_stream.lowest_layer().remote_endpoint(error), ':', received, '\n');

        if(auto model = m_client.lock(); model) {
            /* if it's ACK, we resolve the State in synchronous way, otherwise resolve asyncronously */
            model->HandleMessage(std::move(incomingResponse));
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

} // namespace client

#endif // CLIENT_CONNECTION_HPP__