/**
* @file common.h
* @brief common defines.
* @author       wuting.xu
* @date         2023/10/30
* @par Copyright(c):    2023 megatronix. All rights reserved.
*/

#pragma once

#include <vector>
#include <string>
#include <array>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <memory>
#include <functional>
#include <sstream>
#include <iomanip>

enum class wakeup_type_t {
    fired,    // 点火
    calling,  // 来电或者短信唤醒
};

constexpr auto xml_resp_status = "respStatus";
constexpr auto xml_server_domain = "serverDomain";
constexpr auto xml_file_update_domain = "fileUploadDomain";
constexpr auto xml_server_domain_port = "serverDomainPort";
constexpr auto xml_server_host = "serverHost";
constexpr auto xml_server_port = "serverPort";
constexpr auto xml_error_code = "errorCode";
constexpr auto xml_vin = "vin";
constexpr auto xml_iccid = "iccid";
constexpr auto xml_tuid = "tuid";
constexpr auto xml_imei = "imei";
constexpr auto xml_park_mode = "park_mode";
constexpr auto xml_gbt_on = "gbt_on";
constexpr auto xml_is_match = "isMatch";
constexpr auto str_last_upload_gps_tm = "upload_gps_time";

struct server_info_t {
    std::string str_server_domain{};
    uint16_t u_server_domain_port{0U};
    std::string str_server_host{};
    uint16_t u_server_port{0U};
};

#undef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
    TypeName(const TypeName&);                           \
    void operator=(const TypeName&)

// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#undef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)    \
    TypeName();                                     \
    DISALLOW_EVIL_CONSTRUCTORS(TypeName)

class EVPCipherException : public std::runtime_error {
public:
    EVPCipherException()
            : std::runtime_error("EVP cipher error"), err_code{ERR_peek_last_error()}, err_msg_buf{0} {}

    virtual const char *what() {
        return ERR_error_string(err_code, err_msg_buf);
    }

    unsigned long err_code;
    char err_msg_buf[128];
};

using EVPKey = std::array<uint8_t, EVP_MAX_KEY_LENGTH>;
using EVPIv = std::array<uint8_t, EVP_MAX_IV_LENGTH>;

class EVPCipher {
private:
    std::function<void(EVP_CIPHER_CTX *)> cipherFreeFn = [](EVP_CIPHER_CTX *ptr) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        delete ptr;
#else
        EVP_CIPHER_CTX_free(ptr);
#endif
    };
public:
    EVPCipher(const EVP_CIPHER *type, const EVPKey &key, const EVPIv &iv, bool encrypt) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        ctx = std::unique_ptr<EVP_CIPHER_CTX, decltype(cipherFreeFn)>(new EVP_CIPHER_CTX,
                                                                    cipherFreeFn);
#else
        ctx = std::unique_ptr<EVP_CIPHER_CTX, decltype(cipherFreeFn)>(EVP_CIPHER_CTX_new(),
                                                                    cipherFreeFn);
#endif
        EVP_CIPHER_CTX_init(ctx.get());
        EVP_CipherInit_ex(ctx.get(), type, nullptr, key.data(), iv.data(), encrypt ? 1 : 0);
    }

    ~EVPCipher() {
        EVP_CIPHER_CTX_cleanup(ctx.get());
    }

    void expandAccumulator(size_t inputSize) {
        auto block_size = EVP_CIPHER_CTX_block_size(ctx.get());
        const auto maxIncrease = (((inputSize / block_size) + 1) * block_size);
        accumulator.resize(accumulator.size() + maxIncrease);
    }

    void update(const std::vector<uint8_t> &data) {
        const auto oldSize = accumulator.size();
        expandAccumulator(data.size());
        int encryptedSize = 0;
        if (EVP_CipherUpdate(ctx.get(),
                            accumulator.data() + oldSize,
                            &encryptedSize,
                            data.data(),
                            data.size()) != 1) {
            throw EVPCipherException();
        }

        accumulator.resize(oldSize + encryptedSize);
    }

    void update(const std::string &data) {
        const auto oldSize = accumulator.size();
        expandAccumulator(data.size());
        int encryptedSize = 0;

        auto data_ptr = reinterpret_cast<const uint8_t *>(data.data());
        if (EVP_CipherUpdate(
                ctx.get(), accumulator.data() + oldSize, &encryptedSize, data_ptr, data.size()) !=
            1) {
            throw EVPCipherException();
        }

        accumulator.resize(oldSize + encryptedSize);
    }

    void finalize() {
        if (finalized)
            return;
        const auto oldSize = accumulator.size();
        // Add one more block size to the accumulator;
        expandAccumulator(1);
        int encryptSize = oldSize;
        if (EVP_CipherFinal_ex(ctx.get(), accumulator.data() + oldSize, &encryptSize) != 1) {
            throw EVPCipherException();
        }
        accumulator.resize(oldSize + encryptSize);
        finalized = true;
    }

    std::vector<uint8_t>::iterator begin() {
        finalize();
        return accumulator.begin();
    }

    std::vector<uint8_t>::iterator end() {
        finalize();
        return accumulator.end();
    }

    std::vector<uint8_t>::const_iterator cbegin() {
        finalize();
        return accumulator.cbegin();
    }

    std::vector<uint8_t>::const_iterator cend() {
        finalize();
        return accumulator.cend();
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(cipherFreeFn)> ctx;
    std::vector<uint8_t> accumulator;
    bool finalized = false;
};

std::string convert_to_string(const uint8_t* data, uint16_t size);
std::string convert_to_hex_string(const std::vector<uint8_t>& vec_data);
std::string convert_to_hex_string(const uint8_t* data, uint16_t size);
void convert_hex_string_to_vector(const std::string& str_input, std::vector<uint8_t>& vec_data);
/**
 * @return 以秒为单位
 * */
uint64_t get_timestamp();
/**
 * 判断是否为同一天
 * @param old_tm 上一次时间, 以秒为单位
 * @param new_tm 现在时间, 以秒为单位
 * @return true 同一天; false 不是同一天
 */
bool is_same_day(const uint64_t old_tm, const uint64_t new_tm);

class EndianChecker
{
public:
    static bool isLittleEndianHost()
    {
        std::uint32_t test_number = 0x1;
        auto* test_bytes = reinterpret_cast<std::uint8_t*>(&test_number);
        return (test_bytes[0] == 1);
    }
};

