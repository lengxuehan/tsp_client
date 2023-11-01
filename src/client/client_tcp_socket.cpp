#include "client_tcp_socket.h"
#include "tb_log.h"
#include <iostream>

namespace tsp_client {

    bool client_tcp_socket::connect(const std::string &addr, std::uint32_t port) {
        if(tcp_socket_ == nullptr){
            // TODO fixme
            std::string local_ip_address{"127.0.0.1"};
            tcp_socket_ = std::make_unique<TcpSocket>(
                    local_ip_address, 0U, ssl_cfg_,
                    [this](TcpMessagePtr prt) -> void {
                        TB_LOG_INFO("client_tcp_socket::connect recv tcp message from tsp data size:%d\n",
                                    prt->rxBuffer_.size());
                        package_handler_(prt->rxBuffer_);
                    });
        }
        bool res = tcp_socket_->open();
        if(res) {
            res = tcp_socket_->connect_to_host(addr, port);
        }
        if(res){
            connected_ = true;
        }
        return res;
    }

    void client_tcp_socket::disconnect() {
        connected_ = false;
        tcp_socket_->disconnect_from_host();
    }

    bool client_tcp_socket::is_connected(void) const {
        return connected_;
    }

    bool client_tcp_socket::send(std::vector<uint8_t> &&request){
        TcpMessagePtr tcp_message = std::make_unique<TcpMessage>();
        tcp_message->txBuffer_.swap(request);
        bool res = tcp_socket_->transmit(std::move(tcp_message));
        if(!res && !disconnection_handler_){
            TB_LOG_INFO("client_tcp_socket::send failed, call disconnection_handler\n");
            disconnection_handler_();
        }
        return res;
    }

    void client_tcp_socket::set_on_disconnection_handler(const disconnection_handler_t &disconnection_handler) {
        disconnection_handler_ = disconnection_handler;
    }

    void client_tcp_socket::set_message_header_handler(const package_header_handler_t &header_handler) {
        if(tcp_socket_ != nullptr) {
            tcp_socket_->set(header_handler);
        }
    }

    void client_tcp_socket::set_message_header_size(uint8_t size) {
        if(tcp_socket_ != nullptr) {
            tcp_socket_->set_message_header_size(size);
        }
    }

    void client_tcp_socket::set_message_handler(const package_handler_t &package_handler) {
        package_handler_ = package_handler;
    }
} // namespace tsp_client
