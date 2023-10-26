/**
* @file client.h
* @brief client is the class providing communication with a tsp server.
* @author		wuting.xu
* @date		    2023/10/23
* @par Copyright(c): 	2023 megatronix. All rights reserved.
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
#include "client_tcp_iface.h"
#include "client_connection.h"


namespace tsp_client {

//!
//! tsp_client::client is the class providing communication with a tsp server.
//! It is meant to be used for sending requests to the remote server and receiving its replies.
    class client {
    public:
        //!
        //! high availability (re)connection states
        //!  * dropped: connection has dropped
        //!  * start: attempt of connection has started
        //!  * sleeping: sleep between two attempts
        //!  * ok: connected
        //!  * failed: failed to connect
        //!  * stopped: stop to try to reconnect
        //!
        enum class connect_state {
            dropped,
            start,
            sleeping,
            ok,
            failed,
            stopped
        };

    public:
        //! ctor
        client(const ssl_config& ssl_cfg);

        //! dtor
        ~client(void);

        //! copy ctor
        client(const client &) = delete;

        //! assignment operator
        client &operator=(const client &) = delete;

    public:
        //!
        //! connect handler, called whenever a new connection even occurred
        //!
        typedef std::function<void(const std::string &host, std::size_t port, connect_state status)> connect_callback_t;

        //!
        //! Connect to redis server
        //!
        //! \param host host to be connected to
        //! \param port port to be connected to
        //! \param connect_callback connect handler to be called on connect events (may be null)
        //! \param timeout_msecs maximum time to connect
        //! \param max_reconnects maximum attempts of reconnection if connection dropped
        //! \param reconnect_interval_msecs time between two attempts of reconnection
        //!
        void connect(
                const std::string &host = "127.0.0.1",
                std::size_t port = 8888,
                const connect_callback_t &connect_callback = nullptr,
                std::uint32_t timeout_msecs = 0,
                std::int32_t max_reconnects = 0,
                std::uint32_t reconnect_interval_msecs = 0);

        //!
        //! \return whether we are connected to the redis server
        //!
        bool is_connected(void) const;

        //!
        //! disconnect from redis server
        //!
        void disconnect();

        //!
        //! \return whether an attempt to reconnect is in progress
        //!
        bool is_reconnecting(void) const;

        //!
        //! stop any reconnect in progress
        //!
        void cancel_reconnect(void);

    public:
        //!
        //! reply callback called whenever a reply is received
        //! takes as parameter the received reply
        //!
        typedef std::function<void(const std::vector<uint8_t> &)> reply_callback_t;

        //!
        //! send the given command
        //! the command is actually pipelined and only buffered, so nothing is sent to the network
        //! please call commit() / sync_commit() to flush the buffer
        //!
        //! \param redis_cmd command to be sent
        //! \param callback callback to be called on received reply
        //! \return current instance
        //!
        client &send(const std::vector<uint8_t> &request, const reply_callback_t &callback);

        //!
        //! same as the other send method
        //! but future based: does not take any callback and return an std:;future to handle the reply
        //!
        //! \param redis_cmd command to be sent
        //! \return std::future to handler redis reply
        //!
        std::future<std::vector<uint8_t>> send(const std::vector<uint8_t> &redis_cmd);

    private:
        //!
        //! \return whether a reconnection attempt should be performed
        //!
        bool should_reconnect(void) const;

        //!
        //! resend all pending commands that failed to be sent due to disconnection
        //!
        void resend_failed_commands(void);

        //!
        //! sleep between two reconnect attempts if necessary
        //!
        void sleep_before_next_reconnect_attempt(void);

        //!
        //! reconnect to the previously connected host
        //! automatically re authenticate and resubscribe to subscribed channel in case of success
        //!
        void reconnect(void);

        //!
        //! re authenticate to redis server based on previously used password
        //!
        void re_auth(void);

    private:
        //!
        //! unprotected send
        //! same as send, but without any mutex lock
        //!
        //! \param redis_cmd cmd to be sent
        //! \param callback callback to be called whenever a reply is received
        //!
        void unprotected_send(const std::vector<uint8_t> &request, const reply_callback_t &callback);

        //!
        //! unprotected auth
        //! same as auth, but without any mutex lock
        //!
        //! \param password password to be used for authentication
        //! \param reply_callback callback to be called whenever a reply is received
        //!
        void unprotected_auth(const std::string &password, const reply_callback_t &reply_callback);


    private:
        //!
        //! redis connection receive handler, triggered whenever a reply has been read by the redis connection
        //!
        //! \param connection redis_connection instance
        //! \param reply parsed reply
        //!
        void connection_receive_handler(client_connection &connection, const std::vector<uint8_t> & reply);

        //!
        //! redis_connection disconnection handler, triggered whenever a disconnection occurred
        //!
        //! \param connection redis_connection instance
        //!
        void connection_disconnection_handler(client_connection &connection);

        //!
        //! reset the queue of pending callbacks
        //!
        void clear_callbacks(void);

    private:
        //!
        //! struct to store commands information (command to be sent and callback to be called)
        //!
        struct command_request {
            std::vector<uint8_t> command;
            reply_callback_t callback;
        };

    private:
        //!
        //! server ip we are connected to
        //!
        std::string server_ip_{};
        //!
        //! port we are connected to
        //!
        std::size_t server_port_{0};

        //!
        //! tcp client for redis connection
        //!
        client_connection client_connection_;

        //!
        //! max time to connect
        //!
        std::uint32_t connect_timeout_msecs_{0};
        //!
        //! max number of reconnection attempts
        //!
        std::int32_t max_reconnects_{0};
        //!
        //! current number of attempts to reconnect
        //!
        std::int32_t current_reconnect_attempts_{0};
        //!
        //! time between two reconnection attempts
        //!
        std::uint32_t reconnect_interval_msecs_{0};

        //!
        //! reconnection status
        //!
        std::atomic_bool reconnecting_{false};
        //!
        //! to force cancel reconnection
        //!
        std::atomic_bool cancel_{false};

        //!
        //! sent commands waiting to be executed
        //!
        std::queue<command_request> commands_;

        //!
        //! user defined connect status callback
        //!
        connect_callback_t connect_callback_;

        //!
        //!  callbacks thread safety
        //!
        std::mutex callbacks_mutex_;

        //!
        //! condvar for callbacks updates
        //!
        std::condition_variable sync_condvar_;

        //!
        //! number of callbacks currently being running
        //!
        std::atomic<unsigned int> callbacks_running_{0};
    };
}// namespace tsp_client