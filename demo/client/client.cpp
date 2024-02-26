#include <iostream>
#include <cstring>
#include <csignal>
#include "client/client.h"
#include "client/client_tcp_iface.h"
#include "packages/messages.h"
#include "tb_log.h"
#include "tsp/tsp_proxy.h"

std::condition_variable cv;
std::atomic_bool exist{false};
void signal_init_handler(int) {
    cv.notify_all();
    exist.store(true);
}

int main() {
    using ssl_config = tsp_client::tls_tcp_config;
    std::cout << "Hello, World!" << std::endl;
    std::string host_dir{"/mnt/d/work/tsp_client/demo/etc/changan"}; //  /mnt/e/learn/cpps/CppServer/tools/certificates
    /*
    ssl_config ssl_cfg;
    ssl_cfg.support_tls = false;
    ssl_cfg.str_ca_path = host_dir + "/rootca_cert.pem";
    ssl_cfg.str_client_key_path = host_dir + "/28CAG2023112700000000.key";
    ssl_cfg.str_client_crt_path = host_dir + "/28CAG2023112700000000.cer";
    tsp_client::client client(ssl_cfg);
    std::atomic_bool connected{false};
    client.connect("127.0.0.1", 8888, [&connected](const std::string& host, std::size_t port, tsp_client::connect_state_t status) {
        if (status == tsp_client::connect_state_t::ok) {
            std::cout << "client connected from " << host << ":" << port << std::endl;
            connected.store(true);
        }
        if (status == tsp_client::connect_state_t::failed) {
            std::cout << "client failed to connect from " << host << ":" << port << std::endl;
        }
    },
    [](const std::vector<uint8_t>& message){
        tsp_client::MessageHeader header;
        if (header.parse(message)) {
            TB_LOG_INFO("client::handle_message parse message successful, status:%d, request_id:%s\n",
                        header.status_code, header.request_id);
        } else {
            TB_LOG_INFO("client::handle_message parse message failed. message:%s",
                        message.data());
        }
    },0, 1);

    signal(SIGINT, &signal_init_handler);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000U));
    if (connected.load()){
        uint16_t try_count{5U};
        while (--try_count > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
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
        }
    }*/
    std::shared_ptr<TspProxy> tsp_proxy = std::make_shared<TspProxy>();
    tsp_proxy->on_wake_up();
    signal(SIGINT, &signal_init_handler);
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    //client.disconnect();
    return 0;
}
