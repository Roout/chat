#include "gtest/gtest.h"
#include "Request.hpp"
#include "Utility.hpp"

TEST(Request, ParseLeaveChatroomRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::LEAVE_CHATROOM)) // command
        + Requests::DELIMETER + "0"  // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + "" // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::LEAVE_CHATROOM);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string("Roster557"));
    ASSERT_TRUE(request.GetBody().empty());
}

TEST(Request, ParseJoinChatroomRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::JOIN_CHATROOM)) // command
        + Requests::DELIMETER + "0"  // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + "" // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::JOIN_CHATROOM);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string("Roster557"));
    ASSERT_TRUE(request.GetBody().empty());
}

TEST(Request, ParseAboutChatroomRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::ABOUT_CHATROOM)) // command
        + Requests::DELIMETER + "134"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + "6:some other info"   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::ABOUT_CHATROOM);
    ASSERT_EQ(request.GetChatroom(), 134);
    ASSERT_EQ(request.GetName(), std::string{"Roster557"});
    ASSERT_EQ(request.GetBody(), std::string{"6:some other info"});
}

TEST(Request, ParseCreateChatroomRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::CREATE_CHATROOM)) // command
        + Requests::DELIMETER + "0"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + "Only for girls!"   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::CREATE_CHATROOM);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string{"Roster557"});
    ASSERT_EQ(request.GetBody(), std::string{"Only for girls!"});
}

TEST(Request, ParseListChatroomRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::LIST_CHATROOM)) // command
        + Requests::DELIMETER + "0"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + ""   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::LIST_CHATROOM);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string{"Roster557"});
    ASSERT_EQ(request.GetBody(), std::string{""});
}

TEST(Request, ParseJoinServerRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::JOIN_SERVER)) // command
        + Requests::DELIMETER + "0"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + ""   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::JOIN_SERVER);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string{"Roster557"});
    ASSERT_EQ(request.GetBody(), std::string{""});
}

TEST(Request, ParseLeaveServerRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::LEAVE_SERVER)) // command
        + Requests::DELIMETER + "0"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + ""   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::LEAVE_SERVER);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string{"Roster557"});
    ASSERT_EQ(request.GetBody(), std::string{""});
}

TEST(Request, ParseChatMessageRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::LEAVE_SERVER)) // command
        + Requests::DELIMETER + "0"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + ""   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);
    // check
    ASSERT_EQ(request.GetType(), Requests::RequestType::LEAVE_SERVER);
    ASSERT_EQ(request.GetChatroom(), 0);
    ASSERT_EQ(request.GetName(), std::string{"Roster557"});
    ASSERT_EQ(request.GetBody(), std::string{""});
}

TEST(Request, SerializeRequest) {
    Requests::Request request;
    // form test
    const std::string frame = 
        std::to_string(Util::EnumCast(Requests::RequestType::ABOUT_CHATROOM)) // command
        + Requests::DELIMETER + "134"       // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + "6:some other info"   // body
        + Requests::REQUEST_DELIMETER; 
    // parse
    request.Parse(frame);

    const auto serialized { request.Serialize() };

    ASSERT_EQ(serialized, frame);
}