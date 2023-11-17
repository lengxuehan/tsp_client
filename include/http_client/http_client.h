#pragma once

#include <mutex>
#include <string>
#include <memory>
#include "http_request.h"
#include "http_response.h"
#include "http_cookie.h"

namespace http_client {

    class HttpClient {
        using LockGuard = std::lock_guard<std::mutex>;
        using RequestPtr = std::shared_ptr<HttpRequest>;
        using ResponsePtr = std::shared_ptr<HttpResponse>;

    public:
        static HttpClient *getInstance() {
            return instance_;
        }

        void enable_cookies(const std::string &cookieFile = std::string());

        const std::string &get_cookie_filename() const;

        void set_ssl_verification(const std::string &caFile);

        void set_ssl_verification(std::string &&caFile);

        const std::string &get_ssl_verification() const;

        void set_timeout_for_connect(int value);

        int get_timeout_for_connect() const;

        void set_timeout_for_read(int value);

        int get_timeout_for_read() const;

        void send(const RequestPtr &request);

        void send_async(const RequestPtr &request);

        const HttpCookie *getCookie() const { return cookie_; }

    private:
        HttpClient();

        ~HttpClient();

        HttpClient(const HttpClient &) = delete;

        HttpClient &operator=(const HttpClient &) = delete;

        void process_response(const RequestPtr &request, ResponsePtr &response);

        void doGet(const RequestPtr &request, ResponsePtr &response);

        void doPost(const RequestPtr &request, ResponsePtr &response);

        void doPut(const RequestPtr &request, ResponsePtr &response);

        void doDelete(const RequestPtr &request, ResponsePtr &response);

    private:
        static const int kNumThreads = 1;
        static const int kErrorBufSize = 256;

        static HttpClient *instance_;

        mutable std::mutex cookieFileMutex_;
        std::string cookieFilename_;

        mutable std::mutex sslCaFileMutex_;
        std::string sslCaFilename_;

        int timeoutForConnect_;
        int timeoutForRead_;

        HttpCookie *cookie_;
    };

}