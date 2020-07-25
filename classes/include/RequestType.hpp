#ifndef REQUEST_TYPE_HPP
#define REQUEST_TYPE_HPP

namespace Requests {
    /**
     * All possible types of requeses used in communication 
     * between client and server.  
     */
    enum class RequestType : int {
        // default or error
        UNDEFINED           = 0x0001,
        // chatroom
        LEAVE_CHATROOM      = 0x0002,
        JOIN_CHATROOM       = 0x0004,
        ABOUT_CHATROOM      = 0x0008,
        CREATE_CHATROOM     = 0x0010,
        LIST_CHATROOM       = 0x0020,
        // server
        AUTHORIZE           = 0x0040,
        POST                = 0x0080,
        // chat
        CHAT_MESSAGE        = 0x0100,

        COUNT
    };
}

#endif // REQUEST_TYPE_HPP