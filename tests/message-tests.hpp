#ifndef REQUEST_TESTS_HPP
#define REQUEST_TESTS_HPP 

#include "gtest/gtest.h"
#include "Message.hpp"
#include "Utility.hpp"
#include <string>

TEST(RequestTest, ParseLeaveChatroomRequest) {
    const std::string requestStr = R"(
       {
            "tag": "request",
            "protocol": "RRP",
            "type": "leave-chatroom",
            "timestamp": 344678435900,
            "timeout": 30
        }
    )";  
    
    auto message = Internal::Read(requestStr);
    auto request = dynamic_cast<Internal::Request*>(message.get());

    EXPECT_NE(request, nullptr);
    EXPECT_EQ(request->m_timeout, 30);
    EXPECT_EQ(request->m_type, Internal::QueryType::LEAVE_CHATROOM);
    EXPECT_EQ(request->m_timestamp, 344678435900LL);
    EXPECT_EQ(request->m_attachment, std::string(""));
}

TEST(RequestTest, ParseJoinChatroomRequest) {
    const std::string requestStr = R"(
       {
            "tag": "request",
            "protocol": "RRP",
            "type": "join-chatroom",
            "timestamp": 344678435232,
            "timeout": 30,
            "body": {
                "user": {
                    "name":"nickname"
                },
                "chatroom": {
                    "id":12
                }
            }
        }
    )";  
   
    auto message = Internal::Read(requestStr);
    auto request = dynamic_cast<Internal::Request*>(message.get());

    EXPECT_NE(request, nullptr);
    EXPECT_EQ(request->m_timeout, 30);
    EXPECT_EQ(request->m_type, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(request->m_timestamp, 344678435232LL);
    EXPECT_FALSE(request->m_attachment.empty());
}

TEST(RequestTest, ParseCreateChatroomRequest) {
    const std::string requestStr = R"(
       {
            "tag": "request",
            "protocol": "RRP",
            "type": "create-chatroom",
            "timestamp": 344678435232,
            "timeout": 30,
            "body":{
                "user": {
                    "name":"nickname"
                },
                "chatroom": {
                    "name": "some chatroom name"
                }
            }
        }
    )";  
   
    auto message = Internal::Read(requestStr);
    auto request = dynamic_cast<Internal::Request*>(message.get());

    EXPECT_NE(request, nullptr);
    EXPECT_EQ(request->m_timeout, 30);
    EXPECT_EQ(request->m_type, Internal::QueryType::CREATE_CHATROOM);
    EXPECT_EQ(request->m_timestamp, 344678435232LL);
    EXPECT_FALSE(request->m_attachment.empty());
}

TEST(RequestTest, ParseListChatroomRequest) {
    const std::string requestStr = R"(
       {
            "tag": "request",
            "protocol": "RRP",
            "type": "list-chatroom",
            "timestamp": 344678435266,
            "timeout": 30
        }
    )";  
   
    auto message = Internal::Read(requestStr);
    auto request = dynamic_cast<Internal::Request*>(message.get());

    EXPECT_NE(request, nullptr);
    EXPECT_EQ(request->m_timeout, 30);
    EXPECT_EQ(request->m_type, Internal::QueryType::LIST_CHATROOM);
    EXPECT_EQ(request->m_timestamp, 344678435266LL);
    EXPECT_TRUE(request->m_attachment.empty());
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

// ============ Chat ============ //

TEST(ChatTest, BuildChatMessage) {
    Internal::Request chat {};
   
}

TEST(ChatTest, ParseChatMessage) {
    std::string json = "";
   
}

#endif // REQUEST_TESTS_HPP