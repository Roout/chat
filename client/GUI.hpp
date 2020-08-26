#ifndef CLIENT_GUI_HPP
#define CLIENT_GUI_HPP

#include <iostream>

#include "Message.hpp"

class GUI {
public:

    void UpdateResponse(Internal::Response&& response) {
        m_response = std::move(response);
    }

    void Display() const {
        
    }

    const Internal::Response& GetResponse() const {
        return m_response;
    }

private:
    Internal::Response m_response{};
};

#endif // CLIENT_GUI_HPP