#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include "boost_support/socket/tcp/tcp_server.h"

using TcpMessagePtr = boost_support::socket::tcp::TcpMessagePtr;

void on_new_message(const TcpMessagePtr& res) {
    std::cout << "Server recv data" << std::endl;
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
    std::vector<boost_support::socket::tcp::CreateTcpServerSocket::TcpServerConnection> connections;
    while (running){
        auto&& connection = server_socket.get_tcp_server_connection(on_new_message);
        connections.emplace_back(std::move(connection));
    }

    signal(SIGINT, &signal_init_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    return 0;
}
