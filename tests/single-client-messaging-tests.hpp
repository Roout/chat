#ifndef SESSION_TESTS_HPP
#define SESSION_TESTS_HPP

#include "gtest/gtest.h"
#include "Server.hpp"
#include "Session.hpp"
#include "Client.hpp"
#include "Utility.hpp"
#include "QueryType.hpp"

#include <memory>
#include <thread>
#include <regex>
#include <cstddef>

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

class BasicInteractionTest : public ::testing::Test {
protected:

    void SetUp() override {
        m_context = std::make_shared<boost::asio::io_context>();
        m_timer = std::make_shared<boost::asio::deadline_timer>(*m_context);

        // Create server and start accepting connections
        m_server = std::make_unique<Server> (m_context, 15001);
        m_server->Start();
        // Create client and connect to server
        m_client = std::make_unique<Client> (m_context);
        m_client->Connect("127.0.0.1", 15001);

        for(int i = 0; i < 2; i++) {
            m_threads.emplace_back([io = m_context]() {
                for (;;) {
                    try {
                        io->run();
                        break; // run() exited normally
                    }
                    catch (...) {
                        // Deal with exception as appropriate.
                    }
                }
            });
        }
    }

    void TearDown() override {
        m_server->Shutdown();
        m_context->stop();

        for(auto& t: m_threads) {
            if(t.joinable()) {
                t.join();
            }
        }
    }   

    void WaitFor(std::size_t ms) {
        m_timer->expires_from_now(boost::posix_time::millisec(ms));
        m_timer->wait();
    }
   
protected:
    std::unique_ptr<Server> m_server {};
    std::unique_ptr<Client> m_client {};
    std::shared_ptr<boost::asio::io_context> m_context {};
    std::vector<std::thread> m_threads {};
    std::shared_ptr<boost::asio::deadline_timer> m_timer;
    const std::size_t m_waitAckTimeout { 128 };
};

/**
 * Client connect ->
 * Server accept -> Server waits for SYN
 * Client send SYN ->
 * Server recieve SYN and send ACK ->
 * Client recieve ACK 
 */
TEST_F(BasicInteractionTest, OnlyFixureSetup) {
    // prepare (done in ::SetUp method)
    // execute (done in ::SetUp method)
    this->WaitFor(m_waitAckTimeout);
    // test
    ASSERT_TRUE(m_client->IsAcknowleged())
        << "Client hasn't been acknowleged";
    EXPECT_EQ(m_client->GetGUI().GetResponse().m_type, Internal::QueryType::ACK);
}

/**
 *  Request:
 *  {	
 *      "protocol": "SRR",
 *      "type": "list-chatrooms",
 *      "timestamp": 344678435266,
 *      "timeout": 30
 *  } 
 */
TEST_F(BasicInteractionTest, ChatroomListRequest) {
    /// 0. Confirm Handshake
    this->WaitFor(m_waitAckTimeout);
    EXPECT_TRUE(m_client->IsAcknowleged())
        << "Client hasn't been acknowleged";
    EXPECT_EQ(m_client->GetGUI().GetResponse().m_type, Internal::QueryType::ACK);

    /// 1. Server creates chatrooms
    struct MockChatroom {
        std::string name;
        std::size_t id;
        std::size_t users { 0 };

        bool operator==(const MockChatroom& rhs) const noexcept {
            return id == rhs.id && users == rhs.users && name == rhs.name;
        }
    };
    std::array<MockChatroom, 2> expectedRooms;
    expectedRooms[0].name = "WoW 3.3.5a";
    expectedRooms[0].id = m_server->CreateChatroom(expectedRooms[0].name);
    expectedRooms[1].name = "Dota 2";
    expectedRooms[1].id = m_server->CreateChatroom(expectedRooms[1].name);
 
    /// 2. Request chatroom list
    Internal::Request listRequest{};
    listRequest.m_type = Internal::QueryType::LIST_CHATROOM;
    listRequest.m_timestamp = Utils::GetTimestamp();
    listRequest.m_timeout = 30;
    std::string serialized {};
    listRequest.Write(serialized);
    // send request to server
    m_client->Write(std::move(serialized));   
    this->WaitFor(listRequest.m_timeout);

    /// 3. build a chatroom list
    const auto& jsonString = m_client->GetGUI().GetResponse().m_attachment;

    rapidjson::Document reader;
    reader.Parse(jsonString.c_str());
    const auto& chatrooms = reader["chatrooms"].GetArray();
    // 
    EXPECT_EQ(chatrooms.Size(), expectedRooms.size());
    std::array<MockChatroom, 2> recievedRooms;
    std::size_t i { 0 };
    for(const auto& roomObj: chatrooms) {
        recievedRooms[i].id = roomObj["id"].GetUint64(); 
        recievedRooms[i].name = roomObj["name"].GetString();
        recievedRooms[i].users = roomObj["users"].GetUint64(); 
        i++;
    }
    /// 4. Compare expected result and recieved response
    EXPECT_EQ(m_client->GetGUI().GetResponse().m_type, Internal::QueryType::LIST_CHATROOM);
    for(std::size_t i = 0; i < expectedRooms.size(); i++) {
        EXPECT_EQ(recievedRooms[i], expectedRooms[i]);
    }
}

// Internal::QueryType::JOIN_CHATROOM for already existing chatrooms
TEST_F(BasicInteractionTest, JoinChatroomRequest) {
    /// #0. Cinfirm Handshake
    this->WaitFor(m_waitAckTimeout);
    ASSERT_TRUE(m_client->IsAcknowleged())
        << "Client hasn't been acknowleged";
    EXPECT_EQ(m_client->GetGUI().GetResponse().m_type, Internal::QueryType::ACK);

    
    /// #1 Create chatrooms
    m_server->CreateChatroom("Test chatroom #1"); 
    m_server->CreateChatroom("Test chatroom #2"); 
    m_server->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    const auto desiredId = m_server->CreateChatroom(desiredChatroomName); 
    
    // #2 Join Chatroom
    Internal::Request request{};
    request.m_type = Internal::QueryType::JOIN_CHATROOM;
    request.m_timeout = 30;
    request.m_timestamp = Utils::GetTimestamp();
    // Set up body
    rapidjson::Document doc(rapidjson::kObjectType);
    auto& alloc = doc.GetAllocator();
    doc.AddMember("user", rapidjson::Value(rapidjson::kObjectType), alloc);
    doc["user"].AddMember("name", rapidjson::Value("random username", alloc), alloc);
    
    doc.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType) , alloc);
    doc["chatroom"].AddMember("id", desiredId, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
    doc.Accept(writer);
    request.m_attachment = std::string { buffer.GetString(), buffer.GetLength() }; 

    // send request
    std::string serialized;
    request.Write(serialized);
    m_client->Write(std::move(serialized));   
    // wait 25ms for answer
    this->WaitFor(request.m_timeout);

    // #3 Confirm that we've joined
    const auto& joinReply = m_client->GetGUI().GetResponse();

    EXPECT_EQ(joinReply.m_type, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(joinReply.m_attachment.empty());
}

// Internal::QueryType::CREATE_CHATROOM create and join new chatroom.
TEST_F(BasicInteractionTest, CreateChatroomRequest) {
    /// #0. Cinfirm Handshake
    this->WaitFor(m_waitAckTimeout);
    ASSERT_TRUE(m_client->IsAcknowleged())
        << "Client hasn't been acknowleged";
    EXPECT_EQ(m_client->GetGUI().GetResponse().m_type, Internal::QueryType::ACK);

    /// #1 Create chatrooms
    m_server->CreateChatroom("Test chatroom #1"); 
    m_server->CreateChatroom("Test chatroom #2"); 
    m_server->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    
    // #2 Join Chatroom
    Internal::Request request{};
    request.m_type = Internal::QueryType::CREATE_CHATROOM;
    request.m_timeout = 30;
    request.m_timestamp = Utils::GetTimestamp();
    // Set up body
    rapidjson::Document doc(rapidjson::kObjectType);
    auto& alloc = doc.GetAllocator();
    doc.AddMember("user", rapidjson::Value(rapidjson::kObjectType), alloc);
    doc["user"].AddMember("name", rapidjson::Value("random username", alloc), alloc);
    
    doc.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType) , alloc);
    doc["chatroom"].AddMember("name", rapidjson::Value(desiredChatroomName.c_str(), alloc), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
    doc.Accept(writer);
    request.m_attachment = std::string { buffer.GetString(), buffer.GetLength() }; 

    // send request
    std::string serialized;
    request.Write(serialized);
    m_client->Write(std::move(serialized));   
    // wait 30ms for answer
    this->WaitFor(request.m_timeout);

    // #3 Confirm that we've joined
    const auto& joinReply = m_client->GetGUI().GetResponse();

    EXPECT_EQ(joinReply.m_type, Internal::QueryType::CREATE_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(!joinReply.m_attachment.empty());

    // #4 Parse attachment and extract room ID
    rapidjson::Document reader;
    reader.Parse(joinReply.m_attachment.c_str());
    const auto id = reader["chatroom"]["id"].GetUint64();
    EXPECT_NE(id, chat::Chatroom::NO_ROOM);

    // #5 Confirm that the server has one room with this ID
    const auto expectedUsersCount { 1 }; // only this client!
    const auto& currentChatroomList = m_server->GetChatroomList();
    const std::regex rx { ".+\"id\":[ ]*(\\d+).+\"name\":[ ]*\"(.*)\".+\"users\":[ ]*(\\d+).*" };
    std::smatch match;
    auto foundMatchRoom { false };
    for(const auto& room: currentChatroomList) {
        EXPECT_TRUE(std::regex_match(room, match, rx));

        if(std::stoi(match[1].str()) == id) {
            foundMatchRoom = true;
            EXPECT_EQ(desiredChatroomName, match[2].str());
            EXPECT_EQ(expectedUsersCount, std::stoi(match[3].str()));
        }
    }
    EXPECT_TRUE(foundMatchRoom) << "Can't find a room with ID from server's response";

}


// Internal::QueryType::LEAVE_CHATROOM 
TEST_F(BasicInteractionTest, LeaveChatroomRequest) {
    /// #0. Cinfirm Handshake
    this->WaitFor(m_waitAckTimeout);
    ASSERT_TRUE(m_client->IsAcknowleged())
        << "Client hasn't been acknowleged";
    EXPECT_EQ(m_client->GetGUI().GetResponse().m_type, Internal::QueryType::ACK);

    /// #1 Create chatrooms
    m_server->CreateChatroom("Test chatroom #1"); 
    m_server->CreateChatroom("Test chatroom #2"); 
    m_server->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    const auto desiredId = m_server->CreateChatroom(desiredChatroomName); 
    
    // #2 Join Chatroom
    Internal::Request request{};
    request.m_type = Internal::QueryType::JOIN_CHATROOM;
    request.m_timeout = 30;
    request.m_timestamp = Utils::GetTimestamp();
    // Set up body
    rapidjson::Document doc(rapidjson::kObjectType);
    auto& alloc = doc.GetAllocator();
    doc.AddMember("user", rapidjson::Value(rapidjson::kObjectType), alloc);
    doc["user"].AddMember("name", rapidjson::Value("random username", alloc), alloc);
    
    doc.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType) , alloc);
    doc["chatroom"].AddMember("id", desiredId, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
    doc.Accept(writer);
    request.m_attachment = std::string { buffer.GetString(), buffer.GetLength() }; 

    // send request
    std::string serialized;
    request.Write(serialized);
    m_client->Write(std::move(serialized));   
    // wait 25ms for answer
    this->WaitFor(request.m_timeout);

    // #3 Confirm that we've joined
    const auto& joinReply = m_client->GetGUI().GetResponse();

    EXPECT_EQ(joinReply.m_type, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(joinReply.m_attachment.empty());

    /// #4 Confirm that the server has one room with this ID and 1 user
    enum { ID, USERS, NAME };
    const auto roomTupleOpt = m_server->GetChatroomData(desiredId);
    const auto expectedUsersCount { 1 }; // only this client!
    EXPECT_NE(roomTupleOpt, std::nullopt);
    EXPECT_EQ(desiredChatroomName, std::get<NAME>(*roomTupleOpt));
    EXPECT_EQ(expectedUsersCount, std::get<USERS>(*roomTupleOpt));
    
    /// #5 Leave chatroom
    Internal::Request leaveRequest{};
    leaveRequest.m_type = Internal::QueryType::LEAVE_CHATROOM;
    leaveRequest.m_timeout = 30;
    leaveRequest.m_timestamp = Utils::GetTimestamp();
    // send request
    serialized.clear();
    leaveRequest.Write(serialized);
    m_client->Write(std::move(serialized));   

    // wait for answer
    this->WaitFor(leaveRequest.m_timeout);

    /// #6 Confirmn that we've left
    const auto afterLeaveRoomOpt = m_server->GetChatroomData(desiredId);
    EXPECT_EQ(afterLeaveRoomOpt, std::nullopt);
}

/** TODO:
 * Thread safety tests:
 * - [ ] Multiply clients trying to create the chatroom (maybe with the same name);
 * - [ ] 
 */

#endif // SESSION_TESTS_HPP