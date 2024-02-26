/**
* @file client.h
* @brief interface defining how tcp client should be implemented to be used inside tsp_client.
* @author		wuting.xu
* @date		    2023/10/23
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace tsp_client {
    struct tls_tcp_config{
        std::string server_ip;
        uint16_t port{0U};
        std::string str_ca_path{};
        std::string str_client_key_path{};
        std::string str_client_crt_path{};
        bool support_tls{false};
        uint8_t message_header_size{30};
        uint8_t body_length_index{28};
        uint8_t body_length_size{2};
        uint8_t msg_tail_size{1};
        uint8_t terminal_mark{0};
        std::string ifc{};
    };
    /**
     * @brief high availability (re)connection states
     *
     * dropped: connection has dropped
     * ok: connected
     * start: attempt of connection has started
     * sleeping: sleep between two attempts
     * failed: failed to connect
     * stopped: stop to try to reconnect
     */
    enum class connect_state_t {
        dropped,
        start,
        failed,
        ok,
        sleeping,
        login,
        login_failed,
        logout,
        stopped
    };
    /**
     * @brief connect handler, called whenever a new connection even occurred
     *
     */
    typedef std::function<void(const std::string&, std::size_t, connect_state_t)> connect_callback_t;
    /**
     * @brief reply callback called whenever a reply is received
     * takes as parameter the received reply
     */
    typedef std::function<void(const std::vector<uint8_t> &)> reply_callback_t;

    typedef std::function<void(const std::vector<uint8_t> &, bool)> published_callback_t;

    class client_iface {
    public:
        //! ctor
        client_iface(){};
        //client_iface(const tls_tcp_config& tls_tcp_cfg):tls_tcp_cfg_(tls_tcp_cfg){};

        //! dtor
        virtual ~client_iface(void) = default;

        //! copy ctor
        client_iface(const client_iface &) = delete;
        //! assignment operator
        client_iface &operator=(const client_iface &) = delete;

    public:
        //!
        //! start the tcp client
        //!
        //! \param addr host to be connected to
        //! \param port port to be connected to
        //! \return whether the client is connected to remote server or not
        //!
        virtual void connect(
                const std::string &host = "127.0.0.1",
                std::size_t port = 8888,
                const connect_callback_t &connect_callback = nullptr,
                const reply_callback_t &reply_callback = nullptr,
                std::uint32_t timeout_msecs = 0,
                std::int32_t max_reconnects = 0,
                std::uint32_t reconnect_interval_msecs = 0) = 0;

        //!
        //! stop the tcp client
        //!
        virtual void disconnect() = 0;

        //!
        //! \return whether the client is currently connected or not
        //!
        virtual bool is_connected() const = 0;

        //! Send tcp request
        //!
        virtual void send(const std::vector<uint8_t> &request) = 0;

        //! set message callback handle when published
        //!
        virtual void set_message_published_handle(const published_callback_t &) {};
    protected:
    };

} // namespace tsp_client