#ifndef INTERNAL_MESSAGE_HPP
#define INTERNAL_MESSAGE_HPP

#include <memory>
#include <iostream>
#include <string>
#include "QueryType.hpp"

namespace Internal {
    
    // Double CRLF as delimiter
    const std::string MESSAGE_DELIMITER { "\r\n\r\n" };
    
    /**
     * Message interface 
     */
    struct Message {

        virtual ~Message() = default;

        /**
         * Initialize this instance from the json type
         * @param json
         *  This is a serialized message in json format
         */
        virtual void Read(const std::string& json) = 0;

        /**
         * This method serialize this instance to json format
         * @param[out] json
         *  String which will hold serialized value. 
         */
        virtual void Write(std::string& json) const = 0;

    };

    struct Request : public Message {

        /**
         * Type of request. 
         */
        QueryType m_query { QueryType::UNDEFINED };

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
        
        /**
         * Initialize this instance from the json type
         * @param json
         *  This is a serialized message in json format
         */
        void Read(const std::string& json) override;

        /**
         * This method serialize this instance to json format
         * @param[out] json
         *  String which will hold serialized value. 
         */
        void Write(std::string& json) const override;

    };

    struct Response : public Message {
  
        /**
         * Type of response. 
         */
        QueryType m_query { QueryType::UNDEFINED };

        /**
         * This is a time when the response was formed and sent. 
         * It's a time since epoch in milliseconds.
         */
        long long m_timestamp { 0 };

        /**
         * This is information about errors which may occure.
         */
        int m_status { 0 };
        
        /**
         * This is message that describes what's gone wrong.
         * Empty if 
         * @code 
         * m_status == 200 
         * @endcode
         */
        std::string m_error {};

        /**
         * This is a data required for the choosen query type.
         * It's given in JSON format - serialized json object with fields.  
         */
        std::string m_attachment {};

        // Methods
        
        /**
         * Initialize this instance from the json type
         * @param json
         *  This is a serialized message in json format
         */
        void Read(const std::string& json) override;

        /**
         * This method serialize this instance to json format
         * @param[out] json
         *  String which will hold serialized value. 
         */
        void Write(std::string& json) const override;

    };

}

#endif // INTERNAL_MESSAGE_HPP