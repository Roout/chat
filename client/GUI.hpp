#ifndef CLIENT_GUI_HPP
#define CLIENT_GUI_HPP

#include <iostream>

#include "Message.hpp"

class GUI {
public:

    void UpdateResponse(Internal::Response&& response) {
        m_response = std::move(response);
    }

    void UpdateChat(Internal::Chat&& chat) {
        m_chat = std::move(chat);
    }

    void Display() const {
        
    }

    const Internal::Response& GetResponse() const {
        return m_response;
    }

    const Internal::Chat& GetChat() const {
        return m_chat;
    }

private:
    Internal::Response m_response{};
    Internal::Chat m_chat{};
};

#endif // CLIENT_GUI_HPP