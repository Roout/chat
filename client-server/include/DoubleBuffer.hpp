#ifndef DOUBLE_BUFFER_HPP
#define DOUBLE_BUFFER_HPP

#include <vector>
#include <string>
#include <array>
#include <boost/asio.hpp>

namespace asio = boost::asio;

class Buffers final {
public:
    Buffers(size_t reserved = 10) {
        m_buffers[0].reserve(reserved);
        m_buffers[1].reserve(reserved);
        m_bufferSequence.reserve(reserved);
    }

    void Enque(std::string&& data) {
        m_buffers[m_activeBuffer ^ 1].emplace_back(std::move(data));
    }

    void SwapBuffers() {
        m_bufferSequence.clear();

        m_buffers[m_activeBuffer].clear();
        m_activeBuffer ^= 1;

        for(const auto& buf: m_buffers[m_activeBuffer]) {
            m_bufferSequence.emplace_back(asio::const_buffer(buf.c_str(), buf.size()));
        }
    }

    size_t GetQueueSize() const noexcept {
        return m_buffers[m_activeBuffer ^ 1].size();
    } 

    const std::vector<asio::const_buffer>& GetBufferSequence() const noexcept {
        return m_bufferSequence;
    }
private:
    /// TODO: introduce new class double buffer or look for it in boost.
    using DoubleBuffer = std::array<std::vector<std::string>, 2>;
    
    DoubleBuffer m_buffers;

    std::vector<asio::const_buffer> m_bufferSequence;
    
    size_t m_activeBuffer { 0 };
};

#endif // DOUBLE_BUFFER_HPP