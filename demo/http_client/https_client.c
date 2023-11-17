#include <iostream>
#include <csignal>
#include <condition_variable>
#include "nlohmann/json.hpp"
#include "boost_support/socket/tcp/tcp_types.h"
#include "http_client/http_client.h"
#include "tinyxml2/tinyxml2.h"

using namespace http_client;
using RequestPtr	= std::shared_ptr<http_client::HttpRequest>;
using ResponsePtr	= std::shared_ptr<http_client::HttpResponse>;

void onMessage(const RequestPtr &request, const ResponsePtr &response) {
    std::cout << "\n" << request->getTag() << std::endl;
    if (response->isSucceed()) {
        std::cout << "HTTP request succeed!" << std::endl;

        auto utf8Str = response->responseDataAsString();
        nlohmann::json json_value = nlohmann::json::parse(utf8Str, nullptr, false);

        std::cout << json_value.dump() << std::endl;
    } else {
        std::cout << "HTTP request failed!" << std::endl;
        std::cout << "status code: " << response->getResponseCode() << std::endl;
        std::cout << "reason: " << response->gerErrorBuffer() << std::endl;
    }
}

std::condition_variable cv;
bool running = true;

void signal_init_handler(int) {
    cv.notify_all();
}

int main() {
    std::string host_dir{"/mnt/e/learn/cpps/CppServer/tools/certificates"}; // /mnt/d/work/tsp_client/demo/etc/server
    std::cout << "Hello, World!" << std::endl;

    static const char* xml =
            "<?xml version=\"1.0\"?>"
            "<!DOCTYPE PLAY SYSTEM \"play.dtd\">"
            "<PLAY>"
            "<TITLE>A Midsummer Night's Dream</TITLE>"
            "</PLAY>";
    using namespace tinyxml2;
    XMLDocument doc;
    doc.Parse( xml );

    XMLElement* titleElement = doc.FirstChildElement( "PLAY" )->FirstChildElement( "TITLE" );
    const char* title = titleElement->GetText();
    printf( "Name of play (1): %s\n", title );

    XMLText* textNode = titleElement->FirstChild()->ToText();
    title = textNode->Value();
    printf( "Name of play (2): %s\n", title );

    boost_support::socket::tcp::tls_config ssl_cfg{};
    ssl_cfg.support_tls = true;
    ssl_cfg.str_ca_path = host_dir + "/ca.crt";
    ssl_cfg.str_client_key_path = host_dir + "/server.key";
    ssl_cfg.str_client_crt_path = host_dir + "/server.crt";
    std::string str_dh_file = host_dir + "/dh2048.pem";

    RequestPtr request = std::make_shared<HttpRequest>();
    request->setRequestType(HttpRequest::Type::GET);
    request->setUrl("https://httpbin.org/get");
    request->setCallback(onMessage);

    request->setTag("test case 3: GET");
    HttpClient::getInstance()->send(request);

    request->setTag("test case 3: GET immediate");
    HttpClient::getInstance()->send_async(request);

    signal(SIGINT, &signal_init_handler);

    std::mutex mtx;
    std::unique_lock <std::mutex> lock(mtx);
    cv.wait(lock);
    return 0;
}
