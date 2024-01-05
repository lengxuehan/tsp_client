#include "include/client/client.h"
#include <iostream>
#include <csignal>
#include "tb_log.h"

namespace tsp_client {

    client::client(const tls_tcp_config& ssl_cfg)
            :client_connection_(ssl_cfg) {
        TB_LOG_INFO("tsp::client created\n");
        sender_thread_ = std::thread{&client::thread_send_message_entry, this};
    }

    client::~client(void) {
        //! ensure we stopped reconnection attempts
        if (!cancel_) {
            cancel_reconnect();
        }

        exit_requested_.store(true);
        cv_sender_.notify_one();
        if(sender_thread_.joinable()){
            sender_thread_.join();
        }

        //! disconnect underlying tcp socket
        if (client_connection_.is_connected()) {
            client_connection_.disconnect();
        }

        TB_LOG_INFO("tsp::client destroyed\n");
    }

    void client::connect(
            const std::string &host, std::size_t port,
            const connect_callback_t &connect_callback,
            const reply_callback_t &reply_callback,
            std::uint32_t timeout_msecs,
            std::int32_t max_reconnects,
            std::uint32_t reconnect_interval_msecs) {
        TB_LOG_INFO("tsp::client attempts to connect\n");

        //! Save for auto reconnects
        server_ip_ = host;
        server_port_ = port;
        connect_callback_ = connect_callback;
        reply_callback_ = reply_callback;
        max_reconnects_ = max_reconnects;
        reconnect_interval_msecs_ = reconnect_interval_msecs;

        //! notify start
        if (connect_callback_) {
            connect_callback_(host, port, connect_state::start);
        }

        auto disconnection_handler = std::bind(&client::connection_disconnection_handler, this,
                                               std::placeholders::_1);
        auto receive_handler = std::bind(&client::connection_receive_handler, this, std::placeholders::_1,
                                         std::placeholders::_2);
        bool connected = client_connection_.connect(host, port, disconnection_handler, receive_handler);

        if (connected) {
            //! notify end
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state::ok);
            }
        } else {
            //! notify end
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state::failed);
            }
        }
    }

    void client::disconnect() {
        std::cout << "" << server_ip_ << " port:"
                  << server_port_ << std::endl;
        TB_LOG_INFO("tsp::client attempts to disconnect ip:%s port:%d\n",
                    server_ip_.c_str(), server_port_);

        //! close connection
        client_connection_.disconnect();

        //! make sure we clear buffer of unsent commands
        clear_callbacks();

        TB_LOG_INFO("tsp::client disconnected\n");
    }

    bool client::is_connected(void) const {
        return client_connection_.is_connected();
    }

    void client::cancel_reconnect(void) {
        cancel_ = true;
    }

    bool client::is_reconnecting(void) const {
        return reconnecting_;
    }

    client &client::send(const std::vector<uint8_t> &request, const reply_callback_t &callback) {
        std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

        TB_LOG_INFO("tsp::client attempts to store new command in the send buffer\n");
        unprotected_send(request, callback);
        TB_LOG_INFO("tsp::client stored new command in the send buffer\n");

        return *this;
    }

    client &client::send(const std::vector<uint8_t> &request) {
        {
            std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

            unprotected_send(request, [this](const std::vector<uint8_t> &message)->void{
                handle_message(message);
            });
        }

        cv_sender_.notify_one();
        return *this;
    }

    void client::unprotected_send(const std::vector<uint8_t> &request, const reply_callback_t &callback) {
        commands_.push({request, callback});
    }


    void client::connection_receive_handler(client_connection &, const std::vector<uint8_t> &reply) {
        reply_callback_t callback = nullptr;

        std::cout << "tsp::client received reply" << std::endl;
        {
            std::lock_guard<std::mutex> lock(callbacks_mutex_);
            callbacks_running_ += 1;

            if (commands_.size()) {
                callback = commands_.front().callback;
                commands_.pop();
            }
        }

        if (callback) {
            std::cout << "tsp::client executes reply callback" << std::endl;
            callback(reply);
        }

        {
            std::lock_guard<std::mutex> lock(callbacks_mutex_);
            callbacks_running_ -= 1;
        }
    }

    void client::clear_callbacks(void) {
        if (commands_.empty()) {
            return;
        }

        //! dequeue commands and move them to a local variable
        std::queue<command_request> commands = std::move(commands_);

        callbacks_running_ += commands.size();

        std::thread t([=]() mutable {
            while (!commands.empty()) {
                const auto &callback = commands.front().callback;

                if (callback) {
                    //reply r = {"network failure", reply::string_type::error};
                    //callback(r);
                }

                --callbacks_running_;
                commands.pop();
            }
        });
        t.detach();
    }

    void client::handle_message(const std::vector<uint8_t> &message){
        TB_LOG_INFO("client::handle_message size:%d\n", message.size());
        if(reply_callback_ != nullptr) {
            reply_callback_(message);
        }
    }

    void client::thread_send_message_entry() {
        // 屏蔽信号
        sigset_t signals;
        (void)sigfillset(&signals);
        (void)pthread_sigmask(SIG_SETMASK, &signals, nullptr);
        do {
            {// Get lock
                std::unique_lock<std::mutex> lock{sender_mutex_};
                cv_sender_.wait(lock, [this]()->bool {
                    return ((!commands_.empty()) || exit_requested_.load()); });
                if (exit_requested_.load()) {
                    return; // Exit thread
                }
                auto& request_cmd = commands_.front();
                std::vector<uint8_t> msg = std::move(request_cmd.command);
                bool res = client_connection_.send(std::move(msg));
                if(res) {
                    commands_.pop();
                    // notify msg send successful
                    TB_LOG_INFO("send message successful\n");
                }
            }// Release lock
        } while (true);
    }

    void client::resend_failed_commands(void) {
        TB_LOG_INFO("client::resend_failed_commands count:%d\n", commands_.size());

        //! dequeue commands and move them to a local variable
        std::queue<command_request> commands;
        {
            std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);
            if (commands_.empty()) {
                return;
            }
            commands = std::move(commands_);
        }

        while (commands.size() > 0) {
            //! Reissue the pending command and its callback.
            auto& msg = commands.front().command;
            bool res = client_connection_.send(std::move(msg));
            if(res) {
                // notify msg send successful
                TB_LOG_INFO("send message successful\n");
            }
            commands.pop();
        }
    }

    void client::connection_disconnection_handler(client_connection &connection) {
        //! leave right now if we are already dealing with reconnection
        if (is_reconnecting()) {
            return;
        }

        //! initiate reconnection process
        reconnecting_ = true;
        current_reconnect_attempts_ = 0;

        std::cout << "tsp::client has been disconnected" << std::endl;

        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state::dropped);
        }

        //! Lock the callbacks mutex of the base class to prevent more client commands from being
        //! issued until our reconnect has completed.
        //! std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

        while (should_reconnect()) {
            sleep_before_next_reconnect_attempt();
            reconnect();
        }

        if (!is_connected()) {
            clear_callbacks();

            //! Tell the user we gave up!
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state::stopped);
            }
        }

        //! terminate reconnection
        reconnecting_ = false;
    }

    void client::sleep_before_next_reconnect_attempt(void) {
        if (reconnect_interval_msecs_ <= 0) {
            return;
        }

        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state::sleeping);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_interval_msecs_));
    }

    bool client::should_reconnect(void) const {
        return !is_connected() && !cancel_ &&
               (max_reconnects_ == -1 || current_reconnect_attempts_ < max_reconnects_);
    }

    void client::unprotected_auth(const std::string &password, const reply_callback_t &reply_callback) {
        //! save the password for reconnect attempts.
        //! store command in pipeline
        unprotected_send({"AUTH", "passwd"}, reply_callback);
    }

    void client::re_auth(void) {
        //if (m_password.empty()) {
        //    return;
        //}

//        unprotected_auth("password", [&](tsp::reply &reply) {
//            if (reply.is_string() && reply.as_string() == "OK") {
//                std::cout << "client successfully re-authenticated";
//            } else {
//                std::cout << std::string("client failed to re-authenticate: " + reply.as_string()).c_str();
//            }
//        });
    }


    void client::reconnect(void) {
        //! increase the number of attempts to reconnect
        ++current_reconnect_attempts_;

        //! Try catch block because the redis client throws an error if connection cannot be made.
        try {
            connect(server_ip_, server_port_, connect_callback_, reply_callback_, connect_timeout_msecs_, max_reconnects_,
                    reconnect_interval_msecs_);
        }
        catch (...) {
        }

        if (!is_connected()) {
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state::failed);
            }
            return;
        }

        //! notify end
        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state::ok);
        }

        TB_LOG_INFO("client reconnected ok\n");

        // re_auth(); // user might do this on connection call back handler
        resend_failed_commands();
    }

} // namespace tsp_client
