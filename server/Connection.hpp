#ifndef NET_CONNECTION_HPP
#define NET_CONNECTION_HPP

#include <memory>
#include <cstddef>
#include <cstdint>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "DoubleBuffer.hpp"
#include "Log.hpp"

namespace rt {
    class RequestQueue;
}

class Session;

namespace net {

namespace asio = boost::asio;

class Connection final : public std::enable_shared_from_this<Connection> {
public:

    using TimerCallback = std::function<void(const boost::system::error_code&)>;

    Connection(
        std::uint64_t id
        , asio::ip::tcp::socket&& socket
        , asio::io_context * const context
        , asio::ssl::context * const sslContext
        , std::shared_ptr<rt::RequestQueue> incommingRequests
    );

    ~Connection();

    void AddSubscriber(std::weak_ptr<Session> session);

    void Handshake();

    /**
     * Read with timeout
     */
    void Read(std::uint64_t ms, TimerCallback&&);

    /**
     * Write @text to remote connection.
     * @note
     *  Invoke private Write() overload via asio::post() through strand
     */
    void Write(std::string&& text);

    /**
     * Shutdown Session and close the socket  
     */
    void Close();

    bool IsClosed() const noexcept {
        return m_state == State::CLOSED;
    };

private:
    
    void Publish();

    /**
     * Read data from the remote connection.
     * At first it's invoked at `Read(ms)` completion handler.
     * Otherwise it can be invoked within `on read` completion handler. 
     * This prevents a concurrent execution of the read operation on the same socket.
     */
    void Read();
    
    /**
     * This method calls async I/O write operation.
     */
    void Write();

    void WriteSomeHandler(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    void ReadSomeHandler(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    );

    /**
     * Logging the custom message
     */
    template<class ...Args>
    void AddLog(const LogType ty, Args&& ...args);

private:
    enum class State: std::uint8_t {
        /**
         * Default state: any other state except:
         * `State::WRITING` and `State::CLOSED`
         */
        DEFAULT,
        /**
         * This state is set when the connection 
         * has ongoing writing operation.
         * Doesn't exclude the chance that it may
         * reading.
         */
        WRITING,
        /**
         * This is a state when a connection between peers 
         * was terminated/closed or hasn't even started. 
         * Reason isn't important.
         */
        CLOSED
    };

    std::shared_ptr<Log> m_logger{ nullptr };

    /**
     * It's a socket connected to the remote peer. 
     */
    asio::ssl::stream<asio::ip::tcp::socket> m_socket;

    asio::io_context::strand m_strand;

    /**
     * This is a timer used to set up deadline for the client
     * and invoke handler for the expired request.
     * Now it's used to wait for the synchronization request.  
     */
    asio::deadline_timer m_timer;

    std::shared_ptr<rt::RequestQueue> m_incommingRequests{ nullptr };

    std::weak_ptr<Session> m_subscriber{};

    Buffers m_outbox;

    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_inbox;

    /**
     * Indicate a socket state
     */
    State m_state { State::DEFAULT };

    /**
     * Define hom long the connection can wait for request 
     * from the client. Time is in milliseconds.
     */
    std::uint64_t m_timeout { 128 };

    const std::uint64_t m_id { 0 };
};

template<class ...Args>
void Connection::AddLog(const LogType ty, Args&& ...args) {
    m_logger->Write(ty, std::forward<Args>(args)...);
}

} // namespace net

#endif // NET_CONNECTION_HPP