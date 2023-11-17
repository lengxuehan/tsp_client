#pragma once

#include <string>
#include <vector>

namespace http_client {

    struct CookieInfo {
        std::string domain;
        bool tailMatch;
        std::string path;
        bool secure;
        std::string expires;
        std::string name;
        std::string value;
    };

    class HttpCookie {
    public:
        HttpCookie(const std::string &fullPathFilename);

        ~HttpCookie() = default;

        void read_file();

        void write_file();

        const std::vector<CookieInfo> *get_cookies() const;

        const CookieInfo *get_match_cookie(const std::string &url) const;

        void update_or_add_cookie(const CookieInfo &cookie);

    private:
        std::string get_string_from_file(const std::string &fullPathFilename);

    private:
        std::string cookieFilename_;
        std::vector<CookieInfo> cookies_;
    };

}