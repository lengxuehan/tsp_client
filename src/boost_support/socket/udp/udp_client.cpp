/* Diagnostic Client library
 * Copyright (C) 2023  Avijit Dey
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// includes
#include "udp_client.h"
#include <utility>
#include <iostream>

namespace boost_support {
namespace socket {
namespace udp {
    // ctor
    CreateUdpClientSocket::CreateUdpClientSocket(std::string local_ip_address, const uint16_t local_port_num,
                                                 const PortType port_type, UdpHandlerRead udp_handler_read)
            : local_ip_address_{std::move(local_ip_address)},
              local_port_num_{local_port_num},
              exit_request_{false},
              running_{false},
              port_type_{port_type},
              udp_handler_read_{std::move(udp_handler_read)},
              rxbuffer_{} {
        // Create socket
        udp_socket_ = std::make_unique<UdpSocket::socket>(io_context_);
        // Start thread to receive messages
        thread_ = std::thread([&]() {
            std::unique_lock<std::mutex> lck(mutex_);
            while (!exit_request_) {
                if (!running_) {
                    cond_var_.wait(lck, [this]() { return exit_request_ || running_; });
                }
                if (!exit_request_) {
                    if (running_) {
                        io_context_.restart();
                        io_context_.run();
                    }
                }
            }
        });
    }

    // dtor
    CreateUdpClientSocket::~CreateUdpClientSocket() {
        exit_request_ = true;
        running_ = false;
        cond_var_.notify_all();
        if (thread_.joinable()){
            thread_.join();
        }
    }

    bool CreateUdpClientSocket::open() {
        UdpErrorCodeType ec;
        bool retVal = false;
        // Open the socket
        udp_socket_->open(UdpSocket::v4(), ec);
        if (ec.value() == boost::system::errc::success) {
            // set broadcast option
            boost::asio::socket_base::broadcast broadcast_option(true);
            udp_socket_->set_option(broadcast_option);
            // reuse address
            boost::asio::socket_base::reuse_address reuse_address_option(true);
            udp_socket_->set_option(reuse_address_option);

            if (port_type_ == PortType::kUdp_Broadcast) {
                // Todo : change the hardcoded value of port number 13400
                udp_socket_->bind(UdpSocket::endpoint(boost::asio::ip::address_v4::any(), 13400), ec);
            } else {
                //bind to local address and random port
                udp_socket_->bind(UdpSocket::endpoint(UdpIpAddress::from_string(local_ip_address_), local_port_num_), ec);
            }

            if (ec.value() == boost::system::errc::success) {
                UdpSocket::endpoint endpoint{udp_socket_->local_endpoint()};
                std::cout << "Udp Socket Opened and bound to "
                        << "<" << endpoint.address().to_string() << "," << endpoint.port() << ">"
                        << std::endl;
                // Update the port number with new one
                local_port_num_ = udp_socket_->local_endpoint().port();
                // start reading
                running_ = true;
                cond_var_.notify_all();
                retVal = true;
                // start async receive
                udp_socket_->async_receive_from(
                        boost::asio::buffer(rxbuffer_, kDoipUdpResSize), remote_endpoint_,
                        [this](const UdpErrorCodeType &error, std::size_t bytes_recvd) {
                            handle_message(error, bytes_recvd);
                        });
            } else {
                // Socket binding failed
                std::cout << "Udp Socket Bind failed with message: " << ec.message()
                        << std::endl;
            }
        } else {
            std::cout << "Udp Socket Opening failed with error: " << ec.message()
                    << std::endl;
        }
        return retVal;
    }

    // function to transmit udp messages
    bool CreateUdpClientSocket::transmit(UdpMessageConstPtr udp_message) {
        bool ret_val{false};
        try {
            // Transmit to remote endpoints
            std::size_t send_size{udp_socket_->send_to(
                    boost::asio::buffer(udp_message->tx_buffer_, std::size_t(udp_message->tx_buffer_.size())),
                    UdpSocket::endpoint(UdpIpAddress::from_string(udp_message->host_ip_address_),
                                        udp_message->host_port_num_))};
            // Check for error
            if (send_size == udp_message->tx_buffer_.size()) {
                // successful
                UdpSocket::endpoint endpoint{udp_socket_->local_endpoint()};
                std::cout << "Udp message sent : "
                    << "<" << endpoint.address().to_string() << "," << endpoint.port() << ">"
                    << " -> "
                    << "<" << udp_message->host_ip_address_ << "," << udp_message->host_port_num_
                    << ">" << std::endl;
                ret_val = true;
                // start async receive
                udp_socket_->async_receive_from(
                        boost::asio::buffer(rxbuffer_, kDoipUdpResSize), remote_endpoint_,
                        [this](const UdpErrorCodeType &error, std::size_t bytes_recvd) {
                            handle_message(error, bytes_recvd);
                        });
            }
        } catch (boost::system::system_error const &ec) {
            UdpErrorCodeType error = ec.code();
            std::cout << "Udp message sending to "
                << "<" << udp_message->host_ip_address_ << "> "
                << "failed with error: " << error.message() << std::endl;
        }
        return ret_val;
    }

    // Function to destroy the socket
    bool CreateUdpClientSocket::destroy() {
        // destroy the socket
        udp_socket_->close();
        running_ = false;
        io_context_.stop();
        return true;
    }

    // function invoked when datagram is received
    void CreateUdpClientSocket::handle_message(const UdpErrorCodeType &error, const std::size_t bytes_recvd) {
        // Check for error
        if (error.value() == boost::system::errc::success) {
            if (local_ip_address_ != remote_endpoint_.address().to_string()) {
                UdpMessagePtr udp_rx_message = std::make_unique<UdpMessageType>();
                // Copy the data
                udp_rx_message->rx_buffer_.insert(udp_rx_message->rx_buffer_.end(), rxbuffer_,
                                                  rxbuffer_ + bytes_recvd);
                // fill the remote endpoints
                udp_rx_message->host_ip_address_ = remote_endpoint_.address().to_string();
                udp_rx_message->host_port_num_ = remote_endpoint_.port();

                // all message received, transfer to upper layer
                UdpSocket::endpoint remote_endpoint{remote_endpoint_};
                UdpSocket::endpoint local_endpoint{udp_socket_->local_endpoint()};
                std::cout << "Udp Message received: " << bytes_recvd << " bytes "
                    << "<" << remote_endpoint.address().to_string() << "," << remote_endpoint.port()
                    << ">"
                    << " -> "
                    << "<" << local_ip_address_ << "," << local_port_num_ << ">"
                        << std::endl;

                // send data to upper layer
                udp_handler_read_(std::move(udp_rx_message));
                // start async receive
                udp_socket_->async_receive_from(
                        boost::asio::buffer(rxbuffer_, kDoipUdpResSize), remote_endpoint_,
                        [this](const UdpErrorCodeType &error, std::size_t bytes_recvd) {
                            handle_message(error, bytes_recvd);
                        });
            } else {
                std::cout << "Udp Message received from "
                    << "<" << remote_endpoint_.address().to_string() << "," << remote_endpoint_.port() << ">"
                    << " ignored as received by self ip"
                    << " <" << local_ip_address_ << ">";
            }
        } else {
            if (error.value() != boost::asio::error::operation_aborted) {
                std::cout << "<" << local_ip_address_ << ">: "
                    << "Remote Disconnected with undefined error: " << error.message() << std::endl;
            }
        }
    }
}  // namespace udp
}  // namespace socket
}  // namespace boost_support
