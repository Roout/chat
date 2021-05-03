#ifndef CHAT_USER_HPP
#define CHAT_USER_HPP

#include <string>
#include <cstdint>

namespace Internal {

    struct User final {
        // Lifetime management
        User();

        // Methods
        std::string AsJSON() const;

        static User FromJSON(const std::string& json);
      
        // Properties
        const std::uint64_t m_id;
        std::uint64_t m_chatroom;
        std::string m_username;

    private:
        User(std::uint64_t id);

        inline static std::uint64_t m_instance { 0 }; 
    };
}

#endif // CHAT_USER_HPP