#include "client_tcp_socket.h"
#include <iostream>

namespace tsp_client {

    bool client_tcp_socket::connect(const std::string &addr, std::uint32_t port) {
        if(tcp_socket_ == nullptr){
            tcp_socket_ = std::make_unique<TcpSocket>(addr, port, ssl_cfg_, [this](TcpMessagePtr)-> void{
                // TODO handle tcp message
                std::cout << "client_tcp_socket::connect recv tcp message from tsp" << std::endl;
            });
        }
        bool res = tcp_socket_->connect_to_host(addr, port);
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
            std::cout << "client_tcp_socket::send failed, call disconnection_handler" << std::endl;
            disconnection_handler_();
        }
        return res;
    }

    void client_tcp_socket::set_on_disconnection_handler(const disconnection_handler_t &disconnection_handler) {
        disconnection_handler_ = disconnection_handler;
    }
} // namespace tsp_client
