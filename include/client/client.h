/**
* @file client.h
* @brief client is the class providing communication with a tsp server.
* @author   wuting.xu
* @date     2023/10/23
* @par Copyright(c):    2023 megatronix. All rights reserved.
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include "client_iface.h"
#include "client_connection.h"

namespace tsp_client {
/**
 * @brief tsp_client::client is the class providing communication with a tsp server.
 *
 * It is meant to be used for sending requests to the remote server and receiving its replies.
 */
    class client : public client_iface {
    public:
        typedef std::function<void(const std::string&, std::size_t, connect_state_t)> connect_callback_t;
        typedef std::function<void(const std::vector<uint8_t> &)> reply_callback_t;
        typedef std::function<void(const std::vector<uint8_t> &, bool)> published_callback_t;
    public:
        //! ctor
        client(const tls_tcp_config& tls_tcp_cfg);

        //! dtor
        ~client(void);

        //! copy ctor
        client(const client &) = delete;

        //! assignment operator
        client &operator=(const client &) = delete;

    public:
        /**
         * @brief Connect to tsp server
         *
         * @param host host to be connected to
         * @param port port to be connected to
         * @param connect_callback connect handler to be called on connect events (may be null)
         * @param reply_callback reply handler to be called on reply events (may be null)
         * @param timeout_msecs maximum time to connect
         * @param max_reconnects maximum attempts of reconnection if connection dropped
         * @param reconnect_interval_msecs  time between two attempts of reconnection
         */
        void connect(
                const std::string &host = "127.0.0.1",
                std::size_t port = 8888,
                const connect_callback_t &connect_callback = nullptr,
                const reply_callback_t &reply_callback = nullptr,
                std::uint32_t timeout_msecs = 0,
                std::int32_t max_reconnects = 0,
                std::uint32_t reconnect_interval_msecs = 500) override;

        /**
         * @brief whether we are connected to the redis server
         *
         */
        bool is_connected(void) const override;

        /**
         * @brief disconnect from tsp server
         *
         */
        void disconnect() override;

        /**
         * @brief whether an attempt to reconnect is in progress
         *
         */
        bool is_reconnecting(void) const;

        /**
         * @brief stop any reconnect in progress
         *
         */
        void cancel_reconnect(void);

    public:
        /**
         * @brief send the given command
         * the command is actually pipelined and only buffered, so nothing is sent to the network
         *
         * @param request command to be sent
         * @param callback  callback to be called on received reply
         * @return client& current instance
         */
        client &send(const std::vector<uint8_t> &request, const reply_callback_t &callback);

        /**
         * @brief send the given command
         * the command is actually pipelined and only buffered, so nothing is sent to the network
         *
         * @param request request to be sent
         */
        void send(const std::vector<uint8_t> &request) override;

        void set_message_published_handle(const published_callback_t &published_callback);
    private:
        /**
         * @brief whether a reconnection attempt should be performed
         *
         */
        bool should_reconnect(void) const;

        /**
         * @brief resend all pending commands that failed to be sent due to disconnection
         *
         */
        void resend_failed_commands(void);

        /**
         * @brief sleep between two reconnect attempts if necessary
         *
         */
        void sleep_before_next_reconnect_attempt(void);

        /**
         * @brief reconnect to the previously connected host
         * automatically re authenticate and resubscribe to subscribed channel in case of success
         */
        void reconnect(void);

        //!
        //!
        //!
        /**
         * @brief re authenticate to tsp server based on previously used login message
         *
         */
        void re_login(void);

    private:
        /**
         * @brief unprotected send, ame as send, but without any mutex lock
         *
         * @param request cmd to be sent
         * @param callback callback to be called whenever a reply is received
         */
        void unprotected_send(const std::vector<uint8_t> &request, const reply_callback_t &callback);


    private:
        /**
         * @brief connection receive handler, triggered whenever a reply has been read by the connection
         *
         * @param connection connection instance
         * @param reply  parsed reply
         */
        void connection_receive_handler(client_connection &connection, const std::vector<uint8_t> & reply);

        /**
         * @brief connection disconnection handler, triggered whenever a disconnection occurred
         *
         * @param connection connection instance
         */
        void connection_disconnection_handler(client_connection &connection);

        /**
         * @brief reset the queue of pending callbacks
         *
         */
        void clear_callbacks(void);

    private:
        /**
         * @brief struct to store commands information (command to be sent and callback to be called)
         *
         */
        struct command_request {
            std::vector<uint8_t> command;
            reply_callback_t callback;
        };

        void handle_message(const std::vector<uint8_t>& message);

        void thread_send_message_entry();

    private:
        /**
         * @brief server ip we are connected to
         *
         */
        std::string server_ip_{};
        /**
         * @brief port we are connected to
         *
         */
        std::size_t server_port_{0};
        /**
         * @brief tcp client for redis connection
         *
         */
        client_connection client_connection_;
        /**
         * @brief max time to connect
         *
         */
        std::uint32_t connect_timeout_msecs_{0};
        /**
         * @brief max number of reconnection attempts
         *
         */
        std::int32_t max_reconnects_{0};
        /**
         * @brief current number of attempts to reconnect
         *
         */
        std::int32_t current_reconnect_attempts_{0};
        /**
         * @brief time between two reconnection attempts
         *
         */
        std::uint32_t reconnect_interval_msecs_{0};

        /**
         * @brief reconnection status
         *
         */
        std::atomic_bool reconnecting_{false};

        /**
         * @brief to force cancel reconnection
         *
         */
        std::atomic_bool cancel_{false};

        /**
         * @brief sent commands waiting to be executed
         *
         */
        std::queue<command_request> commands_;

        /**
         * @brief user defined connect status callback
         *
         */
        connect_callback_t connect_callback_;

        /**
         * @brief user defined reply callback
         *
         */
        reply_callback_t reply_callback_;

        /**
         * @brief user defined on request sent
         *
         */
        mutable published_callback_t published_callback_;

        /**
         * @brief callbacks thread safety
         *
         */
        std::mutex callbacks_mutex_;

        /**
         * @brief number of callbacks currently being running
         *
         */
        std::atomic<unsigned int> callbacks_running_{0};

        std::thread sender_thread_;
        std::mutex sender_mutex_;
        std::condition_variable cv_sender_;
        std::atomic_bool exit_requested_{false};
        std::atomic_bool can_be_published_{false};

    };
}// namespace tsp_client