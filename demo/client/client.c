#include <iostream>
#include <cstring>
#include <csignal>
#include "client/client.h"
#include "client/client_tcp_iface.h"
#include "packages/messages.h"

std::condition_variable cv;

void signal_init_handler(int) {
    cv.notify_all();
}

int main() {
    using ssl_config = tsp_client::tls_tcp_config;
    std::cout << "Hello, World!" << std::endl;

    ssl_config ssl_cfg;
    ssl_cfg.support_tls = true;
    ssl_cfg.str_ca_path = "/mnt/d/work/tsp_client/demo/etc/client/ca.crt";
    ssl_cfg.str_client_key_path = "/mnt/d/work/tsp_client/demo/etc/client/client.key";
    ssl_cfg.str_client_crt_path = "/mnt/d/work/tsp_client/demo/etc/client/client.crt";
    tsp_client::client client(ssl_cfg);
    client.connect("127.0.0.1", 8888, [](const std::string& host, std::size_t port, tsp_client::client::connect_state status) {
        if (status == tsp_client::client::connect_state::ok) {
            std::cout << "client connected from " << host << ":" << port << std::endl;
        }
        if (status == tsp_client::client::connect_state::failed) {
            std::cout << "client failed to connect from " << host << ":" << port << std::endl;
        }
    });

    tsp_client::MessageHeader header;
    std::cout << "head size:" << sizeof(header) << std::endl;
    header.body_length = 0;
    header.request_id[0] = 97;
    header.request_id[1] = 97;
    header.request_id[2] = 97;
    header.request_id[3] = 97;
    header.request_id[4] = 97;
    header.request_id[5] = 97;
    header.status_code = static_cast<uint8_t>(tsp_client::StatusCode::kNormal);
    std::vector<uint8_t> data;
    header.serialize(data);
    std::cout << "data size:" << data.size() << std::endl;
    client.send(data);

    signal(SIGINT, &signal_init_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    client.disconnect();
    return 0;
}
