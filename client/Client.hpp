#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <memory>
#include <string_view>
#include <string>
#include <mutex>

#include "Message.hpp"
#include "Utility.hpp"

#include <boost/asio.hpp>

#ifndef UNIT_TESTS
using Stream_t = boost::asio::ip::tcp::socket;
#else
using Stream_t = MockAsyncStream;
#endif

namespace client {
    template<class Stream>
    class Connection;

    using Connection_t = Connection<Stream_t>;
}


class Client final 
    : public std::enable_shared_from_this<Client> {
public:
    enum class State {
        /**
         * This state indicates that 
         * client is closed or disconnected.  
         */
        CLOSED,
        /**
         * This state indicates that client is connected
         * but hasn't sent an ACK request to server
         */
        CONNECTED, 
        /**
         * This state indicated that 
         * client has sent SYN handshake to the 
         * server and is waiting ACK response.
         */
        WAIT_ACK,
        /**
         * This state indicates that the client
         * has already received ACK from the 
         * server and server was acknowledged
         * by the client.
         */
        RECEIVE_ACK,
        COUNT
    };

public:

    Client(std::shared_ptr<boost::asio::io_context> io);

    /**
     * Close connection if it's haven't been terminated yet. 
     */
    ~Client();

    /**
     * Trying to connect to the remote host
     * 
     * @param path
     *  It's ip address of the remote host
     * @param port
     *  It's a port on which a remote host is listening  
     */
    void Connect(std::string_view path, std::string_view port);

    void CloseConnection();

    void HandleMessage(Internal::Response&&);

    Internal::Request CreateSynchronizeRequest() const;
    
    /**
     * Write message to the remote peer
     * @param text
     *  Message in string format 
     */
    void Write(std::string text);
    
    void SetState(State state) noexcept;

    State GetState() const noexcept;

    Internal::Response GetLastResponse() const noexcept;

private:
    std::shared_ptr<boost::asio::io_context> m_io { nullptr };
    std::shared_ptr<client::Connection_t> m_connection { nullptr };

    // TODO: move to some other intermediate type between client and GUI
    Internal::Response m_response;
    // TODO: maybe use atomic
    State m_state { State::CLOSED };
    mutable std::mutex m_mutex;
};

#endif // CHAT_CLIENT_HPP