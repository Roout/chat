#ifndef RT_REQUEST_QUEUE_HPP
#define RT_REQUEST_QUEUE_HPP

#include <queue>
#include <mutex>

#include "Message.hpp"

namespace rt {
    
/**
 * Thread-safe decorator around the std::queue 
 */
class RequestQueue {
public:
    using WrappedType = std::queue<Internal::Request>;

    RequestQueue() = default;

    ~RequestQueue() = default;

    RequestQueue(RequestQueue&& rsh) = delete;

    RequestQueue& operator=(RequestQueue&& rsh) = delete;

    void Swap(RequestQueue& rsh) noexcept {
        if (this != &rsh) {
            std::scoped_lock lock { m_mutex, rsh.m_mutex };
            m_queue.swap(rsh.m_queue);
        }
    }

    void Push(Internal::Request&& request) {
        std::lock_guard<std::mutex> lock{ m_mutex };
        m_queue.emplace(std::move(request));
    }

    void Pop() {
        std::lock_guard<std::mutex> lock{ m_mutex };
        m_queue.pop();
    }

    Internal::Request Extract() {
        Internal::Request top{};
        {
            std::lock_guard<std::mutex> lock{ m_mutex };
            top = std::move(m_queue.front());
            m_queue.pop();
        }
        return top;
    }

    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock{ m_mutex };
        return m_queue.empty();
    }

private:
    mutable std::mutex m_mutex {};
    WrappedType m_queue {};
};

} // namespace rt

#endif // RT_REQUEST_QUEUE_HPP