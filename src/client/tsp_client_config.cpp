#include <stdint.h>
#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
#include "tb_log.h"
#include "client/tsp_client_config.h"

bool TspClientConfig::load_config(const std::string &config_path)
{
    bool result = false;
    std::string errmsg;

    std::ifstream ifs;
    do
    {
        TB_LOG_INFO("open config:%s\n", config_path.c_str());
        ifs.open(config_path.c_str(), std::ios_base::in);
        if (!ifs.is_open()) {
            errmsg = "Failed to open " + config_path;
            break;
        }

        nlohmann::json config = nlohmann::json::parse(ifs, nullptr, false);
        if (config.is_discarded()) {
            errmsg = "Failed to parse " + config_path;
            break;
        }

        environment = config.at("environment");
        if (!config.contains(environment)) {
            errmsg = "Failed to get environment config: " + environment;
            break;
        }

        server_ip    = config[environment].at("server_ip");
        port         = config[environment]["port"];
        support_tls  = config[environment]["support_tls"];
        /*
        ca_path      = config[environment].at("ca_path");
        key_path     = config[environment].at("key_path");
        cert_path    = config[environment].at("cert_path");
        message_header_size  = config[environment]["message_header_size"];
        body_length_index    = config[environment]["body_length_index"];
        body_length_size    = config[environment]["body_length_size"];
        ifc         = config[environment].at("ifc");
        */
        result = true;
    } while (false);

    if (ifs.is_open())
        ifs.close();

    if (errmsg != "")
        TB_LOG_ERROR("%s\n", errmsg.c_str());

    TB_LOG_INFO("finish parse config\n");
    return result;
}

void TspClientConfig::dump_config() const {
    std::string str_protocol{"tcp"};
    if(support_tls){
        str_protocol = "tls";
    }
    TB_LOG_INFO("environment:%s support tls: %d\n", environment.c_str(), support_tls);
    TB_LOG_INFO("tsp-client ca_path:%s key_paht:%s cert_path:%s\n", 
            ca_path.c_str(), key_path.c_str(), cert_path.c_str());
    TB_LOG_INFO("tsp-client server:%s://%s:%d\n", str_protocol.c_str(), server_ip.c_str(), port);
}
