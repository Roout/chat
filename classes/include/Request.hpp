#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <memory>
#include <string>
#include "RequestType.hpp"

namespace Requests {

    /**
     * Sequence used to divide important parts of the request (see Request::Impl)
     * during serialization.
     */
    inline const std::string DELIMETER { "\n\n" };

    inline const std::string REQUEST_DELIMETER { "\n\r\n\r" };

    class Request {
    public:

        Request();
        
        ~Request();

        void Reset();

        /**
         * TODO: this function need refactoring: std::stoi throws!
         * 
         * Parse @request string and initialize @m_impl.
         * 
         * @param frame
         *      Full one request which is not parsed yet.
         * 
         * @return error code 
         *      On success: 0
         *      on error: number > 0 which indicates on which step parsing failed
         */
        [[nodiscard]] int Parse(const std::string& frame);

        /**
         * Prepare request for any output stream.
         * It basically add delimeter after each data member.
         * The last data member is special request delimeter.
         */
        std::string Serialize() const;

        void SetType(RequestType type);

        void SetChatroom(size_t chatroomId);

        void SetName(const std::string& name);

        void SetBody(const std::string& body);

        RequestType GetType() const noexcept;

        size_t GetChatroom() const noexcept;

        const std::string& GetName() const noexcept;

        const std::string& GetBody() const noexcept;
    private:
        /**
         * This is struct which hide implementation details. 
         */
        struct Impl;

        std::unique_ptr<Impl> m_impl { nullptr };
    };
}

#endif // REQUEST_HPP