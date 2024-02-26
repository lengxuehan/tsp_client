//
// Created by wuting.xu on 2023/1/11.
//

#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <vector>
#include <sys/wait.h>
#include <sstream>
#include <map>
#include "shm_map/shm_map.h"
#include "shm_map/base32.h"

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

std::size_t replace_all(std::string &inout, const std::string &what, const std::string &with) {
    std::size_t count{};
    std::string::size_type pos{};
    pos = inout.find(what.data(), pos, what.length());
    while (true) {
        if(pos == std::string::npos){
            break;
        }
        count++;
        (void)inout.replace(pos, what.length(), with.data(), with.length());
        pos += with.length();
        pos = inout.find(what.data(), pos, what.length());
    }
    return count;
}

void print_iter(const char *k, const char *v){
    printf("(%s)=>(%s)\n", k, v);
}

void print_vector(const std::vector<uint8_t> &data){
    printf("vector size=%lu : ", data.size());
    for(auto u : data){
        printf("%d, ", u);
    }
    printf("\n");
}

void base32_encode_data_to_string(std::vector<uint8_t> &data, std::string &str_encode){
    str_encode.resize(BASE32_LEN(data.size()));
    base32_encode(data.data(), data.size(), (uint8_t*)&str_encode[0]);
}

void base32_decode_string_array(const std::string &str_encode, std::vector<uint8_t>& plain_data){
    std::string decode;
    decode.resize(UNBASE32_LEN(strlen(str_encode.c_str())));
    base32_decode((uint8_t*)str_encode.c_str(), (uint8_t*)&decode[0]);
    for(auto c : decode) {
        plain_data.push_back(c);
    }
    while(plain_data.back() != 255 && plain_data.size() > 0){
        plain_data.pop_back();
    }
    if(plain_data.size() > 0) {
        plain_data.pop_back();
    }
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    std::vector<uint8_t> data{0, 255};

    print_vector(data);
    std::string encode_str;
    base32_encode_data_to_string(data, encode_str);
    printf("encode str: %s\n", encode_str.c_str());

    std::vector<uint8_t> vec_plain_data;
    base32_decode_string_array(encode_str, vec_plain_data);

    print_vector(vec_plain_data);
    exit(0);

    map_init(100, 500*1024, "shmmap.dat", NULL);

    /* bash 64不能解决二进制中间有0的问题
    map_put("1", acl_base64_encode(str_test.c_str(), str_test.length()));
    char* value = map_get("12345678");
    std::string decode = base.decode(encode);
    printf("decode: %s\n",decode.c_str());
    strings_to_bytes(acl_base64_decode(encode, strlen(encode)), data);
    */
    //std::string str_data;
    //bytes_to_string(data, str_data);


    //std::string encode_str;
    //base32_encode_data_to_string(data, encode_str);
    //map_put("1", encode_str.c_str());
    bool contains = map_contains("/get/modem/tbox_id");
    if(contains){
        char* encode_v = map_get("/get/modem/tbox_id");
        std::vector<uint8_t> plain_data;
        base32_decode_string_array(encode_v, plain_data);
        print_vector(plain_data);
    }
    exit(0);
    int cmd;
    bool command{true};
    while(command){
        char k[100], v[100];
        printf("map size %d, operation: 1:put, 2:get, 3:iter, 4:free list, 5:free size, 6:exit\n", map_size());
        scanf("%d", &cmd);
        if(cmd == 6) break;
        switch(cmd){
            case 1:
                printf("key:\n");
                scanf("%s", k);
                printf("value:\n");
                scanf("%s", v);
                map_put(k, v);
                break;
            case 2:
                printf("key:\n");
                scanf("%s", k);
                printf("%s=>%s\n", k, map_get(k));
                break;
            case 3:
                map_iter(print_iter);
                break;
            case 4:
                m_free_list_info();
                break;
            case 5:
                printf("free size: %d\n", m_free_size());
                break;

        }
        return 0;
    }


    map_iter(print_iter);
    pid_t pid;
    pid = 1; //fork();
    if(pid > 0) { // parent process
        for(int i = 0; i < 20; ++i) {
            if(i % 2 == 0){
                continue;
            }
            sleep(1);
            std::string k = std::to_string(i);
            map_put(k.c_str(), k.c_str());
        }
        sleep(10);
        //wait(nullptr);
        for(int i = 0; i < 20; ++i){
            std::string k = std::to_string(i);
            char* v = map_get(k.c_str());
            //assert(k == v);
        }
    }else{ // child process
        for(int i = 0; i < 20; ++i) {
            if(i % 2 == 1){
                continue;
            }
            std::string k = std::to_string(i);
            map_put(k.c_str(), k.c_str());
        }
        exit(0);
    }
}
