/**
* @file cloud_proxy_adapter.h
* @brief CloudProxy Interface definition
* @author   wuting.xu
* @date     2023/11/17
* @par Copyright(c):    2023 megatronix. All rights reserved.
*/

#pragma once

#include <string>
#include <memory>
#include "client_tcp_iface.h"

/**
 * @brief CloudProxy makes it easy to communicate with remote tsp server directly by functions call
 */
namespace tsp_client {
    class TspClient
    {
    public:
        /**
         * @brief ctor
         */
        TspClient(const std::string& str_config_path);
        /**
         * @brief ctor
         */
        TspClient(const tsp_client::tls_tcp_config &config);

        /**
         * @brief dtor
         */
        virtual ~TspClient();

        void update_config(const tsp_client::tls_tcp_config &config);

    public:

        void publish(const std::string &topic, const std::vector<uint8_t> &message);

        void set_connection_changed_handle(const connect_callback_t &con_callback);

        void set_message_received_handle(const reply_callback_t &reply_callback);

        void set_message_published_handle(const published_callback_t &published_callback);

        /**
         * @brief connect to tsp server
         */
        bool connect(std::int32_t max_reconnects = 0);
        /**
         * @brief disconnect from tsp server
         */
        void disconnect();

        bool is_connected();
    private:
        void on_connection_changed(const std::string& host, std::size_t port, connect_state_t conn_state);
        void on_message_arrive(const std::vector<uint8_t>& message);
        void on_message_published(const std::vector<uint8_t>& message, bool published);

    private:
        void run(std::int32_t max_reconnects);
        bool destroy();

    private:
        connect_callback_t conn_callback_{nullptr};
        reply_callback_t reply_callback_{nullptr};
        published_callback_t published_callback_{nullptr};
        tsp_client::tls_tcp_config tsp_client_config_;
        std::unique_ptr<tsp_client::client_iface> tls_client_{nullptr};
        bool reload_cfg_{false};
    };
}// namespace tsp_client
