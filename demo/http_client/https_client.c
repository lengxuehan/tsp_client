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

using namespace tinyxml2;

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

std::string get_xml_string_value(const XMLElement* root, const std::string &key){
    const XMLElement* xml_element = root->FirstChildElement(key.c_str());
    std::string res{};
    if(xml_element != nullptr){
        const char* value = xml_element->GetText();
        res = value;
    }else{
        std::cout << "HttpsProxy::get_xml_string_value get '%s' element failed\n" << key << std::endl;
    }
    return res;
}

std::string get_now() {
    std::chrono::system_clock::time_point input = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(input);
    struct tm tm_now{};
    struct tm *buf;
    buf = localtime_r(&tt, &tm_now); //Locale time-zone
    if (buf == nullptr){ // error
        std::cout << "error which may be a runtime constraint violation or a failure to convert the specified time to local calendar time.";
        return {};
    }
    std::stringstream ss;
    //ss << std::put_time(&tm_now, "%Y%m%d%H%M%S");
    ss << tm_now.tm_year + 1900;
    ss << std::setw(2) << std::setfill('0') << tm_now.tm_mon + 1;
    ss << std::setw(2) << std::setfill('0') << tm_now.tm_mday;
    ss << std::setw(2) << std::setfill('0') << tm_now.tm_hour;
    ss << std::setw(2) << std::setfill('0') << tm_now.tm_min;
    ss << std::setw(2) << std::setfill('0') <<  tm_now.tm_sec;
    return ss.str();
}

uint32_t get_xml_uint_value(const XMLElement* root, const std::string &key){
    const XMLElement* xml_element = root->FirstChildElement(key.c_str());
    uint32_t u{0U};
    if(xml_element != nullptr){
        XMLError error = xml_element->QueryUnsignedText(&u);
        if(error != XMLError::XML_SUCCESS){
            std::cout << "HttpsProxy::get_xml_uint_value get '%s' element failed\n" << key << std::endl;
        }
    }else{
        std::cout << "HttpsProxy::get_xml_uint_value get '%s' element failed\n" << key << std::endl;
    }
    return u;
}

int main() {
    std::string host_dir{"/mnt/e/learn/cpps/CppServer/tools/certificates"}; // /mnt/d/work/tsp_client/demo/etc/server
    std::cout << "Hello, World!" << std::endl;

    std::string str_now = get_now();

    std::string str_xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
        <root>
        <respStatus>OK</respStatus>
        <serverDomain>apngw.changan.com.cn</serverDomain>
        <serverDomainPort>443</serverDomainPort>
        </root>)";

    XMLDocument doc;
    doc.Parse(str_xml.c_str());
    XMLElement* root = doc.FirstChildElement("root");
    std::string str_resp_status = get_xml_string_value(root, "respStatus");
    std::string str_server_domain = get_xml_string_value(root, "serverDomain");
    uint16_t u_server_domain_port = (uint16_t)get_xml_uint_value(root, "serverDomainPort");

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
