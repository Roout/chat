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
        JOIN_SERVER,
        LEAVE_SERVER,
        // chat
        CHAT_MESSAGE
    };
}

#endif // REQUEST_TYPE_HPP