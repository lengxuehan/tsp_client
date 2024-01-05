/* Diagnostic Client library
 * Copyright (C) 2023  Avijit Dey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tcp_client.h"
#include <utility>
#include <iostream>
#include "tb_log.h"

namespace boost_support {
namespace socket {
namespace tcp {
    // ctor
    CreateTcpClientSocket::CreateTcpClientSocket(std::string local_ip_address,
                                                 const uint16_t local_port_num,
                                                 const tls_config& tls_cfg,
                                                 TcpHandlerRead &&tcp_handler_read)
            : local_ip_address_{std::move(local_ip_address)},
              local_port_num_{local_port_num},
              exit_request_{false},
              running_{false},
              tcp_handler_read_{tcp_handler_read},
              tls_cfg_{tls_cfg}{
        if (tls_cfg_.support_tls){
            using namespace boost::asio::ssl;
            TB_LOG_INFO("%s\n", tls_cfg_.str_ca_path.c_str());
            tls_ctx_.load_verify_file(tls_cfg_.str_ca_path); // 如果证书是一个字节流，则使用接口add_certificate_authority
            tls_ctx_.use_private_key_file(tls_cfg_.str_client_key_path, context::pem);
            tls_ctx_.use_certificate_file(tls_cfg_.str_client_crt_path, context::pem);
            tcp_socket_tls_ = std::make_unique<boost::asio::ssl::stream<TcpSocket>>(io_context_, tls_ctx_);
        } else {
            // Create socket
            tcp_socket_ = std::make_unique<TcpSocket>(io_context_);
        }
        // Start thread to receive messages
        thread_ = std::thread([&]() {
            std::unique_lock<std::mutex> lck(mutex_);
            while (!exit_request_) {
                if (!running_) {
                    cond_var_.wait(lck, [this]() { return exit_request_ || running_; });
                }
                if (!exit_request_.load()) {
                    if (running_) {
                        lck.unlock();
                        handle_message();
                        lck.lock();
                    }
                }
            }
        });
    }

    // dtor
    CreateTcpClientSocket::~CreateTcpClientSocket() {
        exit_request_ = true;
        running_ = false;
        cond_var_.notify_all();
        thread_.join();
    }

    bool CreateTcpClientSocket::open() {
        TcpErrorCodeType ec{};
        bool retVal{false};
        if (tls_cfg_.support_tls){
            if(tcp_socket_tls_->lowest_layer().is_open()) {
                return true;
            }
            tcp_socket_tls_->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
            tcp_socket_tls_->set_verify_callback([this](bool p, boost::asio::ssl::verify_context& context) {
                return verify_certificate(p, context);
            });
            // Open the socket
            tcp_socket_tls_->lowest_layer().open(Tcp::v4(), ec);
            if (ec.value() == boost::system::errc::success) {
                // reuse address
                tcp_socket_tls_->lowest_layer().set_option(boost::asio::socket_base::reuse_address{true});
                // Set socket to non blocking
                tcp_socket_tls_->lowest_layer().non_blocking(false);
                //bind to local address and random port
                tcp_socket_tls_->lowest_layer().bind(Tcp::endpoint(TcpIpAddress::from_string(local_ip_address_),
                                                                   local_port_num_), ec);
                if (ec.value() == boost::system::errc::success) {
                    // Socket binding success
                    TB_LOG_INFO("Tcp with ssl Socket opened and bound to <%s,%d>\n",
                                tcp_socket_tls_->lowest_layer().local_endpoint().address().to_string().c_str(),
                                tcp_socket_tls_->lowest_layer().local_endpoint().port());
                    retVal = true;
                } else {
                    // Socket binding failed
                    std::cout << "Tcp with  Socket binding failed with message: " << ec.message() << std::endl;
                    retVal = false;
                }
            } else {
                std::cout << "Tcp Socket opening failed with error: " << ec.message() << std::endl;
            }
        } else {
            // Open the socket
            if(tcp_socket_->is_open()) {
                return true;
            }
            tcp_socket_->open(Tcp::v4(), ec);
            if (ec.value() == boost::system::errc::success) {
                // reuse address
                tcp_socket_->set_option(boost::asio::socket_base::reuse_address{true});
                // Set socket to non blocking
                tcp_socket_->non_blocking(false);
                if(!tls_cfg_.ifc.empty()) {
                    struct ifreq ifr;
                    bzero(&ifr, sizeof(ifr));
                    memcpy(ifr.ifr_name, tls_cfg_.ifc.c_str(), tls_cfg_.ifc.length());
                    if (setsockopt(tcp_socket_->native_handle(), SOL_SOCKET, SO_BINDTODEVICE, static_cast<void*>(&ifr), sizeof(ifr)) < 0) {
                        TB_LOG_INFO("bind to local interface:%s failed\n", tls_cfg_.ifc.c_str());
                    }
                }
                //bind to local address and random port
                tcp_socket_->bind(Tcp::endpoint(TcpIpAddress::from_string(local_ip_address_), local_port_num_), ec);
                if (ec.value() == boost::system::errc::success) {
                    // Socket binding success
                    std::cout << "Tcp Socket opened and bound to "
                              << "<" << tcp_socket_->local_endpoint().address().to_string() << ","
                              << tcp_socket_->local_endpoint().port() << ">"
                              << std::endl;
                    retVal = true;
                } else {
                    // Socket binding failed
                    std::cout << "Tcp Socket binding failed with message: " << ec.message() << std::endl;
                    retVal = false;
                }
            } else {
                std::cout << "Tcp Socket opening failed with error: " << ec.message() << std::endl;
            }
        }
        return retVal;
    }

    // connect to host
    bool CreateTcpClientSocket::connect_to_host(const std::string& host_ip_address, uint16_t host_port_num) {
        TcpErrorCodeType ec{};

        using boost::asio::ip::tcp;
        /* Resolve hostname. */
        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), host_ip_address, std::to_string(host_port_num));
        tcp::resolver::iterator iterator = resolver.resolve(query);
        Tcp::endpoint ep = *iterator;
        TB_LOG_INFO("CreateTcpClientSocket::connect_to_host:%s\n", ep.address().to_string().c_str());

        bool ret_val{false};
        if (tls_cfg_.support_tls){
            // connect to provided ipAddress
            tcp_socket_tls_->lowest_layer().connect(ep, ec);
            if (ec.value() == boost::system::errc::success) {
                TB_LOG_INFO("Tcp with tls Socket connected to host <%s,%d>\n",
                            tcp_socket_tls_->lowest_layer().remote_endpoint().address().to_string().c_str(),
                            tcp_socket_tls_->lowest_layer().remote_endpoint().port());
                // handshake
                TB_LOG_INFO("Tcp with tls Socket handshake to host <%s,%d>\n",
                            tcp_socket_tls_->lowest_layer().remote_endpoint().address().to_string().c_str(),
                            tcp_socket_tls_->lowest_layer().remote_endpoint().port());
                tcp_socket_tls_->handshake(boost::asio::ssl::stream_base::client, ec);
                if (ec.value() == boost::system::errc::success){
                    TB_LOG_INFO("Tcp with tls Socket handshake to host");
                    // start reading
                    running_ = true;
                    cond_var_.notify_all();
                    ret_val = true;
                } else{
                    TB_LOG_INFO("Tcp with tls Socket handshake to host error: %s\n",
                                ec.message().c_str());
                }
            } else {
                std::cout << "Tcp with tls Socket connect to host failed with error: "
                << ec.message() << std::endl;
            }
        } else {
            // connect to provided ipAddress
            tcp_socket_->connect(
                    ep, ec);
            if (ec.value() == boost::system::errc::success) {
                std::cout << "Tcp Socket connected to host "
                          << "<" << tcp_socket_->remote_endpoint().address().to_string() << ","
                          << tcp_socket_->remote_endpoint().port() << ">" << std::endl;

                // start reading
                running_ = true;
                cond_var_.notify_all();
                ret_val = true;
            } else {
                std::cout << "Tcp Socket connect to host failed with error: " << ec.message() << std::endl;
            }
        }
        return ret_val;
    }

    // Disconnect from Host
    bool CreateTcpClientSocket::disconnect_from_host() {
        TcpErrorCodeType ec{};
        bool ret_val{false};
        TB_LOG_INFO("CreateTcpClientSocket::disconnect_from_host start to disconnect from host\n");
        if (tls_cfg_.support_tls){
            TB_LOG_INFO("Tcp tls socket start to cancel\n");
            tcp_socket_tls_->lowest_layer().cancel(ec);
            if (ec.value() == boost::system::errc::success) {
                // Graceful shutdown
                TB_LOG_INFO("Tcp tls socket lowest layer start to shutdown\n");
                tcp_socket_tls_->lowest_layer().shutdown(TcpSocket::shutdown_both, ec);
                if (ec.value() == boost::system::errc::success) {
                    // stop reading
                    running_ = false;
                    // Socket shutdown success
                    ret_val = true;
                } else {
                    std::cout << "Tcp tls socket lowest layer shutdown failed with error: "
                    << ec.message() << std::endl;
                }
            } else{
                std::cout << "Tcp tls socket cancel SSL on the stream failed with error: "
                << ec.message() << std::endl;

            }
        } else {// Graceful shutdown
            tcp_socket_->shutdown(TcpSocket::shutdown_both, ec);
            if (ec.value() == boost::system::errc::success) {
                // stop reading
                running_ = false;
                // Socket shutdown success
                ret_val = true;
            } else {
                TB_LOG_INFO("Tcp Socket disconnection from host failed with error:%s\n",
                            ec.message().c_str());
            }
        }
        return ret_val;
    }

    // Function to transmit tcp messages
    bool CreateTcpClientSocket::transmit(TcpMessageConstPtr tcpMessage) {
        TcpErrorCodeType ec;
        bool ret_val{false};
        if(!running_) {
            return ret_val;
        }
        if (tls_cfg_.support_tls){
            boost::asio::write(*tcp_socket_tls_,
                               boost::asio::buffer(tcpMessage->txBuffer_,
                                                   std::size_t(tcpMessage->txBuffer_.size())), ec);
            // Check for error
            if (ec.value() == boost::system::errc::success) {
                std::cout << "Tcp message sent to "
                          << "<" << tcp_socket_tls_->lowest_layer().remote_endpoint().address().to_string() << ","
                          << tcp_socket_tls_->lowest_layer().remote_endpoint().port() << ">" << std::endl;
                ret_val = true;
            } else {
                std::cout << "Tcp message sending failed with error: " << ec.message() << std::endl;
            }
        } else{
            boost::asio::write(*tcp_socket_,
                               boost::asio::buffer(tcpMessage->txBuffer_,
                                                   std::size_t(tcpMessage->txBuffer_.size())), ec);
            // Check for error
            if (ec.value() == boost::system::errc::success) {
                std::cout << "Tcp message sent to "
                          << "<" << tcp_socket_->remote_endpoint().address().to_string() << "," <<
                          tcp_socket_->remote_endpoint().port() << ">" << std::endl;
                ret_val = true;
            } else {
                std::cout << "Tcp message sending failed with error: " << ec.message() << std::endl;
            }
        }
        return ret_val;
    }

    // Destroy the socket
    bool CreateTcpClientSocket::destroy() {
        // destroy the socket
        if (tls_cfg_.support_tls){
            tcp_socket_tls_->lowest_layer().close();
        } else{
            tcp_socket_->close();
        }
        return true;
    }

    // Handle reading from socket
    void CreateTcpClientSocket::handle_message() {
        TcpErrorCodeType ec{};
        TcpMessagePtr tcp_rx_message{std::make_unique<TcpMessageType>()};
        // reserve the buffer
        tcp_rx_message->rxBuffer_.resize(tls_cfg_.message_header_size);
        // start blocking read to read Header first
        if(tls_cfg_.support_tls){
            boost::asio::read(*tcp_socket_tls_, boost::asio::buffer(&tcp_rx_message->rxBuffer_[0],
                                                                    tls_cfg_.message_header_size), ec);
        } else{
            boost::asio::read(*tcp_socket_, boost::asio::buffer(&tcp_rx_message->rxBuffer_[0],
                                                                tls_cfg_.message_header_size), ec);
        }
        // Check for error
        if (ec.value() == boost::system::errc::success) {
            // read the next bytes to read
            uint32_t read_next_bytes = [&tcp_rx_message](uint8_t body_length_index, uint8_t body_length_size) {
                if (body_length_size == 2) {
                    return ((std::uint32_t) ((uint32_t) ((tcp_rx_message->rxBuffer_[body_length_index] << 8) & 0x0000FF00) |
                                             (uint32_t) (tcp_rx_message->rxBuffer_[body_length_index + 1] & 0x000000FF)));
                } else {
                    return ((std::uint32_t) ((uint32_t) ((tcp_rx_message->rxBuffer_[body_length_index] << 24) & 0xFF000000) |
                                             (uint32_t) ((tcp_rx_message->rxBuffer_[body_length_index + 1] << 16) & 0x00FF0000) |
                                             (uint32_t) ((tcp_rx_message->rxBuffer_[body_length_index + 2] << 8) & 0x0000FF00) |
                                             (uint32_t) ((tcp_rx_message->rxBuffer_[body_length_index + 3] & 0x000000FF))));
                }
            }(tls_cfg_.body_length_index, tls_cfg_.body_length_size);
            read_next_bytes += tls_cfg_.msg_tail_size;
            // reserve the buffer
            tcp_rx_message->rxBuffer_.resize(tls_cfg_.message_header_size + std::size_t(read_next_bytes));
            if (tls_cfg_.support_tls){
                boost::asio::read(*tcp_socket_tls_,
                                  boost::asio::buffer(&tcp_rx_message->rxBuffer_[tls_cfg_.message_header_size],
                                                      read_next_bytes), ec);
                if(tls_cfg_.terminal_mark != 0) {
                    if(tcp_rx_message->rxBuffer_.back() != tls_cfg_.terminal_mark) {
                        boost::asio::streambuf left_response;
                        std::size_t n = boost::asio::read_until(*tcp_socket_tls_, left_response, tls_cfg_.terminal_mark);
                        boost::asio::streambuf::const_buffers_type buffers = left_response.data();
                        std::copy(boost::asio::buffers_begin(buffers), boost::asio::buffers_begin(buffers) + n,
                                  std::back_inserter(tcp_rx_message->rxBuffer_));
                    }
                }
            } else{
                boost::asio::read(*tcp_socket_,
                                  boost::asio::buffer(&tcp_rx_message->rxBuffer_[tls_cfg_.message_header_size],
                                                      read_next_bytes), ec);
                if(tls_cfg_.terminal_mark != 0) {
                    if(tcp_rx_message->rxBuffer_.back() != tls_cfg_.terminal_mark) {
                        boost::asio::streambuf left_response;
                        std::size_t n = boost::asio::read_until(*tcp_socket_, left_response, tls_cfg_.terminal_mark);
                        boost::asio::streambuf::const_buffers_type buffers = left_response.data();
                        std::copy(boost::asio::buffers_begin(buffers), boost::asio::buffers_begin(buffers) + n,
                                  std::back_inserter(tcp_rx_message->rxBuffer_));
                    }
                }
            }

            // all message received, transfer to upper layer
            Tcp::endpoint const endpoint_{tls_cfg_.support_tls ?
                    tcp_socket_tls_->lowest_layer().remote_endpoint()
                    : tcp_socket_->remote_endpoint()};
            // fill the remote endpoints
            tcp_rx_message->host_ip_address_ = endpoint_.address().to_string();
            tcp_rx_message->host_port_num_ = endpoint_.port();

            std::cout << "Tcp Message received from "
                << "<" << endpoint_.address().to_string() << ","
                << endpoint_.port() << ">" << std::endl;
            // send data to upper layer
            tcp_handler_read_(std::move(tcp_rx_message));
        } else if (ec.value() == boost::asio::error::eof) {
            running_ = false;
            std::cout << "Remote Disconnected with: " << ec.message() << std::endl;
        } else {
            TB_LOG_INFO("Remote Disconnected with undefined error:%s\n", ec.message().c_str());
            running_ = false;
        }
    }
    bool CreateTcpClientSocket::verify_certificate(bool pre_verified, boost::asio::ssl::verify_context& ctx){
        // The verify callback can be used to check whether the certificate that is
        // being presented is valid for the peer. For example, RFC 2818 describes
        // the steps involved in doing this for HTTPS. Consult the OpenSSL
        // documentation for more details. Note that the callback is called once
        // for each certificate in the certificate chain, starting from the root
        // certificate authority.

        // In this example we will simply print the certificate's subject name.
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        TB_LOG_INFO("Verifying %s\n", subject_name);
        return pre_verified;
    }
}  // namespace tcp
}  // namespace socket
}  // namespace boost_support
