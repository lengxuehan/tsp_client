#include <iostream>
#include <cstring>
#include <csignal>
#include <stdlib.h>
#include "client/client.h"
#include "client/client_tcp_iface.h"
#include "packages/messages.h"
#include "tb_log.h"
#include "tsp/tsp_proxy.h"

#define ARRAY_SIZE(arr)    (sizeof(arr)/sizeof(arr[0]))

std::condition_variable cv;
std::atomic_bool exist{false};

static void destroy_resource() {
    // code to destroy resource
}

static void signal_handler(int32_t signum) {
    destroy_resource();
    //if ((signum >= _SIGMIN) && (signum <= _SIGMAX)) qnx
    if ((signum >= __SIGRTMIN) && (signum <= __SIGRTMAX)){
        signal(signum, SIG_DFL);
        raise(signum);
    }

    cv.notify_all();
    exist.store(true);
}

static void register_signals() {
    int32_t action_signals[] = {
        SIGABRT, SIGALRM, SIGBUS, /*SIGEMT,*/
        SIGFPE, SIGHUP, SIGINT, SIGIO, SIGIOT,
        SIGKILL, SIGPIPE, SIGPOLL, SIGQUIT,
        SIGSEGV, SIGSYS, SIGTERM, SIGTRAP,
        SIGTSTP, SIGTTIN, SIGTTOU, SIGXCPU
    };

    for (int32_t i = 0; i < ARRAY_SIZE(action_signals); ++i) {
        signal(action_signals[i], signal_handler);
    }

    atexit(destroy_resource); // 正常退出
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
    // signal(SIGINT, &signal_init_handler);
    register_signals();
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
    //client.disconnect();
    return 0;
}
