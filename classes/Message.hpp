#ifndef INTERNAL_MESSAGE_HPP
#define INTERNAL_MESSAGE_HPP

#include <memory>
#include <iostream>
#include <string>
#include "QueryType.hpp"
#include "../rapidjson/document.h"

namespace Internal {
    
    // Double CRLF as delimeter
    const std::string MESSAGE_DELIMITER { "\r\n\r\n" };
    
    struct Message {

        virtual ~Message() = default;

        virtual void Read(rapidjson::Document& doc) = 0;

        virtual void Write(std::string& json) = 0;

        virtual const char* GetProtocol() const noexcept = 0;
    };

    struct Request : public Message {

        static constexpr char* const TAG { "request" };

        /**
         * Protocol
         *  - "Simple Request-Response Protocol" (RRP)
         */
        static constexpr char* const PROTOCOL { "RRP" };

        /**
         * Type of request. 
         */
        QueryType m_type { QueryType::UNDEFINED };

        /**
         * This is a time when the request was formed and sent. 
         * It's a time since epoch in milliseconds.
         */
        long long m_timestamp { 0 };

        /**
         * It's a time in milliseconds within which the request 
         * is valid and can be processed.
         */
        std::size_t m_timeout { 0 };
        
        /**
         * This is a data required for the choosen query type.
         * It's given in JSON format - serialized json object with fields.  
         */
        std::string m_attachment {};

        // Methods

        void Read(rapidjson::Document& doc) override;

        void Write(std::string& json) override;

        const char* GetProtocol() const noexcept override {
            return PROTOCOL;
        }
    };

    struct Response : public Message {
        
        static constexpr char* const TAG { "response" };

        /**
         * Protocol
         *  - "Simple Request-Response Protocol" (RRP)
         */
        static constexpr char* const PROTOCOL { "RRP" };

        /**
         * Type of request. 
         */
        QueryType m_type { QueryType::UNDEFINED };

        /**
         * This is a time when the request was formed and sent. 
         * It's a time since epoch in milliseconds.
         */
        long long m_timestamp { 0 };

        /**
         * This is information about errors which may occure.
         */
        int m_status { 0 };
        
        /**
         * This is message that describes what's gone wrong.
         * Empty if @code m_status == 200 @endcode
         */
        std::string m_error {};

        /**
         * This is a data required for the choosen query type.
         * It's given in JSON format - serialized json object with fields.  
         */
        std::string m_attachment {};

        // Methods

        void Read(rapidjson::Document& doc) override;

        void Write(std::string& json) override;

        const char* GetProtocol() const noexcept override {
            return PROTOCOL;
        }
    };

    struct Chat : public Message {
        // Properties

        static constexpr char* const TAG { "chat"};
        /**
         * Protocol used for the communication at chatroom 
         */
        static constexpr char* const PROTOCOL { "chat" }; 
        /**
         * This is a time when the request was formed and sent. 
         * It's a time since epoch in milliseconds.
         */
        long long m_timestamp { 0 };

        /**
         * It's a time in milliseconds within which the request 
         * is valid and can be processed.
         */
        std::size_t m_timeout { 0 };

        /**
         * This is a text message 
         */
        std::string m_message {};

        // Methods
        void Read(rapidjson::Document& doc) override;

        void Write(std::string& json) override;

        const char* GetProtocol() const noexcept override {
            return PROTOCOL;
        }
    };

    /**
     * This function reads json object from the string,
     * deside what type of message it is and create appropriate one.
     */
    std::unique_ptr<Message> Read(const std::string& json);
}

#endif // INTERNAL_MESSAGE_HPP