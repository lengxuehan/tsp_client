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
    struct ssl_config{
        std::string str_ca_path{};
        std::string str_client_key_path{};
        std::string str_client_csr_path{};
        bool support_tls{false};
    };
    class client_tcp_iface {
    public:
        //! ctor
        client_tcp_iface(const ssl_config& ssl_cfg): ssl_cfg_{ssl_cfg}{};

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

    protected:
        const ssl_config& ssl_cfg_;
    };

} // namespace tsp_client