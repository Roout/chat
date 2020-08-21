#ifndef QUERY_TYPE_HPP
#define QUERY_TYPE_HPP

namespace Internal {
    /**
     * All possible types of requests/response used in 
     * communication between client and server.
     */
    enum class QueryType {
        // default or error
        UNDEFINED,

        // two way handshake
        SYN,
        ACK,

        LEAVE_CHATROOM,
        JOIN_CHATROOM,
        CREATE_CHATROOM,
        LIST_CHATROOM,

        COUNT
    };
}

#endif // QUERY_TYPE_HPP