#include "MockStream.hpp"

MockAsyncWriteStream::MockAsyncWriteStream(
    boost::asio::io_context::executor_type& ex
    , std::byte * buffer
    , size_t size
)
    : m_executor { ex }
    , m_buffer { buffer }
    , m_size { size }
{}

MockAsyncReadStream::MockAsyncReadStream(
    boost::asio::io_context::executor_type& ex
    , boost::asio::const_buffer buffer
)
    : m_executor { ex }
    , m_buffer { buffer }
{}