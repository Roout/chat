#include "DoubleBuffer.hpp"

Buffers::Buffers(std::size_t reserved) {
    m_buffers[0].reserve(reserved);
    m_buffers[1].reserve(reserved);
    m_bufferSequence.reserve(reserved);
}

void Buffers::Enque(std::string&& data) {
    m_buffers[m_activeBuffer ^ 1].emplace_back(std::move(data));
}

void Buffers::SwapBuffers() {
    m_bufferSequence.clear();

    m_buffers[m_activeBuffer].clear();
    m_activeBuffer ^= 1;

    for(const auto& buf: m_buffers[m_activeBuffer]) {
        m_bufferSequence.emplace_back(asio::const_buffer(buf.c_str(), buf.size()));
    }
}
