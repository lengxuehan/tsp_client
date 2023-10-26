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

void signal_init_handler(int) {
    cv.notify_all();
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    tsp_client::ssl_config ssl_cfg{};
    boost_support::socket::tcp::CreateTcpServerSocket server_socket("127.0.0.1", 8888, ssl_cfg);
    server_socket.get_tcp_server_connection(on_new_message);

    signal(SIGINT, &signal_init_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    return 0;
}
