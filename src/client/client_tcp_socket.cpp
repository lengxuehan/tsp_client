#include "client/client_tcp_socket.h"
#include "tb_log.h"
#include <iostream>

namespace tsp_client {

    bool client_tcp_socket::connect(const std::string &addr, std::uint32_t port) {
        /*if(tcp_socket_ != nullptr) { // reconnect, should deleted, or reopen socket and reuse endpoint for boost::asio
            if(connected_.exchange(false)) {
                //std::this_thread::sleep_for(std::chrono::seconds(1)); // insure no message can be published
                tcp_socket_->disconnect_from_host();
                tcp_socket_->destroy();
            }
        }*/

        if(connected_.load()) {
            disconnect(); // to avoid error: Transport endpoint is already connected
        }

        std::string local_ip_address{"0.0.0.0"};
        boost_support::socket::tcp::tls_config tls_cfg;
        tls_cfg.str_ca_path = tls_tcp_cfg_.str_ca_path;
        tls_cfg.str_client_key_path = tls_tcp_cfg_.str_client_key_path;
        tls_cfg.str_client_crt_path = tls_tcp_cfg_.str_client_crt_path;
        tls_cfg.support_tls = tls_tcp_cfg_.support_tls;
        tls_cfg.message_header_size = tls_tcp_cfg_.message_header_size;
        tls_cfg.body_length_index = tls_tcp_cfg_.body_length_index;
        tls_cfg.body_length_size = tls_tcp_cfg_.body_length_size;
        tls_cfg.msg_tail_size = tls_tcp_cfg_.msg_tail_size;
        tls_cfg.terminal_mark = tls_tcp_cfg_.terminal_mark;
        tls_cfg.ifc = tls_tcp_cfg_.ifc;

        bool res{false};
        if(tcp_socket_ == nullptr) {
            tcp_socket_ = std::make_shared<TcpSocket>(local_ip_address, 0U, tls_cfg);
            std::chrono::seconds timeout_duration{10};
            std::weak_ptr<TcpSocket> self = tcp_socket_->shared_from_this();
            tcp_socket_->set_tcp_read_handler([this, self](TcpMessagePtr prt) -> void {
                if(self.lock()) {
                    if(package_handler_) {
                        package_handler_(prt->rxBuffer_);
                    }
                }
            });
        }

        if(!opened_.load()){
            res = tcp_socket_->open();
            if(!res) {
                return false;
            }
            opened_.store(true);
        }

        res = tcp_socket_->connect_to_host(addr, port);
        if(res){
            connected_.store(true);
        }
        return res;
        /*
        std::future<bool> future_connected = std::async(std::launch::async, [this, self, &addr, &port](){
            bool res{false};
            auto tcp_socket = self.lock();
            if(tcp_socket) {
                res = tcp_socket->open();
                if(res) {
                    res = tcp_socket->connect_to_host(addr, port);
                }
                if(!res){
                    tcp_socket->destroy();
                }
            }
            return res;
        });

        const std::future_status future_status = future_connected.wait_for(timeout_duration);
        if (future_status == std::future_status::ready) {
            connected_ = future_connected.get();
            return connected_;
        }else{
            return false;
        }*/
    }

    void client_tcp_socket::disconnect() {
        if(connected_.exchange(false)) {
            tcp_socket_->disconnect_from_host();
            tcp_socket_->destroy();
            opened_.store(false);
        }
    }

    bool client_tcp_socket::is_connected(void) const {
        return connected_;
    }

    bool client_tcp_socket::send(std::vector<uint8_t> &&request){
        if(connected_) {
            TcpMessagePtr tcp_message = std::make_unique<TcpMessage>();
            tcp_message->txBuffer_.swap(request);
            bool res = tcp_socket_->transmit(std::move(tcp_message));
            if(!res) {
                disconnect();
                TB_LOG_ERROR("client_tcp_socket::send failed\n");
                if(disconnection_handler_) {
                    TB_LOG_ERROR("client_tcp_socket::send call disconnection_handler\n");
                    disconnection_handler_();
                }
            }
            return res;
        }
        return false;
    }

    void client_tcp_socket::set_on_disconnection_handler(const disconnection_handler_t &disconnection_handler) {
        disconnection_handler_ = disconnection_handler;
    }

    void client_tcp_socket::set_message_handler(const package_handler_t &package_handler) {
        package_handler_ = package_handler;
    }
} // namespace tsp_client
