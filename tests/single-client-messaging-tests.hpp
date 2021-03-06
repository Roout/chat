#ifndef SESSION_TESTS_HPP
#define SESSION_TESTS_HPP

#include "gtest/gtest.h"

#include "Server.hpp"
#include "Session.hpp"
#include "Connection.hpp"
#include "Client.hpp"
#include "RoomService.hpp"

#include "Utility.hpp"
#include "QueryType.hpp"

#include <memory>
#include <thread>
#include <regex>
#include <cstddef>
#include <cstdint>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>

/// Helper functions:

template<class Encoding, class Allocator>
std::string Serialize(const rapidjson::GenericValue<Encoding, Allocator>& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return { buffer.GetString(), buffer.GetSize() };
}


class BasicInteractionTest : public ::testing::Test {
protected:

    void SetUp() override {
        m_context = std::make_shared<boost::asio::io_context>();
        m_sslContext = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
        m_timer = std::make_shared<boost::asio::deadline_timer>(*m_context);

        // Create server and start accepting connections
        m_server = std::make_unique<Server>(m_context, 15001);
        m_server->Start();
        // Create client and connect to server
        m_client = std::make_shared<Client>(m_context, m_sslContext);
        m_client->Connect("127.0.0.1", "15001");

        for (int i = 0; i < 4; i++) {
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
        for (auto& t: m_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }   

    void WaitFor(std::uint64_t ms) {
        m_timer->expires_from_now(boost::posix_time::millisec(ms));
        m_timer->wait();
    }
   
    /**
     * Wait for handshake and confirm that it was successfull
     */
    void ConfirmHandshake() {
        // blocks
        this->WaitFor(m_waitTimeout);
        // test
        ASSERT_TRUE(m_client->GetState() == Client::State::RECEIVE_ACK) << "Client hasn't been acknowleged";
    }

    /**
     * Create a join request and send it to server
     * 
     * @param id
     *  An ID of the chatroom will be joined
     * @param username 
     *  A name which will identify user at the chatroom  
     * @param client
     *  A client wished to join a chatroom
     */
    void JoinChatroom(std::uint64_t id, const std::string& username, Client& client) {
        Internal::Request request{};
        request.m_query = Internal::QueryType::JOIN_CHATROOM;
        request.m_timeout = m_waitTimeout;
        request.m_timestamp = Utils::GetTimestamp();
        // set up attachment
        rapidjson::Document doc(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        doc.AddMember("user", rapidjson::Value(rapidjson::kObjectType), alloc);
        doc["user"].AddMember("name", rapidjson::Value(username.c_str(), alloc), alloc);  
        doc.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType) , alloc);
        doc["chatroom"].AddMember("id", id, alloc);
        request.m_attachment = ::Serialize(doc); 
        // send request
        std::string serialized;
        request.Write(serialized);
        client.Write(std::move(serialized));   
        // wait 25ms for answer
        this->WaitFor(request.m_timeout);
    }

protected:
    std::shared_ptr<boost::asio::io_context> m_context {};
    std::shared_ptr<boost::asio::ssl::context> m_sslContext {};
    std::unique_ptr<Server> m_server {};
    std::shared_ptr<Client> m_client {};
    std::vector<std::thread> m_threads {};
    std::shared_ptr<boost::asio::deadline_timer> m_timer;
    const std::uint64_t m_waitTimeout { 128 };
};

/**
 * Client connect ->
 * Server accept -> Server waits for SYN
 * Client send SYN ->
 * Server recieve SYN and send ACK ->
 * Client recieve ACK 
 */
TEST_F(BasicInteractionTest, OnlyFixureSetup) {
    this->ConfirmHandshake();
}

TEST_F(BasicInteractionTest, ChatroomListRequest) {
    /// 0. Confirm Handshake
    this->ConfirmHandshake();

    /// 1. Server creates chatrooms
    struct MockChatroom {
        std::string name;
        std::uint64_t id;
        std::uint64_t users { 0 };

        bool operator==(const MockChatroom& rhs) const noexcept {
            return id == rhs.id && users == rhs.users && name == rhs.name;
        }

        bool operator<(const MockChatroom& rhs) const noexcept {
            return id < rhs.id;
        }
    };

    std::array<MockChatroom, 2> expectedRooms;
    expectedRooms[0].name = "WoW 3.3.5a";
    expectedRooms[0].id = m_server->GetRoomService()->CreateChatroom(expectedRooms[0].name);
    expectedRooms[1].name = "Dota 2";
    expectedRooms[1].id = m_server->GetRoomService()->CreateChatroom(expectedRooms[1].name);
    std::sort(expectedRooms.begin(), expectedRooms.end());    

    /// 2. Request chatroom list
    Internal::Request listRequest{};
    listRequest.m_query = Internal::QueryType::LIST_CHATROOM;
    listRequest.m_timestamp = Utils::GetTimestamp();
    listRequest.m_timeout = m_waitTimeout;
    std::string serialized {};
    listRequest.Write(serialized);
    // send request to server
    m_client->Write(std::move(serialized));   
    this->WaitFor(listRequest.m_timeout);

    /// 3. build a chatroom list
    const auto& jsonString = m_client->GetLastResponse().m_attachment;

    rapidjson::Document reader;
    reader.Parse(jsonString.c_str());
    const auto& chatrooms = reader["chatrooms"].GetArray();
    // 
    EXPECT_EQ(chatrooms.Size(), expectedRooms.size());
    std::array<MockChatroom, 2> recievedRooms;
    std::size_t i { 0 };
    for (const auto& roomObj: chatrooms) {
        recievedRooms[i].id = roomObj["id"].GetUint64(); 
        recievedRooms[i].name = roomObj["name"].GetString();
        recievedRooms[i].users = roomObj["users"].GetUint64(); 
        i++;
    }
    std::sort(recievedRooms.begin(), recievedRooms.end());    
    /// 4. Compare expected result and received response
    EXPECT_EQ(m_client->GetLastResponse().m_query, Internal::QueryType::LIST_CHATROOM);
    for (std::size_t i = 0; i < expectedRooms.size(); i++) {
        EXPECT_TRUE(recievedRooms[i] == expectedRooms[i]) << "Rooms are different: { "
            << recievedRooms[i].name << ", " << recievedRooms[i].id << ", " << recievedRooms[i].users << " } != { "
            << expectedRooms[i].name << ", " << expectedRooms[i].id << ", " << expectedRooms[i].users << " }";
    }
}

TEST_F(BasicInteractionTest, JoinChatroomRequest) {
    /// #0. Cinfirm Handshake
    this->ConfirmHandshake();
    
    /// #1 Create chatrooms
    m_server->GetRoomService()->CreateChatroom("Test chatroom #1"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #2"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    const auto desiredId = m_server->GetRoomService()->CreateChatroom(desiredChatroomName); 
    
    // #2 Join
    this->JoinChatroom(desiredId, "random user name", *m_client);

    // #3 Confirm that we've joined
    const auto joinReply = m_client->GetLastResponse();

    EXPECT_EQ(joinReply.m_query, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(joinReply.m_attachment.empty());
}

TEST_F(BasicInteractionTest, CreateChatroomRequest) {
    /// #0. Cinfirm Handshake
    this->ConfirmHandshake();

    /// #1 Create chatrooms
    m_server->GetRoomService()->CreateChatroom("Test chatroom #1"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #2"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    
    // #2 Create & Join Chatroom
    Internal::Request request{};
    request.m_query = Internal::QueryType::CREATE_CHATROOM;
    request.m_timeout = m_waitTimeout;
    request.m_timestamp = Utils::GetTimestamp();
    // Set up attachment
    rapidjson::Document doc(rapidjson::kObjectType);
    auto& alloc = doc.GetAllocator();
    doc.AddMember("user", rapidjson::Value(rapidjson::kObjectType), alloc);
    doc["user"].AddMember("name", rapidjson::Value("random username", alloc), alloc);
    
    doc.AddMember("chatroom", rapidjson::Value(rapidjson::kObjectType) , alloc);
    doc["chatroom"].AddMember("name", rapidjson::Value(desiredChatroomName.c_str(), alloc), alloc);

    request.m_attachment = ::Serialize(doc); 

    // send request
    std::string serialized;
    request.Write(serialized);
    m_client->Write(std::move(serialized));   
    // wait 30ms for answer
    this->WaitFor(request.m_timeout);

    // #3 Confirm that we've joined
    auto joinReply = m_client->GetLastResponse();

    EXPECT_EQ(joinReply.m_query, Internal::QueryType::CREATE_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(!joinReply.m_attachment.empty());

    // #4 Parse attachment and extract room ID
    rapidjson::Document reader;
    reader.Parse(joinReply.m_attachment.c_str());
    const auto id = reader["chatroom"]["id"].GetUint64();
    EXPECT_NE(id, chat::Chatroom::NO_ROOM);

    // #5 Confirm that the server has one room with this ID
    const auto expectedUsersCount { 1 }; // only this client!
    const auto& currentChatroomList = m_server->GetRoomService()->GetChatroomList();
    const std::regex rx { ".+\"id\":[ ]*(\\d+).+\"name\":[ ]*\"(.*)\".+\"users\":[ ]*(\\d+).*" };
    std::smatch match;
    auto foundMatchRoom { false };
    for (const auto& room: currentChatroomList) {
        EXPECT_TRUE(std::regex_match(room, match, rx));

        if (static_cast<std::uint64_t>(std::stoi(match[1].str())) == id) {
            EXPECT_FALSE(foundMatchRoom) << "Room with ID = " << id << " already exist.";
            foundMatchRoom = true;
            EXPECT_EQ(desiredChatroomName, match[2].str());
            EXPECT_EQ(expectedUsersCount, std::stoi(match[3].str()));
        }
    }
    EXPECT_TRUE(foundMatchRoom) << "Can't find a room with ID from server's response";

}

TEST_F(BasicInteractionTest, LeaveChatroomRequest) {
    /// #0. Cinfirm Handshake
    this->ConfirmHandshake();

    /// #1 Create chatrooms
    m_server->GetRoomService()->CreateChatroom("Test chatroom #1"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #2"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    const auto desiredId = m_server->GetRoomService()->CreateChatroom(desiredChatroomName); 
    
    // #2 Join Chatroom
    this->JoinChatroom(desiredId, "random user name", *m_client);

    // #3 Confirm that we've joined
    const auto joinReply = m_client->GetLastResponse();

    EXPECT_EQ(joinReply.m_query, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(joinReply.m_attachment.empty());

    /// #4 Confirm that the server has one room with this ID and 1 user
    enum { ID, USERS, NAME };
    const auto roomTupleOpt = m_server->GetRoomService()->GetChatroomData(desiredId);
    const auto expectedUsersCount { 1 }; // only this client!
    EXPECT_NE(roomTupleOpt, std::nullopt);
    EXPECT_EQ(desiredChatroomName, std::get<NAME>(*roomTupleOpt));
    EXPECT_EQ(expectedUsersCount, std::get<USERS>(*roomTupleOpt));
    
    /// #5 Leave chatroom
    Internal::Request chat{};
    chat.m_query = Internal::QueryType::LEAVE_CHATROOM;
    chat.m_timeout = m_waitTimeout;
    chat.m_timestamp = Utils::GetTimestamp();
    // send request
    std::string serialized {};
    chat.Write(serialized);
    m_client->Write(std::move(serialized));   

    // wait for answer
    this->WaitFor(chat.m_timeout);

    /// #6 Confirmn that we've left
    const auto afterLeaveRoomOpt = m_server->GetRoomService()->GetChatroomData(desiredId);
    EXPECT_EQ(afterLeaveRoomOpt, std::nullopt);
}

TEST_F(BasicInteractionTest, ChatMessageRequest) {
    // #0 Confirm handshake
    this->ConfirmHandshake();

    // #1 Create chatrooms
    m_server->GetRoomService()->CreateChatroom("Test chatroom #1"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #2"); 
    m_server->GetRoomService()->CreateChatroom("Test chatroom #3"); 
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    const auto desiredId = m_server->GetRoomService()->CreateChatroom(desiredChatroomName); 
    
    // #2 Join Chatroom
    this->JoinChatroom(desiredId, "user #1", *m_client);

    // #3 Confirm that we've joined
    const auto joinReply = m_client->GetLastResponse();

    EXPECT_EQ(joinReply.m_query, Internal::QueryType::JOIN_CHATROOM);
    EXPECT_EQ(joinReply.m_status, 200);
    EXPECT_TRUE(joinReply.m_attachment.empty());

    // #4 Add another client to the chatroom
    auto sslContext = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    auto client = std::make_shared<Client>(m_context, sslContext);
    client->Connect("127.0.0.1", "15001");
    this->WaitFor(m_waitTimeout);
    ASSERT_TRUE(client->GetState() == Client::State::RECEIVE_ACK) << "Client hasn't been acknowleged";
    this->JoinChatroom(desiredId, "user #2", *client);

    auto room = m_server->GetRoomService()->GetChatroomData(desiredId);

    // #5 Build chat request
    const std::string msg { "Hello!I'm Bob!" };
    Internal::Request chat{};
    chat.m_query = Internal::QueryType::CHAT_MESSAGE;
    chat.m_timeout = m_waitTimeout;
    chat.m_timestamp = Utils::GetTimestamp();
    chat.m_attachment = "{\"message\":\"" + msg + "\"}";
    // send request
    std::string serialized{};
    chat.Write(serialized);
    m_client->Write(std::move(serialized));   
    // wait for answer
    this->WaitFor(chat.m_timeout);

    // #6 Confirm response to the sender
    const auto chatReply = m_client->GetLastResponse();
    EXPECT_EQ(chatReply.m_query, Internal::QueryType::CHAT_MESSAGE);
    EXPECT_EQ(chatReply.m_status, 200);
    EXPECT_TRUE(chatReply.m_attachment.empty());

    // #7 Confirm response to broadcast
    const auto incoming = client->GetLastResponse();
    EXPECT_EQ(incoming.m_query, Internal::QueryType::CHAT_MESSAGE);
    EXPECT_EQ(incoming.m_status, 200);
    EXPECT_FALSE(incoming.m_attachment.empty());
    
    rapidjson::Document attachment;
    attachment.Parse(incoming.m_attachment.c_str());
    EXPECT_EQ(msg, std::string(attachment["message"].GetString()));
}

/** TODO:
 * Thread safety tests:
 * - [ ] Multiply clients trying to create the chatroom (maybe with the same name);
 * - [ ] 
 */

#endif // SESSION_TESTS_HPP