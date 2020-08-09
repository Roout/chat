#ifndef CLIENT_GUI_HPP
#define CLIENT_GUI_HPP

#include <iostream>
#include <string>

class GUI {
public:

    void UpdateBuffer(const std::string& buffer) {
        m_buffer = buffer;
    }

    void Display() const {
        std::cout << m_buffer << '\n';
    }

    const std::string& GetBuffer() const {
        return m_buffer;
    }

private:
    std::string m_buffer;
};

#endif // CLIENT_GUI_HPP