/* Diagnostic Client library
 * Copyright (C) 2023  Avijit Dey
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
// includes
#include <boost/asio.hpp>
#include <string>
#include <thread>
#include <boost/asio/ssl.hpp>
#include "tcp_types.h"
#include "client/client_tcp_iface.h"

namespace boost_support {
namespace socket {
namespace tcp {
    /*
    @ Class Name        : Create Tcp Socket
    @ Class Description : Class used to create a tcp socket for handling transmission
                   and reception of tcp message from driver
    */
    class CreateTcpClientSocket {
    public:
        // Type alias for tcp protocol
        using Tcp = boost::asio::ip::tcp;
        // Type alias for tcp socket
        using TcpSocket = Tcp::socket;
        // Type alias for tcp ip address
        using TcpIpAddress = boost::asio::ip::address;
        // Type alias for tcp error codes
        using TcpErrorCodeType = boost::system::error_code;
        // Tcp function template used for reception
        using TcpHandlerRead = std::function<void(TcpMessagePtr)>;

    public:
        //ctor
        CreateTcpClientSocket(std::string local_ip_address, uint16_t local_port_num, const tls_config& tls_cfg,
                              TcpHandlerRead &&tcp_handler_read);

        //dtor
        virtual ~CreateTcpClientSocket();

        // Function to Open the socket
        bool open();

        // Function to Connect to host
        bool connect_to_host(const std::string& host_ip_address, uint16_t host_port_num);

        // Function to Disconnect from host
        bool disconnect_from_host();

        // Function to trigger transmission
        bool transmit(TcpMessageConstPtr tcpMessage);

        // Function to destroy the socket
        bool destroy();
    private:
        // function to handle read
        void handle_message();
        bool verify_certificate(bool pre_verified,  boost::asio::ssl::verify_context& ctx);
    private:
        // local Ip address
        std::string local_ip_address_;
        // local port number
        uint16_t local_port_num_;
        // tcp socket
        std::unique_ptr<TcpSocket> tcp_socket_{nullptr};
        // tcp socket on tls
        std::unique_ptr<boost::asio::ssl::stream<TcpSocket>> tcp_socket_tls_{nullptr};
        // boost io context
        boost::asio::io_context io_context_;
        boost::asio::ssl::context tls_ctx_{boost::asio::ssl::context::tlsv12};
        // flag to terminate the thread
        std::atomic_bool exit_request_;
        // flag th start the thread
        std::atomic_bool running_;
        // conditional variable to block the thread
        std::condition_variable cond_var_;
        // threading var
        std::thread thread_;
        // locking critical section
        std::mutex mutex_;
        // Handler invoked during read operation
        TcpHandlerRead tcp_handler_read_;
        // tls config
        tls_config tls_cfg_;
    };
}  // namespace tcp
}  // namespace socket
}  // namespace boost_support