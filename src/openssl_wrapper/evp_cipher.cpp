//
// Created by wuting.xu on 2023/11/23.
//
#include <iostream>
#include <algorithm>
#include "openssl_wrapper/evp_cipher.h"

static bool do_crypt(const std::string &str_token, const std::string &str_iv, const std::vector<uint8_t> &message_body_in, std::vector<uint8_t> &message_body_out, int enc){
    int templ{0}, total{0};
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    // 初始化,设置算法类型和加解密类型以及加密密钥和偏移量
    EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), nullptr, (uint8_t*)str_token.data(), (uint8_t*)str_iv.data(), enc);
    // 设置是否启用填充,默认是启用的
    // 如果填充参数为零,则不执行填充,此时加密或解密的数据总量必须是块大小的倍数,否则将发生错误
    EVP_CIPHER_CTX_set_padding(ctx, 1);
    // key和iv长度断言检查,断言长度随着cipher的不同而不同
    OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);
    OPENSSL_assert(EVP_CIPHER_CTX_iv_length(ctx) == 16);
    templ= 0;
    total = 0;
    if (!EVP_CipherUpdate(ctx, (uint8_t*)message_body_out.data(), &templ, message_body_in.data(), message_body_in.size())){
        printf("EVP_CipherUpdate fail...\n");
        goto err;
    }
    total += templ;

    if (!EVP_CipherFinal_ex(ctx, (uint8_t*)message_body_out.data() + total, &templ)){
        std::cout << "EVP_CipherFinal_ex fail..." << std::endl;
        goto err;
    }
    total += templ;
    EVP_CIPHER_CTX_free(ctx);
    return true;
err:
    EVP_CIPHER_CTX_free(ctx);
    return false;
}

bool encrypt(const std::string &str_token, const std::string &str_iv, const std::vector<uint8_t> &plain_message_body, std::vector<uint8_t> &whisper_message_body) {
    int enc{1}; // enc
    //whisper_message_body.resize((plain_message_body.size()/16 + 1)*16);
    //return do_crypt(str_token, str_iv, plain_message_body, whisper_message_body, enc);
    try {
        EVPKey aes_key;
        EVPIv aes_iv;
        std::copy_n(str_token.begin(), 16, aes_key.begin());
        std::copy_n(str_iv.begin(), 2, aes_iv.begin());
        EVPCipher cipher(EVP_aes_128_cbc(), aes_key, aes_iv, true);
        cipher.update(plain_message_body);
        whisper_message_body = std::vector<uint8_t>(cipher.cbegin(), cipher.cend());
    } catch (EVPCipherException& e){
        std::cout << "encrypt failed:" << e.what() << std::endl;
        return false;
    }
    return true;
}

bool decrypt(const std::string &str_token, const std::string &str_iv, const std::vector<uint8_t> &whisper_message_body, std::vector<uint8_t> &plain_message_body){
    int enc{0}; // enc
    //plain_message_body.resize(whisper_message_body.size()/2 + 1);
    //return do_crypt(str_token, str_iv, whisper_message_body, plain_message_body, enc);

    try {
        EVPKey aes_key;
        EVPIv aes_iv;
        std::copy_n(str_token.begin(), 16, aes_key.begin());
        std::copy_n(str_iv.begin(), 2, aes_iv.begin());
        EVPCipher cipher(EVP_aes_128_cbc(), aes_key, aes_iv, false);
        cipher.update(whisper_message_body);
        plain_message_body = std::vector<uint8_t>(cipher.cbegin(), cipher.cend());
    } catch (EVPCipherException& e){
        std::cout << "decrypt failed:" << e.what() << std::endl;
        return false;
    }
    return true;
}