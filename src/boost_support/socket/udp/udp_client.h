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

#include "udp_types.h"

namespace boost_support {
namespace socket {
namespace udp {
    using UdpSocket = boost::asio::ip::udp;
    using UdpIpAddress = boost::asio::ip::address;
    using UdpErrorCodeType = boost::system::error_code;

    /*
    @ Class Name        : Create Udp Socket
    @ Class Description : Class used to create a udp socket for handling transmission
                   and reception of udp message from driver
    */
    class CreateUdpClientSocket {
    public:
        // Udp function template used for reception
        using UdpHandlerRead = std::function<void(UdpMessagePtr)>;
        // Port Type
        enum class PortType : std::uint8_t {
            kUdp_Broadcast = 0,
            kUdp_Unicast
        };

    public:
        // ctor
        CreateUdpClientSocket(std::string local_ip_address, uint16_t local_port_num, PortType port_type,
                              UdpHandlerRead udp_handler_read);

        // dtor
        virtual ~CreateUdpClientSocket();

        // Function to Open the socket
        bool open();

        // Transmit
        bool transmit(UdpMessageConstPtr udp_message);

        // Function to destroy the socket
        bool destroy();
    private:
        // function to handle read
        void handle_message(const UdpErrorCodeType &error, std::size_t bytes_recvd);
    private:
        // local Ip address
        std::string local_ip_address_;
        // local port number
        uint16_t local_port_num_;
        // udp socket
        std::unique_ptr<UdpSocket::socket> udp_socket_;
        // boost io context
        boost::asio::io_context io_context_;
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
        // end points
        UdpSocket::endpoint remote_endpoint_;
        // port type - broadcast / unicast
        PortType port_type_;
        // Handler invoked during read operation
        UdpHandlerRead udp_handler_read_;
        // Rx buffer
        uint8_t rxbuffer_[kDoipUdpResSize];
    };
}  // namespace udp
}  // namespace socket
}  // namespace boost_support