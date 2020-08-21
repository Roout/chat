#ifndef REQUEST_TESTS_HPP
#define REQUEST_TESTS_HPP 

#include "gtest/gtest.h"
#include "Message.hpp"
#include "Utility.hpp"

TEST(Request, ParseLeaveChatroomRequest) {
    
}

TEST(Request, ParseJoinChatroomRequest) {
    
}

TEST(Request, ParseCreateChatroomRequest) {
    
}

TEST(Request, ParseListChatroomRequest) {
    
}

TEST(RequestTest, SerializeRequest) {
    Internal::Request request {};
   
}

TEST(ResponseTest, SerializeResponse) {
    Internal::Response response {};
   
}

TEST(ChatTest, SerializeChatMessage) {
    Internal::Request chat {};
   
}

TEST(ChatTest, ParseChatMessage) {
    std::string json = "";
   
}

#endif // REQUEST_TESTS_HPP