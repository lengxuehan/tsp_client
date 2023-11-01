#include <iostream>
#include <thread>
#include <vector>
#include "boost_support/socket/tcp/tcp_server.h"
#include "packages/messages.h"
#include "packages/packet_helper.h"

using TcpMessagePtr = boost_support::socket::tcp::TcpMessagePtr;

std::string bytes_to_string(uint8_t* bytes, uint32_t length){
    std::stringstream  str;
    for(int i = 0; i < length; ++i){
        str << *(bytes + i);
    }
    return str.str();
}
using TcpServerConnection = boost_support::socket::tcp::CreateTcpServerSocket::TcpServerConnection;

void on_new_message(std::vector<TcpServerConnection>& connections,
                    const TcpMessagePtr &res) {
    std::cout << "Server recv data size:" << res->rxBuffer_.size() << std::endl;
    tsp_client::MessageHeader header;
    header.parse(res->rxBuffer_);
    std::cout << "status:" << (uint32_t)header.status_code << " request_id:"
        << bytes_to_string(header.request_id, 6)
        << " body length:" << header.body_length << std::endl;
    using TcpMessage = boost_support::socket::tcp::TcpMessageType;
    TcpMessagePtr tcp_message = std::make_unique<TcpMessage>();
    tcp_message->txBuffer_ = res->rxBuffer_;
    connections[0].transmit(std::move(tcp_message));
}

std::condition_variable cv;
bool running = true;

void signal_init_handler(int) {
    cv.notify_all();
}
int main() {
    std::cout << "Hello, World!" << std::endl;
    tsp_client::ssl_config ssl_cfg{};
    ssl_cfg.support_tls = true;
    ssl_cfg.str_ca_path = "/mnt/d/work/tsp_client/demo/etc/server/ca.crt";
    ssl_cfg.str_client_key_path = "/mnt/d/work/tsp_client/demo/etc/server/server.key";
    ssl_cfg.str_client_crt_path = "/mnt/d/work/tsp_client/demo/etc/server/server.crt";
    boost_support::socket::tcp::CreateTcpServerSocket server_socket("127.0.0.1", 8888, ssl_cfg);
    std::vector<TcpServerConnection> connections;
    while (running) {
        boost_support::socket::tcp::CreateTcpServerSocket::TcpServerConnection&& connection
        = server_socket.get_tcp_server_connection([&](const TcpMessagePtr &res) {
                    on_new_message(connections, res);
                });
        connection.set_message_header_size(tsp_client::PackHelper::get_message_header_size());
        connections.emplace_back(std::move(connection));
        connections[0].received_message();
    }
    signal(SIGINT, &signal_init_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    return 0;
}
