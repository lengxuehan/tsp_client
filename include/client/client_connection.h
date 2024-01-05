/**
* @file client.h
* @brief client_connection tcp connection wrapper handling protocol based on tcp.
* @author		wuting.xu
* @date		    2023/10/23
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "client/client_tcp_iface.h"

namespace tsp_client {
    class client_connection {
    public:
        //! ctor
        client_connection(const tls_tcp_config& tls_tcp_cfg);

        //! dtor
        ~client_connection(void);

        //! copy ctor
        client_connection(const client_connection &) = delete;

        //! assignment operator
        client_connection &operator=(const client_connection &) = delete;

    public:
        //!
        //! disconnection handler takes as parameter the instance of the connection
        //!
        typedef std::function<void(client_connection &)> disconnection_handler_t;

        //!
        //! reply handler takes as parameter the instance of the connection and the built reply
        //!
        typedef std::function<void(client_connection&, const std::vector<uint8_t>&)> reply_callback_t;

        //!
        //! connect to the given host and port, and set both disconnection and reply callbacks
        //!
        //! \param host host to be connected to
        //! \param port port to be connected to
        //! \param disconnection_handler handler to be called in case of disconnection
        //! \param reply_callback handler to be called once a reply is ready
        //!
        //! \return whether we are connected to the redis server or not
        //!
        bool connect(
                const std::string &host = "127.0.0.1",
                std::size_t port = 8888,
                const disconnection_handler_t &disconnection_handler = nullptr,
                const reply_callback_t &reply_callback = nullptr);

        //!
        //! disconnect from redis server
        //!
        //! \param wait_for_removal when sets to true, disconnect blocks until the underlying TCP client has been effectively removed from the io_service and that all the underlying callbacks have completed.
        //!
        void disconnect();

        //!
        //! \return whether we are connected to the redis server or not
        //!
        bool is_connected(void) const;

        //!
        //! send the given command
        //! the command is actually pipelined and only buffered, so nothing is sent to the network
        //! please call commit() to flush the buffer
        //!
        //! \param redis_cmd command to be sent
        //! \return current instance
        //!
        bool send(std::vector<uint8_t> &&request);

    private:
        //!
        //! tcp_client receive handler
        //! called by the tcp_client whenever a read has completed
        //!
        //! \param result read result
        //!
        bool tcp_client_receive_handler(const std::vector<uint8_t> &response);

        //!
        //! tcp_client disconnection handler
        //! called by the tcp_client whenever a disconnection occurred
        //!
        void tcp_client_disconnection_handler(void);

    private:
        //!
        //! simply call the disconnection handler (does nothing if disconnection handler is set to null)
        //!
        void call_disconnection_handler(void);

    private:
        //!
        //! tcp client for redis connection
        //!
        std::shared_ptr<client_tcp_iface> client_{nullptr};

        //!
        //! reply callback called whenever a reply has been read
        //!
        reply_callback_t reply_callback_;

        //!
        //! disconnection handler whenever a disconnection occurred
        //!
        disconnection_handler_t disconnection_handler_;
    };
} // namespace tsp_client
