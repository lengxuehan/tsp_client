#include <iostream>
#include <csignal>
#include "client/client.h"
#include "tb_log.h"
#include "packages/messages.h"

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
        TB_LOG_INFO("client::connect attempts to connect\n");
        can_be_published_ = false;
        //! Save for auto reconnects
        server_ip_ = host;
        server_port_ = port;
        connect_callback_ = connect_callback;
        reply_callback_ = reply_callback;
        max_reconnects_ = max_reconnects;
        reconnect_interval_msecs_ = reconnect_interval_msecs;

        /*//! notify start
        if (connect_callback_) {
            connect_callback_(host, port, connect_state_t::start);
        }*/

        auto disconnection_handler = std::bind(&client::connection_disconnection_handler, this,
                                               std::placeholders::_1);
        auto receive_handler = std::bind(&client::connection_receive_handler, this, std::placeholders::_1,
                                         std::placeholders::_2);
        bool connected = client_connection_.connect(host, port, disconnection_handler, receive_handler);

        if (connected) {
            //! notify end
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state_t::ok);
            }
            can_be_published_ = true;
        } else {
            //! notify end
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state_t::failed);
            }
        }
    }

    void client::set_message_published_handle(const published_callback_t &published_callback) {
        published_callback_ = published_callback;
    }

    void client::disconnect() {
        TB_LOG_INFO("tsp::client attempts to disconnect ip:%s port:%d\n", server_ip_.c_str(), server_port_);

        can_be_published_ = false;

        //! close connection
        client_connection_.disconnect();

        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state_t::dropped);
        }

        //! make sure we clear buffer of unsent commands
        // clear_callbacks();
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
        {
            std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

            unprotected_send(request, [this](const std::vector<uint8_t> &message)->void{
                handle_message(message);
            });
        }

        cv_sender_.notify_one();

        return *this;
    }

    void client::send(const std::vector<uint8_t> &request) {
        {
            std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

            unprotected_send(request, [this](const std::vector<uint8_t> &message)->void{
                handle_message(message);
            });
        }

        cv_sender_.notify_one();
    }

    void client::unprotected_send(const std::vector<uint8_t> &request, const reply_callback_t &callback) {
        commands_.push({request, callback});
    }

    void client::connection_receive_handler(client_connection &, const std::vector<uint8_t> &reply) {
        /*reply_callback_t callback{nullptr};

        {
            std::lock_guard<std::mutex> lock(callbacks_mutex_);
            callbacks_running_ += 1;

            if (commands_.size()) {
                callback = commands_.front().callback;
                commands_.pop();
            }
        }

        if (callback) {
            callback(reply);
        } else {
            TB_LOG_ERROR("tsp::client executes reply got empty callback\n");
        }

        {
            std::lock_guard<std::mutex> lock(callbacks_mutex_);
            callbacks_running_ -= 1;
        }*/
        handle_message(reply);
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
                if(!can_be_published_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                    TB_LOG_ERROR("client::thread_send_message_entry wait to connect to remote host ip:%s\n",
                                 client_connection_.get_remote_host_ip().c_str());
                    continue;
                }
                auto& request_cmd = commands_.front();
                std::vector<uint8_t> msg = std::move(request_cmd.command);
                std::vector<uint8_t> tmp_msg;
                if(published_callback_ != nullptr) {
                    tmp_msg = msg;
                }
                bool res = client_connection_.send(std::move(msg));
                if(res) {
                    commands_.pop();
                    // notify msg send successful
                    if(published_callback_ != nullptr) {
                        published_callback_(tmp_msg, true);
                    }
                } else {
                    can_be_published_ = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                    TB_LOG_ERROR("client::thread_send_message_entry send msg failed remote host ip:%s\n",
                                 client_connection_.get_remote_host_ip().c_str());
                    if(max_reconnects_ == 0) { // no try reconnect
                        if(published_callback_ != nullptr) {
                            published_callback_(tmp_msg, false);
                        }
                    }
                }
            }// Release lock
        } while (true);
    }

    void client::resend_failed_commands(void) {
        if (commands_.empty()) {
            return;
        }

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
            std::vector<uint8_t> tmp_msg;
            if(published_callback_ != nullptr) {
                tmp_msg = msg;
            }
            bool res = client_connection_.send(std::move(msg));
            if(!res) {
                TB_LOG_ERROR("client::resend_failed_commands failed, drop message\n");
                if(published_callback_ != nullptr) {
                    published_callback_(tmp_msg, false);
                }
            } else {
                if(published_callback_ != nullptr) {
                    published_callback_(tmp_msg, true);
                }
            }
            commands.pop();
        }
    }

    void client::connection_disconnection_handler(client_connection &connection) {
        can_be_published_ = false;
        //! leave right now if we are already dealing with reconnection
        if (is_reconnecting()) {
            return;
        }

        //! initiate reconnection process
        reconnecting_ = true;
        current_reconnect_attempts_ = 0;

        TB_LOG_INFO("tsp::connection_disconnection_handler has been disconnected\n");

        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state_t::dropped);
        }

        //! Lock the callbacks mutex of the base class to prevent more client commands from being
        //! issued until our reconnect has completed.
        //! std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

        TB_LOG_INFO("tsp::connection_disconnection_handler try to reconnect\n");
        while (should_reconnect()) {
            sleep_before_next_reconnect_attempt();
            reconnect();
        }

        if (!is_connected()) {
            //clear_callbacks();

            //! Tell the user we gave up!
            TB_LOG_INFO("tsp::connection_disconnection_handler gave up to reconnect\n");
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state_t::stopped);
            }
        }

        //! terminate reconnection
        TB_LOG_INFO("tsp::connection_disconnection_handler terminate reconnection\n");
        reconnecting_ = false;
    }

    void client::sleep_before_next_reconnect_attempt(void) {
        if (reconnect_interval_msecs_ <= 0) {
            return;
        }

        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state_t::sleeping);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_interval_msecs_));
    }

    bool client::should_reconnect(void) const {
        TB_LOG_INFO("client::should_reconnect connected:%d cancel:%d max_reconnects:%d\n", is_connected(),
                    cancel_.load(), max_reconnects_);
        return !is_connected() && !cancel_ &&
               (max_reconnects_ == -1 || current_reconnect_attempts_ < max_reconnects_);
    }

    void client::re_login(void) {
    }


    void client::reconnect(void) {
        TB_LOG_INFO("client::reconnect attempts to connect current reconnect attempts:%d\n", current_reconnect_attempts_);
        //! increase the number of attempts to reconnect
        ++current_reconnect_attempts_;

        //! Try catch block because the redis client throws an error if connection cannot be made.
        try {
            connect(server_ip_, server_port_, connect_callback_, reply_callback_, connect_timeout_msecs_,
                    max_reconnects_, reconnect_interval_msecs_);
        }
        catch (...) {
        }

        if (!is_connected()) {
            if (connect_callback_) {
                connect_callback_(server_ip_, server_port_, connect_state_t::failed);
            }
            return;
        }

        //! notify end
        if (connect_callback_) {
            connect_callback_(server_ip_, server_port_, connect_state_t::ok);
        }

        TB_LOG_INFO("client reconnected ok\n");

        //re_login(); // user might do this on connection call back handler
        resend_failed_commands();
    }

} // namespace tsp_client
