#include "include/client/client_connection.h"
#include "client_tcp_socket.h"
#include <iostream>

namespace tsp_client {

    client_connection::client_connection(const ssl_config& ssl_cfg)
            : client_(std::make_shared<client_tcp_socket>(ssl_cfg)){
    }

    client_connection::~client_connection(void) {
        client_->disconnect();
        std::cout << "tsp_client_::connection destroyed" << std::endl;
    }

    bool client_connection::connect(const std::string &host, std::size_t port,
                                    const disconnection_handler_t &client_disconnection_handler,
                                    const reply_callback_t &client_reply_callback) {
        try {
            std::cout << "client_connection::connect attempts to connect host:" << host
            <<" port:" << port << std::endl;

            //! connect client
            bool res = client_->connect(host, (uint32_t) port);
            client_->set_on_disconnection_handler(
                    std::bind(&client_connection::tcp_client_disconnection_handler, this));
            if(res){
                std::cout << "client_connection::connect successful to connect remote server" << std::endl;
            }else{
                std::cout << "client_connection::connect failed to connect remote server" << std::endl;
            }

            return res;
        }
        catch (const std::exception &e) {
            std::cout << std::string("client_connection::connect ") + e.what() << std::endl;
            throw std::exception(e);
        }

        // TODO implement me
        reply_callback_ = client_reply_callback;
        disconnection_handler_ = client_disconnection_handler;
    }

    void client_connection::disconnect() {
        std::cout << "client_connection::disconnect attempts to disconnect" << std::endl;

        //! close connection
        client_->disconnect();
        std::cout << "client_connection::disconnect disconnected" << std::endl;
    }

    bool client_connection::is_connected(void) const {
        return client_->is_connected();
    }

    client_connection &client_connection::send(std::vector<uint8_t> &&request) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);

        std::cout << "tsp_client_::send attempts to send request" << std::endl;

        //client_tcp_iface::write_request request = {std::vector<char>{buffer.begin(), buffer.end()}, nullptr};
        client_->send(std::move(request));

        std::cout << "client_connection::send end to sent request";

        return *this;
    }

    void client_connection::call_disconnection_handler(void) {
        if (disconnection_handler_) {
            std::cout << "client_connection::call_disconnection_handler calls disconnection handler" << std::endl;
            disconnection_handler_(*this);
        }
    }

    void client_connection::tcp_client_receive_handler(const std::vector<uint8_t> &response) {

        try {
            std::cout << "client_connection::connection tcp_client_receive_handler packet, "
                         "attempts to build reply" << std::endl;
            // TODO parse packet
            //m_builder << std::string(result.buffer.begin(), result.buffer.end());
        }
        catch (const std::exception &e) {
            std::cout << "tsp_client_::connection could not build reply (invalid format), "
                         "disconnecting," << e.what() << std::endl;
            call_disconnection_handler();
            return;
        }
        reply_callback_(*this, response);
        /*
        while(m_builder.reply_available()) {
            std::cout << "tsp_client_::connection reply fully built";

            auto reply = m_builder.get_front();
            m_builder.pop_front();

            if (m_reply_callback) {
                std::cout << "tsp_client_::connection executes reply callback";
                m_reply_callback(*this, reply);
            }
        }*/
    }

    void client_connection::tcp_client_disconnection_handler(void) {
        std::cout << "tsp_client::connection has been disconnected" << std::endl;
        //! call disconnection handler
        call_disconnection_handler();
    }

} // namespace tsp_client
