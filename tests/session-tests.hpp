#include "gtest/gtest.h"
#include "Server.hpp"
#include "Session.hpp"
#include "Client.hpp"
#include "InteractionStage.hpp"

#include <memory>
#include <thread>
#include <regex>

class TCPInteractionTest : public ::testing::Test {
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

    void WaitFor(size_t ms) {
        m_timer->expires_from_now(boost::posix_time::millisec(ms));
        m_timer->wait();
    }
   
protected:
    std::unique_ptr<Server> m_server {};
    std::unique_ptr<Client> m_client {};
    std::shared_ptr<boost::asio::io_context> m_context {};
    std::vector<std::thread> m_threads {};
    std::shared_ptr<boost::asio::deadline_timer> m_timer;
};

/**
 * Client connect ->
 * Server accept -> Server send welcome message
 */
TEST_F(TCPInteractionTest, OnlyFixureSetup) {
    // prepare (done in ::SetUp method)
    // execute (done in ::SetUp method)
    this->WaitFor(25);
    // test
    EXPECT_EQ(m_client->GetStage(), IStage::State::UNAUTHORIZED) 
        << "Problems with Fixure initialization occured: "
        << "\nClient's stage is: " << static_cast<size_t>(m_client->GetStage())
        << "\nExpected: " << static_cast<size_t>(IStage::State::UNAUTHORIZED);
}

/**
 * Client connect ->
 * Server accept -> Server send welcome message ->
 * Client send authorrize request ->
 * Server read and response with confirmation  
 */
TEST_F(TCPInteractionTest, ClientAuthorization) {
    Requests::Request request{};
    request.SetType(Requests::RequestType::AUTHORIZE);
    request.SetName("Random Name #1");
    request.SetBody("Testing authorization");
    
    // send authorization request to server
    m_client->Write(request.Serialize());    
    // give 3 seconds for server - client communication
    this->WaitFor(25);
    
    // check results:
    EXPECT_EQ(m_client->GetStage(), IStage::State::AUTHORIZED) 
        << "Problems with Fixure initialization occured: "
        << "\nClient's stage is: " << static_cast<size_t>(m_client->GetStage())
        << "\nExpected: " << static_cast<size_t>(IStage::State::AUTHORIZED);
}

TEST_F(TCPInteractionTest, AuthorizedChatroomListRequest) {
    /// 1. Server creates chatrooms
    m_server->CreateChatroom("WoW 3.3.5a");
    m_server->CreateChatroom("Dota 2");
    m_server->CreateChatroom("Programming");
    m_server->CreateChatroom("Chatting");
    
    /// 2. Complete authorization 
    Requests::Request request{};
    request.SetType(Requests::RequestType::AUTHORIZE);
    request.SetName("Random Name #1");
    
    // send authorization request to server
    m_client->Write(request.Serialize());   
    // give 0.025 seconds for server - client communication
    this->WaitFor(25);
    // make sure that authorization was completed
    EXPECT_EQ(m_client->GetStage(), IStage::State::AUTHORIZED) 
        << "Problems with Fixure initialization occured: "
        << "\nClient's stage is: " << static_cast<size_t>(m_client->GetStage())
        << "\nExpected: " << static_cast<size_t>(IStage::State::AUTHORIZED); 
    
    /// 3. Request chatroom list
    Requests::Request listRequest{};
    listRequest.SetType(Requests::RequestType::LIST_CHATROOM);
    // send request to server
    m_client->Write(listRequest.Serialize());   
    // give  0.025 seconds for server response
    this->WaitFor(25);

    /// 4. Compare expected result and recieved response
    const auto sourceChatList = m_server->GetChatroomList(); // chatroom list which has been sent
    const auto aquiredChatList = m_client->GetGUI().GetRequest().GetBody(); // chatroom list which has been recieved united via '\n' symbol
    // devide chatroom list
    std::vector<std::string> resultList;
    size_t start { 0 };
    size_t finish = aquiredChatList.find('\n', start);
    while( finish != std::string::npos ) {
        resultList.emplace_back(aquiredChatList.substr(start, finish - start));
        start = finish + 1;
        finish = aquiredChatList.find('\n', start);
    }
    resultList.emplace_back(aquiredChatList.substr(start, std::string::npos));

    // compare source and result
    EXPECT_EQ(m_client->GetGUI().GetRequest().GetCode(), Requests::ErrorCode::SUCCESS);
    EXPECT_EQ(resultList.size(), sourceChatList.size());
    for(size_t i = 0; i < resultList.size(); i++) {
        EXPECT_EQ(resultList[i], sourceChatList[i]);
    }
}

TEST_F(TCPInteractionTest, UnauthorizedChatroomListRequest) {
    /// 1. Server creates chatrooms
    m_server->CreateChatroom("WoW 3.3.5a");
    m_server->CreateChatroom("Dota 2");
    m_server->CreateChatroom("Programming");
    m_server->CreateChatroom("Chatting");
    
    /// 2. Ignore authorization 
    EXPECT_EQ(m_client->GetStage(), IStage::State::UNAUTHORIZED) 
        << "Problems with Fixure initialization occured: "
        << "\nClient's stage is: " << static_cast<size_t>(m_client->GetStage())
        << "\nExpected: " << static_cast<size_t>(IStage::State::UNAUTHORIZED); 

    /// 3. Request chatroom list
    Requests::Request listRequest{};
    listRequest.SetType(Requests::RequestType::LIST_CHATROOM);
    // send request to server
    m_client->Write(listRequest.Serialize());   
    // give  0.025 seconds for server response
    this->WaitFor(25);
    /// 4. Compare expected result and recieved response
    const auto sourceChatList = m_server->GetChatroomList(); // chatroom list which has been sent
    const auto &recievedReqeuest = m_client->GetGUI().GetRequest();
    
    EXPECT_EQ(recievedReqeuest.GetType(), Requests::RequestType::LIST_CHATROOM);
    EXPECT_EQ(recievedReqeuest.GetStage(), IStage::State::UNAUTHORIZED);
    EXPECT_EQ(recievedReqeuest.GetCode(), Requests::ErrorCode::FAILURE);

}

// Requests::RequestType::JOIN_CHATROOM for already existing chatrooms
TEST_F(TCPInteractionTest, AuthorizedJoinChatroomRequest) {
    const std::string desiredChatroomName {"This TestF's Target chatroom"};
    // #0 Create chatrooms
    m_server->CreateChatroom("Test chatroom #1"); 
    m_server->CreateChatroom("Test chatroom #2"); 
    m_server->CreateChatroom("Test chatroom #3"); 
    m_server->CreateChatroom(desiredChatroomName); 
    // #1 Complete authorization
    Requests::Request request{};
    request.SetType(Requests::RequestType::AUTHORIZE);
    request.SetName("Authorized Join Chatroom Request");
    
    // send authorization request to server
    m_client->Write(request.Serialize());    
    // give 3 seconds for server - client communication
    this->WaitFor(25);
    
    // check results:
    EXPECT_EQ(m_client->GetStage(), IStage::State::AUTHORIZED) 
        << "Problems with Fixure initialization occured: "
        << "\nClient's stage is: " << static_cast<size_t>(m_client->GetStage())
        << "\nExpected: " << static_cast<size_t>(IStage::State::AUTHORIZED); 
    
    // #2 Get current chatroom list
    request.Reset();
    request.SetType(Requests::RequestType::LIST_CHATROOM);
    // send request
    m_client->Write(request.Serialize());   
    // wait 25ms for answer
    this->WaitFor(25);
    // confirm that there is a successfull reply
    const auto& listReply = m_client->GetGUI().GetRequest();
    EXPECT_EQ(listReply.GetCode(), Requests::ErrorCode::SUCCESS);
    const auto& chatroomList = listReply.GetBody();
    
    // #3 Extract desired chatroom ID
    const auto namePos = chatroomList.find(desiredChatroomName);
    EXPECT_NE(namePos, std::string::npos) 
        << "Can't find: " << desiredChatroomName;
    // Chatroom string representation is in json format: { "id": .., "name": "..", "users": .. }
    const auto idTagPos = chatroomList.rfind("id", namePos);
    EXPECT_NE(idTagPos, std::string::npos) 
        << "Can't find id for: " << desiredChatroomName;
    
    // Extract id
    const auto idStart = chatroomList.find(':', idTagPos);
    const auto idEnd = chatroomList.find(',', idStart);
    
    EXPECT_NE(idStart, std::string::npos);
    EXPECT_NE(idEnd, std::string::npos);
    
    const auto idRep =  chatroomList.substr(idStart + 1, idEnd - idStart - 1);
    const auto id = std::stoi(idRep);

    // #4 Join Chatroom
    request.Reset();
    request.SetType(Requests::RequestType::JOIN_CHATROOM);
    request.SetChatroom(id);
    // send request
    m_client->Write(request.Serialize());   
    // wait 25ms for answer
    this->WaitFor(25);

    // #5 Confirm that we've joined
    const auto& joinReply = m_client->GetGUI().GetRequest();

    EXPECT_EQ(joinReply.GetType(), Requests::RequestType::JOIN_CHATROOM);
    EXPECT_EQ(joinReply.GetCode(), Requests::ErrorCode::SUCCESS) 
        << "Server deni access to the chatroom: " << desiredChatroomName 
        << " with id: " << id;
    EXPECT_EQ(joinReply.GetStage(), IStage::State::AUTHORIZED);
    EXPECT_EQ(joinReply.GetChatroom(), id);
}

// Requests::RequestType::CREATE_CHATROOM create and join new chatroom.
TEST_F(TCPInteractionTest, AuthorizedCreateChatroomRequest) {
    // #0 Create chatrooms
    m_server->CreateChatroom("Test chatroom #1"); 
    m_server->CreateChatroom("Test chatroom #2"); 
    m_server->CreateChatroom("Test chatroom #3"); 
    // #1 Complete authorization
    Requests::Request request{};
    request.SetType(Requests::RequestType::AUTHORIZE);
    request.SetName("Authorized Join Chatroom Request");
    
    // send authorization request to server
    m_client->Write(request.Serialize());    
    // give 3 seconds for server - client communication
    this->WaitFor(25);
    
    // check results:
    EXPECT_EQ(m_client->GetStage(), IStage::State::AUTHORIZED) 
        << "Problems with Fixure initialization occured: "
        << "\nClient's stage is: " << static_cast<size_t>(m_client->GetStage())
        << "\nExpected: " << static_cast<size_t>(IStage::State::AUTHORIZED); 

    // #2 Create Chatroom
    const std::string roomName { "Room for testing a room creation via client's request" };
    request.Reset();
    request.SetType(Requests::RequestType::CREATE_CHATROOM);
    request.SetName(roomName);
    // send request
    m_client->Write(request.Serialize());   
    // wait 25ms for answer
    this->WaitFor(25);

    // #3 Confirm that we've joined
    const auto& joinReply = m_client->GetGUI().GetRequest();

    EXPECT_EQ(joinReply.GetType(), Requests::RequestType::CREATE_CHATROOM);
    EXPECT_EQ(joinReply.GetCode(), Requests::ErrorCode::SUCCESS) 
        << "Server forbid creating chatrooms";
    EXPECT_EQ(joinReply.GetStage(), IStage::State::AUTHORIZED);
    EXPECT_EQ(joinReply.GetName(), roomName);

    // #4 Confirm number of users in this room & it's existence;
    int usersCount { -1 };
    const auto chatrooms { m_server->GetChatroomList() };
    // find just created chatroom
    // { "id": 2, "name": "some long name", "users": 1 } 
    const std::regex rx { ".+\"name\":[ ]*\"(.*)\".+\"users\":[ ]*(\\d+).*" };
    std::smatch match;
    for(const auto& room: chatrooms) {
        EXPECT_TRUE(std::regex_match(room,  match, rx));
        // The first sub_match is the whole string; the next
        // sub_match is the first parenthesized expression.
        const auto name { match[1].str() };
        if( name == roomName ) {
            usersCount = std::stoi(match[2].str());
            break;
        }
    } 
    EXPECT_EQ(usersCount, 1) 
        << "Either some weird error occured either room doesn't exist!";
}

// Requests::RequestType::LEAVE_CHATROOM 
TEST_F(TCPInteractionTest, AuthorizedLeaveChatroomRequest) {

}

/** TODO:
 * Thread safety tests:
 * - [ ] Multiply clients trying to create the chatroom (maybe with the same name);
 * - [ ] 
 */