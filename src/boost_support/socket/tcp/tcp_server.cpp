/* Diagnostic Client library
* Copyright (C) 2023  Avijit Dey
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "tcp_server.h"
#include <iostream>
#include "tb_log.h"

namespace boost_support {
namespace socket {
namespace tcp {

    using TcpIpAddress = boost::asio::ip::address;
    using TcpErrorCodeType = boost::system::error_code;

    CreateTcpServerSocket::CreateTcpServerSocket(std::string local_ip_address, const uint16_t local_port_num,
                                                 const ssl_config& ssl_cfg)
            : local_ip_address_{std::move(local_ip_address)},
              local_port_num_{local_port_num},
              ssl_cfg_{ssl_cfg} {
        if(ssl_cfg.support_tls){
            // Create accepter
            tcp_acceptor_ = std::make_unique<TcpAcceptor>(io_context_,
                                                          Tcp::endpoint(Tcp::v4(), local_port_num_), true);
            TB_LOG_INFO("Tcp Socket Acceptor created at <%s, %d>\n", local_ip_address_.c_str(), local_port_num_);
            using namespace boost::asio::ssl;
            //TODO fixme, just for a example
            ssl_context_.use_tmp_dh_file("/mnt/d/work/tsp_client/demo/etc/server/dh2048.pem");
            ssl_context_.load_verify_file(ssl_cfg.str_ca_path);
            ssl_context_.set_options(context::verify_peer | context::single_dh_use);
            ssl_context_.set_verify_mode(verify_peer); //
            ssl_context_.use_certificate_file(ssl_cfg.str_client_crt_path, context::pem);
            ssl_context_.use_private_key_file(ssl_cfg.str_client_key_path, context::pem);
        }else{
            // Create accepter
            tcp_acceptor_ = std::make_unique<TcpAcceptor>(io_context_,
                                                          Tcp::endpoint(Tcp::v4(), local_port_num_), true);
            TB_LOG_INFO("Tcp Socket Acceptor created at <%s, %d>\n", local_ip_address_.c_str(), local_port_num_);
        }
    }

    bool CreateTcpServerSocket::verify_certificate(bool pre_verified, boost::asio::ssl::verify_context& ctx){
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

    CreateTcpServerSocket::TcpServerConnection CreateTcpServerSocket::get_tcp_server_connection(
            TcpHandlerRead &&tcp_handler_read) {
        TcpErrorCodeType ec;
        Tcp::endpoint endpoint{};
        if(ssl_cfg_.support_tls){
            CreateTcpServerSocket::TcpServerConnection tcp_connection{io_context_, ssl_context_,
                                                                      std::move(tcp_handler_read)};

            // blocking accept
            tcp_acceptor_->accept(tcp_connection.get_ssl_socket().lowest_layer(), endpoint, ec);
            if (ec.value() == boost::system::errc::success) {
                TB_LOG_INFO("Tcp with tls Socket connection received from client <%s,%d>\n",
                            endpoint.address().to_string().c_str(), endpoint.port());
                tcp_connection.get_ssl_socket().set_verify_callback([this](bool p, boost::asio::ssl::verify_context& context) {
                    return verify_certificate(p, context);
                });
                tcp_connection.get_ssl_socket().handshake(boost::asio::ssl::stream_base::server, ec);
                TB_LOG_INFO("Tcp with tls Socket start to handshake with client host <%s,%d>\n",
                            endpoint.address().to_string().c_str(),
                            endpoint.port());
                if (ec.value() == boost::system::errc::success) {
                    TB_LOG_INFO("Tcp with tls Socket handshake successful \n");
                }else{
                    TB_LOG_INFO("Tcp with tls Socket handshake failed: %s\n", ec.message().c_str());
                }
            } else {
                TB_LOG_INFO("Tcp with tls Socket Connect to client failed with error: %s\n", ec.message().c_str());
            }
            return tcp_connection;
        }else{
            CreateTcpServerSocket::TcpServerConnection tcp_connection{io_context_, std::move(tcp_handler_read)};

            // blocking accept
            tcp_acceptor_->accept(tcp_connection.get_socket(), endpoint, ec);
            if (ec.value() == boost::system::errc::success) {
                TB_LOG_INFO("Tcp Socket connection received from client <%s,%d>\n",
                            endpoint.address().to_string().c_str(), endpoint.port());
            } else {
                TB_LOG_INFO("Tcp Socket Connect to client failed with error: %s\n", ec.message().c_str());
            }
            return tcp_connection;
        }
    }

    CreateTcpServerSocket::TcpServerConnection::TcpServerConnection(boost::asio::io_context &io_context,
                                                                    TcpHandlerRead &&tcp_handler_read)
            : tcp_socket_{std::make_unique<TcpSocket>(io_context)},
              tcp_handler_read_{tcp_handler_read} {}

    CreateTcpServerSocket::TcpServerConnection::TcpServerConnection(boost::asio::io_context &io_context,
                                                                    boost::asio::ssl::context &ssl_context,
                                                                    TcpHandlerRead &&tcp_handler_read)
            :tcp_socket_{nullptr},
             tcp_handler_read_{tcp_handler_read},
             tcp_socket_ssl_{std::make_unique<boost::asio::ssl::stream<TcpSocket>>(io_context, ssl_context)}{}

   TcpSocket &CreateTcpServerSocket::TcpServerConnection::get_socket() {
        return *tcp_socket_;
    }

    boost::asio::ssl::stream<TcpSocket> &CreateTcpServerSocket::TcpServerConnection::get_ssl_socket(){
        return *tcp_socket_ssl_;
    }

    bool CreateTcpServerSocket::TcpServerConnection::transmit(TcpMessageConstPtr udp_tx_message) {
        TcpErrorCodeType ec{};
        bool ret_val{false};
        if(tcp_socket_ != nullptr){
            boost::asio::write(*tcp_socket_,
                               boost::asio::buffer(udp_tx_message->txBuffer_,
                                                   std::size_t(udp_tx_message->txBuffer_.size())), ec);
            // Check for error
            if (ec.value() == boost::system::errc::success) {
                Tcp::endpoint endpoint_{tcp_socket_->remote_endpoint()};
                TB_LOG_INFO("Tcp message sent to <%s,%d>\n",
                            endpoint_.address().to_string().c_str(), endpoint_.port());
                ret_val = true;
            } else {
                TB_LOG_INFO("Tcp message sending failed with error: %s\n", ec.message().c_str());
            }
        } else {
            boost::asio::write(*tcp_socket_ssl_,
                               boost::asio::buffer(udp_tx_message->txBuffer_,
                                                   std::size_t(udp_tx_message->txBuffer_.size())), ec);
            // Check for error
            if (ec.value() == boost::system::errc::success) {
                Tcp::endpoint endpoint_{tcp_socket_ssl_->lowest_layer().remote_endpoint()};
                TB_LOG_INFO("Tcp with ssl message sent to <%s,%d>\n",
                            endpoint_.address().to_string().c_str(), endpoint_.port());
                ret_val = true;
            } else {
                TB_LOG_INFO("Tcp with ssl  message sending failed with error: %s\n", ec.message().c_str());
            }
        }

        return ret_val;
    }

    bool CreateTcpServerSocket::TcpServerConnection::received_message() {
        TcpErrorCodeType ec;
        bool connection_closed{false};
        TcpMessagePtr tcp_rx_message = std::make_unique<TcpMessageType>();
        // reserve the buffer
        tcp_rx_message->rxBuffer_.resize(kMessageHeaderSize);
        if(tcp_socket_ != nullptr){
            // start blocking read to read Header first without ssl
            boost::asio::read(*tcp_socket_, boost::asio::buffer(&tcp_rx_message->rxBuffer_[0],
                                                                kMessageHeaderSize), ec);
        }else{
            // start blocking read to read Header first with ssl
            boost::asio::read(*tcp_socket_ssl_, boost::asio::buffer(&tcp_rx_message->rxBuffer_[0],
                                                                    kMessageHeaderSize), ec);
        }
        // Check for error
        if (ec.value() == boost::system::errc::success) {
            // read the next bytes to read
            uint32_t read_next_bytes = 0;
            // reserve the buffer
            tcp_rx_message->rxBuffer_.resize(kMessageHeaderSize + std::size_t(read_next_bytes));
            if(tcp_socket_ != nullptr){
                boost::asio::read(*tcp_socket_,
                                  boost::asio::buffer(&tcp_rx_message->rxBuffer_[kMessageHeaderSize],
                                                      read_next_bytes), ec);
                // all message received, transfer to upper layer
                Tcp::endpoint endpoint{tcp_socket_->remote_endpoint()};
                TB_LOG_INFO("Tcp Message received from <%s,%d>\n", endpoint.address().to_string().c_str(),
                            endpoint.port());
                // fill the remote endpoints
                tcp_rx_message->host_ip_address_ = endpoint.address().to_string();
                tcp_rx_message->host_port_num_ = endpoint.port();
            }else{
                boost::asio::read(*tcp_socket_ssl_,
                                  boost::asio::buffer(&tcp_rx_message->rxBuffer_[kMessageHeaderSize],
                                                      read_next_bytes), ec);
                // all message received, transfer to upper layer
                Tcp::endpoint endpoint{tcp_socket_ssl_->lowest_layer().remote_endpoint()};
                TB_LOG_INFO("Tcp with ssl Message received from <%s,%d>\n", endpoint.address().to_string().c_str(),
                            endpoint.port());
                // fill the remote endpoints
                tcp_rx_message->host_ip_address_ = endpoint.address().to_string();
                tcp_rx_message->host_port_num_ = endpoint.port();
            }
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
        if (tcp_socket_ != nullptr && tcp_socket_->is_open()) {
            // Graceful shutdown
            tcp_socket_->shutdown(TcpSocket::shutdown_both, ec);
            if (ec.value() == boost::system::errc::success) {
                ret_val = true;
            } else {
                TB_LOG_INFO("Tcp Socket Disconnection failed with error: %s\n", ec.message().c_str());
            }
            tcp_socket_->close();
        }
        if (tcp_socket_ssl_ != nullptr && tcp_socket_ssl_->lowest_layer().is_open()) {
            // Graceful shutdown
            tcp_socket_ssl_->shutdown(ec);
            if(ec.value() == boost::system::errc::success) {
                tcp_socket_ssl_->lowest_layer().shutdown(TcpSocket::shutdown_both, ec);
                if (ec.value() == boost::system::errc::success) {
                    ret_val = true;
                } else {
                    TB_LOG_INFO("Tcp with ssl Socket lowest layer Disconnection failed with error: %s\n",
                                ec.message().c_str());
                }
            }else {
                TB_LOG_INFO("Tcp with ssl Socket Disconnection failed with error: %s\n", ec.message().c_str());
            }

            tcp_socket_ssl_->lowest_layer().close();
        }
        return ret_val;
    }

}  // namespace tcp
}  // namespace socket
}  // namespace boost_support
