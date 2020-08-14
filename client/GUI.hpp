#ifndef CLIENT_GUI_HPP
#define CLIENT_GUI_HPP

#include <iostream>

#include "Request.hpp"

class GUI {
public:

    void UpdateRequest(Requests::Request&& request) {
        m_request = std::move(request);
    }

    void Display() const {
        std::cout << m_request.GetBody() << '\n';
    }

    const Requests::Request& GetRequest() const {
        return m_request;
    }

private:
    Requests::Request m_request{};
};

#endif // CLIENT_GUI_HPP