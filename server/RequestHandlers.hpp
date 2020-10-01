#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "classes/QueryType.hpp"
#include "classes/Message.hpp"

#include <memory>
#include <type_traits>

class Session;

namespace RequestHandlers {

    using Internal::Request;
    using Internal::Response;
    using Internal::QueryType;

    /// Declarations

    class Executor {
    public:
        /// Lifecycle
        Executor(const Request *request, Session * service);
        
        virtual ~Executor() = default;

        Executor& operator=(const Executor&) = delete;

        Executor(const Executor&) = delete;

        /// Methods
        
        void Run();

    protected:
        
        /**
         * Check whether a request is valid, i.e. 
         * client meets all requeirements to ask for the defined
         * in request command
         */
        virtual bool IsValidRequest() = 0;
        
        /**
         * Perform what is asked by incoming request. 
         * Modify reply base on result.
         */
        virtual void ExecuteRequest() = 0;

        /**
         * Send already formed reply to remote peer
         */
        void SendResponse();

        /// Properties
        
        const Request *m_request { nullptr };
        Session *m_service { nullptr };
        Response m_reply {};
    };

    class Sync : public Executor {
    public:
        using Executor::Executor;

    private:
        bool IsValidRequest() override;

        void ExecuteRequest() override;
    };

    class LeaveChatroom : public Executor {
    public:
        using Executor::Executor;

    private:
        bool IsValidRequest() override;

        void ExecuteRequest() override;
    };

    class JoinChatroom : public Executor {
    public:
        using Executor::Executor;
        
    private:
        bool IsValidRequest() override;

        void ExecuteRequest() override;
    };

    class CreateChatroom : public Executor {
    public:
        using Executor::Executor;
        
    private:
        bool IsValidRequest() override;

        void ExecuteRequest() override;
    };

    class ListChatroom : public Executor {
    public:
        using Executor::Executor;
        
    private:
        bool IsValidRequest() override;

        void ExecuteRequest() override;
    };

    class ChatMessage : public Executor {
    public:
        using Executor::Executor;
        
    private:
        bool IsValidRequest() override;

        void ExecuteRequest() override;
    };

    /// Helper types
    namespace Traits {

        template<QueryType query>
        struct RequestExecutor {
            using Type = void; 
        };

        template<>
        struct RequestExecutor<QueryType::SYN> {
            using Type = Sync;
        };

        template<>
        struct RequestExecutor<QueryType::LEAVE_CHATROOM> {
            using Type = LeaveChatroom;
        };

        template<>
        struct RequestExecutor<QueryType::JOIN_CHATROOM> {
            using Type = JoinChatroom;
        };

        template<>
        struct RequestExecutor<QueryType::CREATE_CHATROOM> {
            using Type = CreateChatroom;
        };

        template<>
        struct RequestExecutor<QueryType::LIST_CHATROOM> {
            using Type = ListChatroom;
        };

        template<>
        struct RequestExecutor<QueryType::CHAT_MESSAGE> {
            using Type = ChatMessage;
        };
    }
}

/// Exposed to programmer

template<Internal::QueryType query>
std::unique_ptr<RequestHandlers::Executor> CreateExecutor(
    const Internal::Request *request, 
    Session * service
 ) {
    /**
     *  TODO: add comile-time check for existance of 
     * executor which handle given QueryType 
     */
    static_assert( true, 
        "Fail instantinate CreateExecutor function"
    );

    using namespace RequestHandlers;
    using ExecutorType = typename Traits::RequestExecutor<query>::Type; 

    return std::make_unique<ExecutorType>(request, service);
} 

#endif // REQUEST_HANDLER_HPP
