/* Diagnostic Client library
* Copyright (C) 2023  Avijit Dey
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "tcp_server.h"
#include <iostream>

namespace boost_support {
namespace socket {
namespace tcp {

    using TcpIpAddress = boost::asio::ip::address;
    using TcpErrorCodeType = boost::system::error_code;

    CreateTcpServerSocket::CreateTcpServerSocket(std::string local_ip_address, const uint16_t local_port_num)
            : local_ip_address_{std::move(local_ip_address)},
              local_port_num_{local_port_num} {
        // Create accepter
        tcp_acceptor_ = std::make_unique<TcpAcceptor>(io_context_, Tcp::endpoint(Tcp::v4(), local_port_num_), true);
        std::cout << "Tcp Socket Accepter created at "
            << "<" << local_ip_address_ << "," << local_port_num_ << ">" << std::endl;
    }

    CreateTcpServerSocket::TcpServerConnection CreateTcpServerSocket::get_tcp_server_connection(
            TcpHandlerRead &&tcp_handler_read) {
        TcpErrorCodeType ec;
        Tcp::endpoint endpoint{};
        CreateTcpServerSocket::TcpServerConnection tcp_connection{io_context_, std::move(tcp_handler_read)};

        // blocking accept
        tcp_acceptor_->accept(tcp_connection.get_socket(), endpoint, ec);
        if (ec.value() == boost::system::errc::success) {
            std::cout << "Tcp Socket connection received from client "
                << "<" << endpoint.address().to_string() << "," << endpoint.port() << ">" << std::endl;
        } else {
            std::cout << "Tcp Socket Connect to client failed with error: " << ec.message() << std::endl;
        }
        return tcp_connection;
    }

    CreateTcpServerSocket::TcpServerConnection::TcpServerConnection(boost::asio::io_context &io_context,
                                                                    TcpHandlerRead &&tcp_handler_read)
            : tcp_socket_{io_context},
              tcp_handler_read_{tcp_handler_read} {}

    TcpSocket &CreateTcpServerSocket::TcpServerConnection::get_socket() {
        return tcp_socket_;
    }

    bool CreateTcpServerSocket::TcpServerConnection::transmit(TcpMessageConstPtr udp_tx_message) {
        TcpErrorCodeType ec{};
        bool ret_val{false};
        boost::asio::write(tcp_socket_,
                           boost::asio::buffer(udp_tx_message->txBuffer_,
                                               std::size_t(udp_tx_message->txBuffer_.size())), ec);
        // Check for error
        if (ec.value() == boost::system::errc::success) {
            Tcp::endpoint endpoint_{tcp_socket_.remote_endpoint()};
            std::cout << "Tcp message sent to "
                << "<" << endpoint_.address().to_string() << "," << endpoint_.port() << ">";
            ret_val = true;
        } else {
            std::cout << "Tcp message sending failed with error: " << ec.message() << std::endl;
        }
        return ret_val;
    }

    bool CreateTcpServerSocket::TcpServerConnection::received_message() {
        TcpErrorCodeType ec;
        bool connection_closed{false};
        TcpMessagePtr tcp_rx_message = std::make_unique<TcpMessageType>();
        // reserve the buffer
        tcp_rx_message->rxBuffer_.resize(kDoipHeaderSize);
        // start blocking read to read Header first
        boost::asio::read(tcp_socket_, boost::asio::buffer(&tcp_rx_message->rxBuffer_[0], kDoipHeaderSize), ec);
        // Check for error
        if (ec.value() == boost::system::errc::success) {
            // read the next bytes to read
            uint32_t read_next_bytes = [&tcp_rx_message]() {
                return ((uint32_t) ((uint32_t) ((tcp_rx_message->rxBuffer_[4] << 24) & 0xFF000000) |
                                    (uint32_t) ((tcp_rx_message->rxBuffer_[5] << 16) & 0x00FF0000) |
                                    (uint32_t) ((tcp_rx_message->rxBuffer_[6] << 8) & 0x0000FF00) |
                                    (uint32_t) ((tcp_rx_message->rxBuffer_[7] & 0x000000FF))));
            }();
            // reserve the buffer
            tcp_rx_message->rxBuffer_.resize(kDoipHeaderSize + std::size_t(read_next_bytes));
            boost::asio::read(tcp_socket_,
                              boost::asio::buffer(&tcp_rx_message->rxBuffer_[kDoipHeaderSize], read_next_bytes),
                              ec);
            // all message received, transfer to upper layer
            Tcp::endpoint endpoint_{tcp_socket_.remote_endpoint()};
            std::cout << "Tcp Message received from "
                << "<" << endpoint_.address().to_string() << "," << endpoint_.port() << ">";
            // fill the remote endpoints
            tcp_rx_message->host_ip_address_ = endpoint_.address().to_string();
            tcp_rx_message->host_port_num_ = endpoint_.port();

            // send data to upper layer
            tcp_handler_read_(std::move(tcp_rx_message));
        } else if (ec.value() == boost::asio::error::eof) {
            std::cout << "Remote Disconnected with: " << ec.message() << std::endl;
            connection_closed = true;
        } else {
            std::cout << "Remote Disconnected with undefined error: " << ec.message() << std::endl;
            connection_closed = true;
        }
        return connection_closed;
    }

    bool CreateTcpServerSocket::TcpServerConnection::shutdown() {
        TcpErrorCodeType ec{};
        bool ret_val{false};
        // Graceful shutdown
        if (tcp_socket_.is_open()) {
            tcp_socket_.shutdown(TcpSocket::shutdown_both, ec);
            if (ec.value() == boost::system::errc::success) {
                ret_val = true;
            } else {
                std::cout << "Tcp Socket Disconnection failed with error: " << ec.message() << std::endl;
            }
            tcp_socket_.close();
        }
        return ret_val;
    }

}  // namespace tcp
}  // namespace socket
}  // namespace boost_support
