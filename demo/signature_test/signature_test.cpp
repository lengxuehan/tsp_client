//
// Created by wuting.xu on 2023/1/11.
//
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include "openssl_wrapper/signature.h"
#include "openssl_wrapper/evp_cipher.h"

void strings_to_bytes(const std::string &str, std::vector<uint8_t> &data){
    for(auto c : str) {
        data.push_back(c);
    }
}

void strings_to_bytes(const char* decode, std::vector<uint8_t> &data){
    while (*decode != '\0'){
        data.push_back(*decode);
        ++decode;
    }
}

void bytes_to_string(const std::vector<uint8_t> &data, std::string &str){
    std::stringstream ss;
    for(auto u : data){
        ss << u;
    }
    str = ss.str();
}

void test_aes_128(){
    std::string str_token{"0123456789abcdeF"};
    std::string str_iv{"1234567887654321"};
    std::vector<uint8_t> plain_message_body{1,2,3,4,5,6,7,8,1,2,3,4,5,6,7, 8};
    std::vector<uint8_t> whisper_message_body;
    std::vector<uint8_t> decrypto_whisper_msg_body;
    encrypt(str_token, str_iv, plain_message_body, whisper_message_body);

    decrypt(str_token, str_iv, whisper_message_body, decrypto_whisper_msg_body);

    std::cout << str_token << std::endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    test_aes_128();
    std::vector<uint8_t> tm{1,2,3};
    std::string str_tm(tm.data(), tm.data() + tm.size());
    std::cout << str_tm << std::endl;
    std::vector<uint8_t> new_tm(tm.data(), tm.data() + tm.size());
    tm.clear();
    strings_to_bytes(str_tm, tm);
    std::string file_name{"/mnt/d/work/tsp_client/demo/etc/ca.crt"};
    std::ifstream file(file_name.c_str(), std::ifstream::binary);
    if (!file){
        return -1;
    }
    unsigned char buf[1024];
    std::vector<unsigned char> vec_file;
    size_t size = 0;
    FILE *fp = fopen(file_name.data(), "r");
    if(fp != nullptr){
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        vec_file.resize(size);
        fseek(fp, 0, SEEK_SET);
    }
    size_t nRead = fread(&vec_file[0], sizeof(unsigned char), size, fp);
    std::cout << "fread read:" << nRead << " size:" << size << std::endl;
    if(nRead != size){
        std::cout << "fread file error" << std::endl;
        return 0;
    }
    uint32_t buf_size{0};
    int res = openssl_sha256(vec_file.data(),vec_file.size(), buf, &buf_size);
    if(res != 1){
        std::cout << "md5 file error" << std::endl;
    }
    std::string str;//(buf, buf + buf_size);
    std::stringstream ss;
    for(uint32_t i = 0; i < buf_size; ++i){
        uint8_t left = buf[i] >> 4;
        uint8_t right = buf[i] & 0x0F;
        std::cout << "left:" << (uint16_t)left << " right:" << (uint16_t)right  << std::endl;
        ss << std::hex << (uint16_t)left << std::hex << (uint16_t)right;
    }
    str = ss.str();
    std::cout << str << " md5 res len:" << buf_size << std::endl;
    return 0;
}
