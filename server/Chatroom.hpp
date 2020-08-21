#ifndef CHATROOM_HPP
#define CHATROOM_HPP

#include <string>
#include <array>
#include <memory>
#include <functional>
#include <cstddef> // std::size_t 

class Session;

namespace chat {

    class Chatroom {
    public:
        static constexpr std::size_t NO_ROOM { 0 };

        Chatroom();

        Chatroom(const std::string & name);

        Chatroom(Chatroom&&chatroom);

        Chatroom & operator=(Chatroom&&rhs);

        ~Chatroom();

        void Close();

        [[nodiscard]] std::size_t GetId() const noexcept;

        [[nodiscard]] std::size_t GetSessionCount() const noexcept;

        [[nodiscard]] const std::string& GetName() const noexcept;

        [[nodiscard]] bool IsEmpty() const noexcept;

        /**
         * @return 
         *      The indication whether the session was insert successfully or not. 
         */
        [[nodiscard]] bool AddSession(const std::shared_ptr<Session>& session);

        [[nodiscard]] bool RemoveSession(const Session * const session);

        [[nodiscard]] bool Contains(const Session * const session) const noexcept;

        [[nodiscard]] std::string AsJSON() const;

        void Rename(const std::string& name);

        /// Chat functions:
        void Broadcast(const std::string& text);

        void Broadcast(const std::string& text, std::function<bool(const Session&)> predicate);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

}

#endif // CHATROOM_HPP