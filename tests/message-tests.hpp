#ifndef REQUEST_TESTS_HPP
#define REQUEST_TESTS_HPP 

#include "gtest/gtest.h"
#include <string>
#include "Message.hpp"
#include "Utility.hpp"

TEST(RequestTest, ParseLeaveChatroomRequest) {
    const std::string requestStr = R"(
       {
            "query": "leave-chatroom",
            "timestamp": 344678435900,
            "timeout": 30
        }
    )";  
    
    Internal::Request request{};
    request.Read(requestStr);

    EXPECT_EQ(request.m_timeout, 30);
    EXPECT_EQ(request.m_query, Internal::QueryType::LEAVE_CHATROOM);
    EXPECT_EQ(request.m_timestamp, 344678435900LL);
    EXPECT_TRUE(request.m_attachment.empty());
}

TEST(RequestTest, ParseJoinChatroomRequest) {
    const std::string requestStr = R"(
       {
            "query": "join-chatroom",
            "timestamp": 344678435232,
            "timeout": 30,
            "attachment": {
                "user": {
                    "name":"nickname"
                },
                "chatroom": {
                    "id":12
                }
            }
        }
    )";  
   
    Internal::Request request{};
    request.Read(requestStr);

    EXPECT_EQ(request.m_timeout, 30);
    EXPECT_EQ(request.m_query, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(request.m_timestamp, 344678435232LL);
    EXPECT_FALSE(request.m_attachment.empty());
}

TEST(RequestTest, ParseCreateChatroomRequest) {
    const std::string requestStr = R"(
       {
            "query": "create-chatroom",
            "timestamp": 344678435232,
            "timeout": 30,
            "attachment":{
                "user": {
                    "name":"nickname"
                },
                "chatroom": {
                    "name": "some chatroom name"
                }
            }
        }
    )";  
   
    Internal::Request request{};
    request.Read(requestStr);

    EXPECT_EQ(request.m_timeout, 30);
    EXPECT_EQ(request.m_query, Internal::QueryType::CREATE_CHATROOM);
    EXPECT_EQ(request.m_timestamp, 344678435232LL);
    EXPECT_FALSE(request.m_attachment.empty());
}

TEST(RequestTest, ParseListChatroomRequest) {
    const std::string requestStr = R"(
       {
            "query": "list-chatroom",
            "timestamp": 344678435266,
            "timeout": 30
        }
    )";  
   
    Internal::Request request{};
    request.Read(requestStr);
   
    EXPECT_EQ(request.m_timeout, 30);
    EXPECT_EQ(request.m_query, Internal::QueryType::LIST_CHATROOM);
    EXPECT_EQ(request.m_timestamp, 344678435266LL);
    EXPECT_TRUE(request.m_attachment.empty());
}

TEST(RequestTest, ParseChatMessageRequest) {
    const std::string requestStr = R"(
       {
            "query": "chat-message",
            "timestamp": 344678435266,
            "timeout": 30,
            "attachment": {
                "message": "Hello, it's a client side!"
            }
        }
    )";  
   
    Internal::Request request{};
    request.Read(requestStr);
   
    EXPECT_EQ(request.m_timeout, 30);
    EXPECT_EQ(request.m_query, Internal::QueryType::CHAT_MESSAGE);
    EXPECT_EQ(request.m_timestamp, 344678435266LL);
    EXPECT_FALSE(request.m_attachment.empty());
}

// ========== Response ========= //
TEST(ResponseTest, ParseLeaveChatroomResponse) {
    
}

TEST(ResponseTest, ParseJoinChatroomResponse) {
    
}

TEST(ResponseTest, ParseCreateChatroomResponse) {
    
}

TEST(ResponseTest, ParseListChatroomResponse) {
    
}


TEST(ResponseTest, BuildLeaveChatroomResponse) {
    
}

TEST(ResponseTest, BuildJoinChatroomResponse) {
    
}

TEST(ResponseTest, BuildCreateChatroomResponse) {
    
}

TEST(ResponseTest, BuildListChatroomResponse) {
    
}

#endif // REQUEST_TESTS_HPP