#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <functional> // std::bind
#include <boost/asio.hpp>

namespace asio = boost::asio;

class Client final {
public:
    
    Client(std::shared_ptr<asio::io_context> io):
        m_io { io },
        m_socket{ *m_io }
    {
    }

    ~Client() {
        this->Close();
    }

    void Connect(const std::string& path, std::uint16_t port) {
        asio::ip::tcp::resolver resolver(*m_io);
        auto endpoints = resolver.resolve(path, std::to_string(port));
        if( endpoints.empty() ) {
            this->Close();
        }
        else {
            auto endpoint { endpoints.begin()->endpoint() };
            m_socket.async_connect(endpoint, std::bind(&Client::OnConnect, this, std::placeholders::_1));
        }
    }
    
    void Write(std::string && text ) {
        m_outcomingBuffer = std::move(text);
        m_socket.async_write_some(
            asio::buffer(m_outcomingBuffer),
            std::bind(&Client::OnWrite, 
                this, 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        );
    }

private:

    void Close() {
        boost::system::error_code ec;
        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if(ec) {

        }
        ec.clear();
        
        m_socket.close(ec);
        if(ec) {

        }
    }

    void OnConnect(const boost::system::error_code& err) {
        if(err) {
            std::cerr << err.message() << "\n";
        }
        else {
            std::cerr << "Connected successfully!\n";
            // start waiting incoming calls
            this->Read();
        }
    }

    void Read() {
        m_isReading = true;
        std::cout << "Start reading!\n";
        m_tempBuffer.resize(1024);
        m_socket.async_read_some(
            asio::buffer(m_tempBuffer),
            std::bind(&Client::OnRead, this, std::placeholders::_1, std::placeholders::_2)
        );
    }

    void OnRead(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    ) {
        if(transferredBytes) {
            m_tempBuffer.resize(transferredBytes);
            std::cout << m_socket.remote_endpoint() << ": "<< m_tempBuffer << '\n'; 
        }

        if( !error ) {
            m_isReading = true;
            this->Read();
        } 
        else if( error == boost::asio::error::eof) {
            // peer has closed this connection
            // handle all recieved data
            m_isReading = false;
            this->Close();
        } 
        else {
            m_isReading = false;
            std::cerr << error.message() << "\n";
            this->Close();
        }
    }

    void OnWrite(
        const boost::system::error_code& error, 
        std::size_t transferredBytes
    ) {
        if( !error ){
            std::cout << "Just sent: " << transferredBytes << " bytes\n";
            m_transferred += transferredBytes;
            std::cout << "Already sent: " << m_transferred << " bytes\n";
            
            if(m_transferred < m_outcomingBuffer.size() ) {
                // we need to send other data
                std::cout << " Trying to send: "<< m_outcomingBuffer.size() - m_transferred << " bytes\n";
                m_socket.async_write_some(
                    asio::buffer(&m_outcomingBuffer[m_transferred], m_outcomingBuffer.size() - m_transferred), 
                    std::bind(&Client::OnWrite, 
                        this, 
                        std::placeholders::_1, 
                        std::placeholders::_2
                    )
                );
            }
            else if( m_transferred == m_outcomingBuffer.size()) {
                m_transferred = 0;
            }
        }
    }

private:
    
    std::shared_ptr<asio::io_context>   m_io;
    
    asio::ip::tcp::socket               m_socket;
    
    bool            m_isReading { false };
    std::string     m_tempBuffer;
    std::string     m_outcomingBuffer;
    std::size_t     m_transferred { 0 };                    
};