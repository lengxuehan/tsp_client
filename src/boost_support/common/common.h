
#pragma once

namespace boost_support {
namespace common {
    struct ssl_config{
        std::string str_ca_path{};
        std::string str_client_key_path{};
        std::string str_client_csr_path{};
        bool support_tls{false};
    };

}  // namespace logger
}  // namespace boost_support