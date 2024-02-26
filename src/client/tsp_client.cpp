#include "tb_log.h"
#include "client/tsp_client.h"
#include "client/tsp_client_config.h"
#include "client/client.h"

namespace tsp_client {
    TspClient::TspClient(const std::string& str_config_path) {
        TspClientConfig tls_tcp_cfg;
        bool loaded = tls_tcp_cfg.load_config(str_config_path);
        if (!loaded) {
            TB_LOG_ERROR("tsp_client load config %s failed\n", str_config_path.c_str());
            return;
        }
        tls_tcp_cfg.dump_config();

        tsp_client_config_.server_ip   = tls_tcp_cfg.server_ip;
        tsp_client_config_.port        = tls_tcp_cfg.port;
        tsp_client_config_.str_ca_path = tls_tcp_cfg.ca_path;
        tsp_client_config_.str_client_key_path = tls_tcp_cfg.key_path;
        tsp_client_config_.str_client_crt_path = tls_tcp_cfg.cert_path;
        tsp_client_config_.support_tls         = tls_tcp_cfg.support_tls;
        tsp_client_config_.message_header_size = tls_tcp_cfg.message_header_size;
        tsp_client_config_.body_length_index   = tls_tcp_cfg.body_length_index;
        tsp_client_config_.body_length_size    = tls_tcp_cfg.body_length_size;
        tsp_client_config_.ifc                 = tls_tcp_cfg.ifc;
    }

    TspClient::TspClient(const tsp_client::tls_tcp_config &config):tsp_client_config_(config){

    }

    TspClient::~TspClient() {
        destroy();
    }

    void TspClient::update_config(const tsp_client::tls_tcp_config &config){
        tsp_client_config_ = config;
        reload_cfg_ = true;
    }

    bool TspClient::connect(std::int32_t max_reconnects) {
        TB_LOG_INFO("TspClient::connect\n");

        if(tls_client_ == nullptr) {
            tls_client_ = std::make_unique<tsp_client::client>(tsp_client_config_);
        }else {
            if(reload_cfg_) {
                 tls_client_->disconnect();
                 tls_client_.reset(nullptr);
                 tls_client_ = std::make_unique<tsp_client::client>(tsp_client_config_);
                 reload_cfg_ = false;
            }
        }
        run(max_reconnects);
        return tls_client_->is_connected();
    }

    void TspClient::disconnect() {
        if(tls_client_ != nullptr) {
            tls_client_->disconnect();
        }
    }

    bool TspClient::is_connected() {
        if(tls_client_ != nullptr) {
            return tls_client_->is_connected();
        }
        return false;
    }

    void TspClient::run(std::int32_t max_reconnects) {
        TB_LOG_INFO("TspClient::run\n");
        tls_client_->connect(tsp_client_config_.server_ip, tsp_client_config_.port, 
            [this](const std::string& host, std::size_t port, connect_state_t status) {
                on_connection_changed(host, port, status);
            },
            [this](const std::vector<uint8_t>& message) {
                on_message_arrive(message);
            }, 0, max_reconnects);
        tls_client_->set_message_published_handle([this](const std::vector<uint8_t>& message, bool published){
            on_message_published(message, published);
        });
    }

    bool TspClient::destroy() {
        TB_LOG_INFO("TspClient destroy\n");
        if(tls_client_) {
             tls_client_->disconnect();
             tls_client_.reset(nullptr);
        }
        return true;
    }

    void TspClient::publish(const std::string &topic, const std::vector<uint8_t> &message) {
        //TB_LOG_INFO("TspClient::publish topic:%s\n", topic.c_str());
        if(tls_client_ != nullptr) {
            tls_client_->send(message);
        }
    }

    void TspClient::set_connection_changed_handle(const connect_callback_t &con_callback) {
        conn_callback_ = con_callback;
    }

    void TspClient::set_message_received_handle(const reply_callback_t &reply_callback) {
       TB_LOG_INFO("TspClient::set_message_received_handle");
       reply_callback_ = reply_callback;
    }

    void TspClient::set_message_published_handle(const published_callback_t &published_callback) {
       published_callback_ = published_callback;
    }

    void TspClient::on_connection_changed(const std::string& host, std::size_t port, connect_state_t conn_state) {
        TB_LOG_INFO("TspClient::on_connection_changed %s:%d conn_state:%d \n", 
            host.c_str(), port, conn_state);
        if(conn_callback_ != nullptr) {
           conn_callback_(host, port, conn_state);
        }else{
            TB_LOG_ERROR("TspClient::on_connection_changed got null conn_callback_\n");
        }
    }
    void TspClient::on_message_arrive(const std::vector<uint8_t>& message) {
        //TB_LOG_INFO("TspClient::on_message_arrive");
        if(reply_callback_ != nullptr) {
            reply_callback_(message);
        }else{
            TB_LOG_ERROR("TspClient::on_message_arrive got null reply_callback_\n");
        }
    }

    void TspClient::on_message_published(const std::vector<uint8_t>& message, bool published) {
        if(published_callback_ != nullptr) {
            published_callback_(message, published);
        }
    }
}// namespace tsp_client