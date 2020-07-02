#pragma once
#include <memory>
#include <string>
#include <optional>
#include <queue>
#include <boost/asio.hpp>

/**
 * !Both executed only in the thread where run() is called. [documentation]
 * 
 * Post - 	non-blocking call. Function can't be executed 
 * 			inside the function where it was posted.
 * 
 * Dispatch -  may block the function where it was dispatched.
 * 
 * The dispatch() function can be invoked from the current
 * worker thread, while the post() function has to wait until the handler of the worker is complete
 * before it can be invoked. In other words, the dispatch() function's events can be executed from the
 * current worker thread even if there are other pending events queued up, while the post() function's
 * events have to wait until the handler completes the execution before being allowed to be executed.
 * 
 * io_context::poll() - same as run() but execute only already READY handles => so no blocking used
 * 
 * io_context::run() - 	blocks execution (wait for and execute for handles that are NOT ready yet), i.e.
 *  					blocks until all work has finished.
 * 
 * strand - A strand is defined as a strictly sequential invocation of event handlers 
 *          (i.e. no concurrent invocation). Use of strands allows execution of code 
 *          in a multithreaded program without the need for explicit locking (e.g. using mutexes). 
 */

namespace asio = boost::asio;


class Server;

/// TODO: Prevent several async_write/read in/from one direction!
///
class Session final : public std::enable_shared_from_this<Session> {
public:
    
    Session( 
        asio::ip::tcp::socket && socket, 
        Server * const server 
    );

    ~Session() {
        this->Close();
    };

    /**
     * Send @text to remote connection 
     * ASSUME 
     *  - that size(text) <= size(m_buffer)
     *  - send message only once
     */
    void Send(std::string text);

    /**
     * Read @text from the remote connection
     */
    void Read();

    bool IsClosed() const noexcept {
        return m_isClosed;
    }
    /**
     * Shutdown Session and close the socket  
     */
    void Close();

private:
    
    void WriteSomeHandler(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );

    void ReadSomeHandler(
        const boost::system::error_code& error, 
        size_t transferredBytes
    );
private:

    /**
     * It's a socket connected to the server. 
     */
    asio::ip::tcp::socket   m_socket;

    /// TODO: remove server pointer. 
    /// Session should communicate with server via protocol and preefined commands
    Server * const m_server { nullptr };

    asio::io_context::strand m_strand;
    /**
     * A buffer used for outcoming information.
     */
    std::string m_buffer;
    /**
     * A buffer used for incoming information.
     */
    asio::streambuf m_streamBuffer;
    /**
     * Already transferred bytes 
     */
    size_t m_transferred { 0 };

    bool m_isClosed { false };
};

class Server final {
public:
    
    Server(std::shared_ptr<asio::io_context>, std::uint16_t port);

    void Start();

    void Shutdown();

    void Broadcast(const std::string& text);
    
    void BroadcastEveryoneExcept(const std::string& text, const Session*);

    void RemoveSession(const Session * s);

private:
    friend class Session;
    /**
     * Context is being passed by a pointer because we don't need to know
     * where and how it's being run. We just ditch this responsibility to someone else.
     */
    std::shared_ptr<asio::io_context>       m_context;
    asio::ip::tcp::acceptor                 m_acceptor;
    /**
     * Socket  
     */
    std::optional<asio::ip::tcp::socket>    m_socket;
    /**
     * Use shared_ptr instead of simple instance because 
     * Session is derived from std::enamble_shared_from_this so
     * we need to have allocated control block for ::shared_from_this
     */
    std::vector<std::shared_ptr<Session>>   m_sessions;
};

