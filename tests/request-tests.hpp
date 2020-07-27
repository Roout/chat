#include "gtest/gtest.h"
#include "Request.hpp"
#include "Utility.hpp"

TEST(Request, ParseLeaveChatroomRequest) {
    
}

TEST(Request, ParseJoinChatroomRequest) {
    
}

TEST(Request, ParseAboutChatroomRequest) {
  
}

TEST(Request, ParseCreateChatroomRequest) {
    
}

TEST(Request, ParseListChatroomRequest) {
    
}

TEST(Request, ParseAuthorizeRequest) {
    Requests::Request before{}, after{};
    // form test
    before.SetType(Requests::RequestType::AUTHORIZE);
    before.SetStage(IStage::State::UNAUTHORIZED);
    before.SetName("Roster557");
    // parse
    const auto serialized { before.Serialize() };
    const auto result = after.Parse(serialized);
    // check
    const auto successCode { 0 };
    ASSERT_EQ(result,           successCode);
    ASSERT_EQ(after.GetType(),  Requests::RequestType::AUTHORIZE);
    ASSERT_EQ(after.GetStage(), IStage::State::UNAUTHORIZED);
    ASSERT_EQ(after.GetName(),  std::string{"Roster557"});
    ASSERT_EQ(after.GetBody(),  std::string{""});
}

TEST(Request, ParsePostRequest) {
    Requests::Request before{}, after{};
    // form test
    before.SetType(Requests::RequestType::POST);
    before.SetStage(IStage::State::UNAUTHORIZED);
    before.SetName("Roster557");
    before.SetBody("#1 Friends from Bruklin\n#2 Friends from Tokio");
    // parse
    const auto result = after.Parse(before.Serialize());
    // check
    const auto successCode { 0 };
    ASSERT_EQ(result,           successCode);
    ASSERT_EQ(after.GetType(),  Requests::RequestType::POST);
    ASSERT_EQ(after.GetStage(), IStage::State::UNAUTHORIZED);
    ASSERT_EQ(after.GetName(),  std::string{"Roster557"});
    ASSERT_EQ(after.GetBody(),  std::string{"#1 Friends from Bruklin\n#2 Friends from Tokio"});
}

TEST(Request, ParseChatMessageRequest) {
   
}

TEST(Request, SerializeRequest) {
    Requests::Request request {};
    request.SetType(Requests::RequestType::POST);
    request.SetStage(IStage::State::AUTHORIZED);
    request.SetCode(Requests::ErrorCode::SUCCESS);
    request.SetChatroom(43);
    request.SetName("Roster557");
    request.SetBody("6:some other info");
    // form test
    const std::string frame = 
        std::to_string(Utils::EnumCast(Requests::RequestType::POST)) // command
        + Requests::DELIMETER + std::to_string(Utils::EnumCast(IStage::State::AUTHORIZED))
        + Requests::DELIMETER + std::to_string(Utils::EnumCast(Requests::ErrorCode::SUCCESS))
        + Requests::DELIMETER + "43" // chatroom id
        + Requests::DELIMETER + "Roster557" // unique username
        + Requests::DELIMETER + "6:some other info" // body
        + Requests::REQUEST_DELIMETER; 

    const auto serialized { request.Serialize() };
    ASSERT_EQ(serialized, frame);

    Requests::Request duplicate{};
    const auto result { duplicate.Parse(frame) };
    ASSERT_EQ(duplicate.Serialize(), frame);
}