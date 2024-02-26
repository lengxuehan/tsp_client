#ifndef TLS_TCP_CONFIG_H
#define TLS_TCP_CONFIG_H
#include <string>
class TspClientConfig
{
public:
    std::string environment;

    std::string server_ip;      /**< TSP服务端地址 */
    uint16_t port;              /**< TSP client连接端口：1883、8883、或其他值 */
    bool support_tls;           /**< TSP client连接连接协议：tcp、ssl */
    std::string ca_path;        /**< CA证书路径 */
    std::string key_path;       /**< SSL证书路径 */
    std::string cert_path;      /**< SSL证书路径 */
    uint8_t message_header_size{30};  /**< 消息头大小 */
    uint8_t body_length_index{29};    /**< 消息头中body length起始字节位 */
    uint8_t body_length_size{2};      /**< body length的字节个数 */
    std::string ifc{};

public:
    bool load_config(const std::string &config_path);

    void dump_config() const;
};

#endif // TLS_TCP_CONFIG_H