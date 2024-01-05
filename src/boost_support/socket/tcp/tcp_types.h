/* Diagnostic Client library
* Copyright (C) 2023  Avijit Dey
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <memory>
#include <vector>

namespace boost_support {
namespace socket {
namespace tcp {
    struct tls_config{
        std::string str_ca_path{};
        std::string str_client_key_path{};
        std::string str_client_crt_path{};
        bool support_tls{false};
        uint8_t message_header_size{0};
        uint8_t body_length_index{29};
        uint8_t body_length_size{2};
        uint8_t msg_tail_size{1};
        uint8_t terminal_mark{0};
        std::string ifc{};
    };
    // tcp message type
    class TcpMessageType {
    public:
        enum class tcpSocketState : std::uint8_t {
            // Socket state
            kIdle = 0x00,
            kSocketOnline,
            kSocketOffline,
            kSocketRxMessageReceived,
            kSocketTxMessageSend,
            kSocketTxMessageConf,
            kSocketError
        };
        enum class tcpSocketError : std::uint8_t {
            // state
            kNone = 0x00
        };
        // buffer type
        using buffType = std::vector<uint8_t>;
        // ip address type
        using ipAddressType = std::string;

    public:
        // ctor
        TcpMessageType() = default;

        // dtor
        virtual ~TcpMessageType() = default;

        // socket state
        tcpSocketState tcp_socket_state_{tcpSocketState::kIdle};

        // socket error
        tcpSocketError tcp_socket_error_{tcpSocketError::kNone};

        // Receive buffer
        buffType rxBuffer_;

        // Transmit buffer
        buffType txBuffer_;

        // host ipaddress
        ipAddressType host_ip_address_{};

        // host port num
        uint16_t host_port_num_{};
    };

    // unique pointer to const TcpMessage
    using TcpMessageConstPtr = std::unique_ptr<const TcpMessageType>;
    // unique pointer to TcpMessage
    using TcpMessagePtr = std::unique_ptr<TcpMessageType>;
}  // namespace tcp
}  // namespace socket
}  // namespace boost_support