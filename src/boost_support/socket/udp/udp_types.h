/* Diagnostic Client library
 * Copyright (C) 2023  Avijit Dey
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace boost_support {
namespace socket {
namespace udp {

    class UdpMessageType {
    public:
        // buffer type
        using BuffType = std::vector<uint8_t>;
        // ip address type
        using IPAddressType = std::string;

    public:
        // ctor
        UdpMessageType() = default;

        // dtor
        virtual ~UdpMessageType() = default;

        // Receive buffer
        BuffType rx_buffer_;
        // Transmit buffer
        BuffType tx_buffer_;
        // host ipaddress
        IPAddressType host_ip_address_;
        // host port num
        uint16_t host_port_num_{};
    };

    // unique pointer to const UdpMessage
    using UdpMessageConstPtr = std::unique_ptr<const UdpMessageType>;
    // unique pointer to UdpMessage
    using UdpMessagePtr = std::unique_ptr<UdpMessageType>;
    // Response size udp
    constexpr uint8_t kDoipUdpResSize = 50U;
}  // namespace udp
}  // namespace socket
}  // namespace boost_support