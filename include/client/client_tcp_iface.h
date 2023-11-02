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
        uint16_t port{8888};
        std::string str_ca_path{};
        std::string str_client_key_path{};
        std::string str_client_crt_path{};
        bool support_tls{false};
        uint8_t message_header_size{30};
        uint8_t body_length_index{29};
        uint8_t body_length_size{2};
    };
    class client_tcp_iface {
    public:
        //! ctor
        client_tcp_iface(const tls_tcp_config& ssl_cfg): tls_tcp_cfg_{ssl_cfg}{};

        //! dtor
        virtual ~client_tcp_iface(void) = default;

    public:
        //!
        //! start the tcp client
        //!
        //! \param addr host to be connected to
        //! \param port port to be connected to
        //! \return whether the client is connected to remote server or not
        //!
        virtual bool connect(const std::string &addr, std::uint32_t port) = 0;

        //!
        //! stop the tcp client
        //!
        virtual void disconnect() = 0;

        //!
        //! \return whether the client is currently connected or not
        //!
        virtual bool is_connected() const = 0;

        //! Send tcp request
        //! \return whether the client is currently sent or not
        //!
        virtual bool send(std::vector<uint8_t> &&request) = 0;
    public:
        //!
        //! disconnection handler
        //!
        typedef std::function<void()> disconnection_handler_t;

        //!
        //! set on disconnection handler
        //!
        //! \param disconnection_handler handler to be called in case of a disconnection
        //!
        virtual void set_on_disconnection_handler(const disconnection_handler_t &disconnection_handler) = 0;

        //!
        //! package header handler
        //!
        typedef std::function<bool(const std::vector<uint8_t> &)> package_handler_t;
        //!
        //! et on message parse handler to deal with package
        //!
        //! \param header_handler handler to be called in case of parsing package
        //!
        virtual void set_message_handler(const package_handler_t &package_handler) = 0;
    protected:
        const tls_tcp_config& tls_tcp_cfg_;
    };

} // namespace tsp_client