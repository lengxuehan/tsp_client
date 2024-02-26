#include "client/client_connection.h"
#include "client/client_tcp_socket.h"
#include "tb_log.h"
#include "packages/packet_helper.h"

namespace tsp_client {

    client_connection::client_connection(const tls_tcp_config& tls_tcp_cfg)
            : client_(std::make_shared<client_tcp_socket>(tls_tcp_cfg)){
    }

    client_connection::~client_connection(void) {
        client_->disconnect();
        TB_LOG_INFO("client_connection::client_connection destroyed\n");
    }

    bool client_connection::connect(const std::string &host, std::size_t port,
                                    const disconnection_handler_t &client_disconnection_handler,
                                    const reply_callback_t &client_reply_callback) {
        str_remote_host_ =  host;
        TB_LOG_INFO("client_connection::connect attempts to connect host:%s port:%d\n",
                    host.c_str(), port);
        try {
            //! connect client
            bool res = client_->connect(host, (uint32_t) port);
            if(res) {
                client_->set_on_disconnection_handler(
                        std::bind(&client_connection::tcp_client_disconnection_handler, this));
                client_->set_message_handler([this](const std::vector<uint8_t> &data)->bool {
                    return this->tcp_client_receive_handler(data);
                });
                reply_callback_ = client_reply_callback;
                disconnection_handler_ = client_disconnection_handler;
                TB_LOG_INFO("client_connection::connect successful to connect remote server\n");
            } else {
                TB_LOG_INFO("client_connection::connect failed to connect remote server\n");
            }

            return res;
        }
        catch (const std::exception &e) {
            TB_LOG_INFO("client_connection::connect %s\n", e.what());
            //throw std::exception(e);
            return false;
        }
    }

    void client_connection::disconnect() {
        TB_LOG_INFO("client_connection::disconnect attempts to disconnect\n");
        client_->disconnect();
    }

    bool client_connection::is_connected(void) const {
        return client_->is_connected();
    }

    bool client_connection::send(std::vector<uint8_t> &&request) {
        return client_->send(std::move(request));
    }

    std::string client_connection::get_remote_host_ip() {
        return str_remote_host_;
    }

    void client_connection::call_disconnection_handler(void) {
        TB_LOG_INFO("client_connection::call_disconnection_handler calls disconnection handler\n");
        if (disconnection_handler_) {
            disconnection_handler_(*this);
        } else {
            TB_LOG_ERROR("client_connection::call_disconnection_handler got empyt disconnection handler\n");
        }
    }

    bool client_connection::tcp_client_receive_handler(const std::vector<uint8_t> &response) {
        try {
            if(reply_callback_ != nullptr) {
                reply_callback_(*this, response);
            }

            return true;
        }
        catch (const std::exception &e) {
            TB_LOG_INFO("tsp_client_::connection could not parse packet (invalid format), "
                        "disconnecting:%s\n", e.what());
            call_disconnection_handler();
            return false;
        }
    }

    void client_connection::tcp_client_disconnection_handler(void) {
        TB_LOG_INFO("tsp_client::connection has been disconnected\n");
        //! call disconnection handler
        call_disconnection_handler();
    }

} // namespace tsp_client
