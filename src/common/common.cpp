#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include "tb_log.h"

std::string convert_to_string(const uint8_t* data, uint16_t size) {
    std::ostringstream stream;
    //stream << std::hex;
    for(uint16_t i = 0; i < size; ++i) {
        stream << (*(data + i));
    }
    return stream.str();
}

 std::string convert_to_hex_string(const std::vector<uint8_t>& vec_data) {
    std::ostringstream stream;
    //stream << "0x" << std::hex;
    stream << std::hex;
    for(auto u : vec_data) {
        stream << std::setw(2) << std::setfill ('0') << (uint16_t)u;
    }
    return stream.str();
}

std::string convert_to_hex_string(const uint8_t* data, uint16_t size) {
    std::ostringstream stream;
    //stream << "0x" << std::hex;
    stream << std::hex;
    for(uint16_t i = 0; i < size; ++i) {
        stream << std::setw(2) << std::setfill ('0') << (uint16_t)(*(data + i));
    }
    return stream.str();
 }

void convert_hex_string_to_vector(const std::string& str_input, std::vector<uint8_t>& vec_data) {
    size_t i = 0;
    if (str_input.find("0x") != std::string::npos) {
        i = 2;
    }
     size_t n_length = str_input.length();
    for (; i < n_length; i += 2) {
        size_t n{1U};
        if ((i + 1) < n_length) {
            n = 2;
        }
        vec_data.emplace_back(static_cast<uint8_t>(std::stoul(str_input.substr(i, n), nullptr, 16)));
    }
 }

bool is_same_day(const uint64_t old_tm, const uint64_t new_tm){
    auto t_old = static_cast<std::time_t>(old_tm);
    auto t_new = static_cast<std::time_t>(new_tm);
    struct tm tm_tmp;
    struct tm *tm_old = localtime_r(&t_old, &tm_tmp);
    if (tm_old  == nullptr){
        TB_LOG_INFO("is_same_day tm_old localtime_r return nullptr.");
        return false;
    }
    int32_t oy = tm_tmp.tm_year;
    int32_t od = tm_tmp.tm_yday;
    struct tm *tm_new = localtime_r(&t_new, &tm_tmp);
    if(tm_new == nullptr){
        TB_LOG_INFO("is_same_day tm_new localtime_r return nullptr.");
        return false;
    }
    int32_t ny = tm_tmp.tm_year;
    int32_t nd = tm_tmp.tm_yday;
    return (oy == ny) && (od == nd);
}

uint64_t get_timestamp() { //如果函数返回值修改为int64_t，程序中需要修改的地方太多了
    const auto start = std::chrono::system_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            start.time_since_epoch()).count());
}