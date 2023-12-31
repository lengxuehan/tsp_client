#include <memory>
#include <curl/curl.h>
#include <thread>
#include <sstream>
#include "http_client/http_client.h"

using namespace http_client;

static size_t writeCallback(void *data, size_t size, size_t nMember, void *stream) {
    std::vector<char> *recvBuf = static_cast<std::vector<char> *>(stream);
    size_t writeSize = size * nMember;
    recvBuf->insert(recvBuf->end(), (char *) data, (char *) data + writeSize);

    return writeSize;
}

static size_t readCallback(void *data, size_t size, size_t nMember, void *stream) {
    size_t nRead = fread(data, size, nMember, (FILE *) stream);
    return nRead;
}

class Curl {
    using RequestPtr = std::shared_ptr<HttpRequest>;
    using ResponsePtr = std::shared_ptr<HttpResponse>;
public:
    Curl() :
            curl_(curl_easy_init()),
            headers_(nullptr) {
    }

    ~Curl() {
        if (curl_) curl_easy_cleanup(curl_);

        if (headers_) curl_slist_free_all(headers_);
    }

    bool init(const HttpClient &client,
              const RequestPtr &request,
              void *responseData,
              void *responseHeader,
              char *errorBuf) {
        if (curl_ == nullptr) return false;
        /*set no signal*/
        if (!setOption(CURLOPT_NOSIGNAL, 1L)) return false;

        /*set accept encoding*/
        if (!setOption(CURLOPT_ACCEPT_ENCODING, "")) return false;

        /*set cookie*/
        std::string cookieFilename = client.get_cookie_filename();
        if (!cookieFilename.empty()) {
            if (!setOption(CURLOPT_COOKIEFILE, cookieFilename.data())) {
                return false;
            }
            if (!setOption(CURLOPT_COOKIEJAR, cookieFilename.data())) {
                return false;
            }
        }

        /*set timeout*/
        if (!setOption(CURLOPT_TIMEOUT, client.get_timeout_for_read())) {
            return false;
        }
        /*set connect timeout*/
        if (!setOption(CURLOPT_CONNECTTIMEOUT, client.get_timeout_for_connect())) {
            return false;
        }
        /*set ssl*/
        std::string sslCaFilename = client.get_ssl_ca_verification();
        std::string sslCertFilename = client.get_ssl_cert_verification();
        std::string sslKeyFilename = client.get_ssl_key_verification();
        if (sslCaFilename.empty()) {
            if (!setOption(CURLOPT_SSL_VERIFYPEER, false))
                return false;
            if (!setOption(CURLOPT_SSL_VERIFYHOST, false))
                return false;
        } else if(!sslCertFilename.empty() && !sslKeyFilename.empty()) {
            if (!setOption(CURLOPT_SSL_VERIFYPEER, 1L))
                return false;
            //if (!setOption(CURLOPT_SSL_VERIFYHOST, 2L)) return false;
            if (!setOption(CURLOPT_CERTINFO, sslCaFilename.data())) {
                return false;
            }
            if (!setOption(CURLOPT_SSLCERT, sslCertFilename.data())) {
                return false;
            }
            if (!setOption(CURLOPT_SSLKEY, sslKeyFilename.data())) {
                return false;
            }
        }else {
            if (!setOption(CURLOPT_SSL_VERIFYPEER, 1L)) return false;
            if (!setOption(CURLOPT_SSL_VERIFYHOST, 2L)) return false;
            if (!setOption(CURLOPT_CAINFO, sslCaFilename.data())) {
                return false;
            }
        }
        /*set header*/
        std::vector<std::string> headers = request->getHeaders();
        if (!headers.empty()) {
            for (auto &header: headers) {
                headers_ = curl_slist_append(headers_, header.c_str());
            }

            if (!setOption(CURLOPT_HTTPHEADER, headers_)) {
                return false;
            }
        }
        /*set main option*/
        return setOption(CURLOPT_URL, request->getUrl()) &&
               setOption(CURLOPT_WRITEFUNCTION, writeCallback) &&
               setOption(CURLOPT_WRITEDATA, responseData) &&
               setOption(CURLOPT_HEADERFUNCTION, writeCallback) &&
               setOption(CURLOPT_HEADERDATA, responseHeader) &&
               setOption(CURLOPT_ERRORBUFFER, errorBuf);
    }

    template<typename T>
    bool setOption(CURLoption option, T data) {
        return CURLE_OK == curl_easy_setopt(curl_, option, data);
    }

    bool perform(long *responseCode) {
        if (CURLE_OK != curl_easy_perform(curl_)) return false;

        CURLcode code = curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, responseCode);
        if (code != CURLE_OK || !(*responseCode >= 200 && *responseCode < 300)) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
            return false;
        }

        return true;
    }

private:
    CURL *curl_;
    curl_slist *headers_;
};

HttpClient *HttpClient::instance_ = new HttpClient();

HttpClient::HttpClient() :
        timeoutForConnect_(30),
        timeoutForRead_(60){
    curl_global_init(CURL_GLOBAL_ALL);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

void HttpClient::enable_cookies(const std::string &cookieFile) {
    LockGuard lock(cookieFileMutex_);
    if (!cookieFile.empty()) {
        cookieFilename_ = cookieFile;
    } else {
        cookieFilename_ = "cookieFile.txt";
    }
}

const std::string &HttpClient::get_cookie_filename() const {
    LockGuard lock(cookieFileMutex_);
    return cookieFilename_;
}

void HttpClient::set_ssl_ca_verification(const std::string &caFile) {
    LockGuard lock(sslCaFileMutex_);
    sslCaFilename_ = caFile;
}

void HttpClient::set_ssl_ca_verification(std::string &&caFile) {
    LockGuard lock(sslCaFileMutex_);
    sslCaFilename_ = std::move(caFile);
}

const std::string &HttpClient::get_ssl_ca_verification() const {
    LockGuard lock(sslCaFileMutex_);
    return sslCaFilename_;
}

void HttpClient::set_ssl_cert_verification(const std::string &certFile) {
    LockGuard lock(sslCaFileMutex_);
    sslCertFilename_ = certFile;
}

void HttpClient::set_ssl_cert_verification(std::string &&certFile) {
    LockGuard lock(sslCaFileMutex_);
    sslCertFilename_ = std::move(certFile);
}

const std::string &HttpClient::get_ssl_cert_verification() const {
    return sslCertFilename_;
}

void HttpClient::set_ssl_key_verification(const std::string &keyFile) {
    LockGuard lock(sslCaFileMutex_);
    sslKeyFilename_ = std::move(keyFile);
}

void HttpClient::set_ssl_key_verification(std::string &&keyFile) {
    LockGuard lock(sslCaFileMutex_);
    sslKeyFilename_ = std::move(keyFile);
}

const std::string &HttpClient::get_ssl_key_verification() const {
    return sslKeyFilename_;
}

void HttpClient::set_timeout_for_connect(int value) {
    timeoutForConnect_ = value;
}

int HttpClient::get_timeout_for_connect() const {
    return timeoutForConnect_;
}

void HttpClient::set_timeout_for_read(int value) {
    timeoutForRead_ = value;
}

int HttpClient::get_timeout_for_read() const {
    return timeoutForRead_;
}

void HttpClient::send(const RequestPtr &request) {
    ResponsePtr response = std::make_shared<HttpResponse>();
    process_response(request,response);
    //threadPool_.run(std::bind(&HttpClient::process_response, this, request, response));
}

void HttpClient::send_async(const RequestPtr &request) {
    ResponsePtr response = std::make_shared<HttpResponse>();
    auto t = std::thread(std::bind(&HttpClient::process_response, this, request, response));
    t.detach();
}

void HttpClient::process_response(const RequestPtr &request, ResponsePtr &response) {
    switch (request->getRequestType()) {
        case HttpRequest::Type::GET:
            doGet(request, response);
            break;
        case HttpRequest::Type::POST:
            doPost(request, response);
            break;
        case HttpRequest::Type::PUT:
            doPut(request, response);
            break;
        case HttpRequest::Type::DOWNLOAD:
            doDownload(request, response);
            break;
        case HttpRequest::Type::DELETE:
            doPut(request, response);
            break;
        default:
            break;
    }

    auto callback = request->getResponseCallback();
    callback(request, response);
}

void HttpClient::doGet(const RequestPtr &request, ResponsePtr &response) {
    char errorBuf[kErrorBufSize] = {0};
    auto responseData = response->getResponseData();
    auto responseHeader = response->getResponseHeader();
    long responseCode = -1;

    Curl curl;
    bool ok = curl.init(*this, request, responseData, responseHeader, errorBuf) &&
              curl.setOption(CURLOPT_FOLLOWLOCATION, true) &&
              curl.perform(&responseCode);

    response->setResponseCode(responseCode);
    if (ok) {
        response->setSucceed(true);
    } else {
        response->setSucceed(false);
        response->setErrorBuffer(errorBuf);
    }
}

void HttpClient::doPost(const RequestPtr &request, ResponsePtr &response) {
    char errorBuf[kErrorBufSize] = {0};
    auto responseData = response->getResponseData();
    auto responseHeader = response->getResponseHeader();
    auto postData = request->getRequestData();
    auto postDataSize = request->getRequestDataSize();
    long responseCode = -1;

    Curl curl;
    bool ok = curl.init(*this, request, responseData, responseHeader, errorBuf) &&
              curl.setOption(CURLOPT_POST, 1) &&
              curl.setOption(CURLOPT_POSTFIELDS, postData) &&
              curl.setOption(CURLOPT_POSTFIELDSIZE, postDataSize) &&
              curl.perform(&responseCode);

    response->setResponseCode(responseCode);
    if (ok) {
        response->setSucceed(true);
    } else {
        response->setSucceed(false);
        response->setErrorBuffer(errorBuf);
    }
}

void HttpClient::doPut(const RequestPtr &request, ResponsePtr &response) {
    auto responseData = response->getResponseData();
    auto responseHeader = response->getResponseHeader();
    char errorBuf[kErrorBufSize] = {0};
    auto requestData = request->getRequestData();
    auto requestDataSize = request->getRequestDataSize();
    long responseCode = -1;

    FILE *fp = nullptr;
    auto path = request->getUploadFilePath();
    size_t size = 0;
    if (!path.empty()) {
        fp = fopen(path.data(), "rb");
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
    }

    Curl curl;
    curl.init(*this, request, responseData, responseHeader, errorBuf);
    //curl.setOption(CURLOPT_PUT, 1L);
    curl.setOption(CURLOPT_CUSTOMREQUEST, "PUT");
    curl.setOption(CURLOPT_VERBOSE, true);
    curl.setOption(CURLOPT_POSTFIELDS, requestData);
    curl.setOption(CURLOPT_POSTFIELDSIZE, requestDataSize);

    if (fp) {
        curl.setOption(CURLOPT_UPLOAD, 1L);
        curl.setOption(CURLOPT_READFUNCTION, readCallback);
        curl.setOption(CURLOPT_READDATA, fp);
        curl.setOption(CURLOPT_INFILESIZE_LARGE, (curl_off_t) size);
    }

    bool ok = curl.perform(&responseCode);

    response->setResponseCode(responseCode);
    if (ok) {
        response->setSucceed(true);
    } else {
        response->setSucceed(false);
        response->setErrorBuffer(errorBuf);
    }

    if (fp) fclose(fp);
}

void HttpClient::doDownload(const RequestPtr &request, ResponsePtr &response) {
    auto responseData = response->getResponseData();
    auto responseHeader = response->getResponseHeader();
    char errorBuf[kErrorBufSize] = {0};
    auto requestData = request->getRequestData();
    auto requestDataSize = request->getRequestDataSize();
    long responseCode = -1;

    auto path = request->getDownloadFilePath();
    FILE * file_to_be_written = fopen(path.c_str(), "w");
    if (file_to_be_written == nullptr) {
        return;
    }
    std::vector<char> vec_stream;
    Curl curl;
    curl.init(*this, request, responseData, responseHeader, errorBuf);
    curl.setOption(CURLOPT_WRITEFUNCTION, writeCallback);
    curl.setOption(CURLOPT_WRITEDATA, &vec_stream);

    bool ok = curl.perform(&responseCode);

    response->setResponseCode(responseCode);
    if (ok) {
        response->setSucceed(true);
        size_t nlen = fwrite(vec_stream.data(), vec_stream.size(), vec_stream.size(), file_to_be_written);
        if (vec_stream.size() != nlen){
            std::stringstream ss;
            ss << "download_to_file fwrite size: " << vec_stream.size() << " written len: " << nlen << " error:" << errno;
            snprintf(errorBuf, kErrorBufSize, "%s\n", ss.str().c_str());
            response->setSucceed(false);
        }
    } else {
        response->setSucceed(false);
        response->setErrorBuffer(errorBuf);
    }

    if (file_to_be_written){
        fclose(file_to_be_written);
    }
}

void HttpClient::doDelete(const RequestPtr &request, ResponsePtr &response) {
    char errorBuf[kErrorBufSize] = {0};
    auto responseData = response->getResponseData();
    auto responseHeader = response->getResponseHeader();
    auto postData = request->getRequestData();
    auto postDataSize = request->getRequestDataSize();
    long responseCode = -1;

    Curl curl;
    bool ok = curl.init(*this, request, responseData, responseHeader, errorBuf) &&
              curl.setOption(CURLOPT_CUSTOMREQUEST, "DELETE") &&
              curl.setOption(CURLOPT_FOLLOWLOCATION, true) &&
              curl.perform(&responseCode);

    response->setResponseCode(responseCode);
    if (ok) {
        response->setSucceed(true);
    } else {
        response->setSucceed(false);
        response->setErrorBuffer(errorBuf);
    }
}