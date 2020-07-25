#include "gtest/gtest.h"
#include "Server.hpp"
#include "Session.hpp"
#include "Client.hpp"

#include <memory>
#include <thread>

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


TEST_F(TCPInteractionTest, OnlyFixureSetup) {
    EXPECT_TRUE(true) << "Problems with Fixure initialization occured";
}

TEST_F(TCPInteractionTest, ClientAuthorization) {
    Requests::Request request{};
    request.SetType(Requests::RequestType::AUTHORIZE);
    request.SetName("Random Name #1");
    request.SetBody("Testing authorization");
    
    // send authorization request to server
    this->m_client->Write(request.Serialize());    
    // give 3 seconds for server - client communication
    this->WaitFor(3'000);
    
    // check results:

}

