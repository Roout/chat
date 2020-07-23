#ifndef REQUEST_TYPE_HPP
#define REQUEST_TYPE_HPP

namespace Requests {
    /**
     * All possible types of requeses used in communication 
     * between client and server.  
     */
    enum class RequestType : int {
        // default or error
        UNDEFINED,
        // chatroom
        LEAVE_CHATROOM,
        JOIN_CHATROOM,
        ABOUT_CHATROOM,
        CREATE_CHATROOM,
        LIST_CHATROOM,
        // server
        AUTHORIZE,
        POST,
        // chat
        CHAT_MESSAGE,

        COUNT
    };
}

#endif // REQUEST_TYPE_HPP