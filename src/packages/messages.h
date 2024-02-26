/**
* @file cmd_def.h
* @brief cmd defines.
* @author		wuting.xu
* @date		    2023/10/30
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once
#include <vector>
#include <string>
namespace tsp_client {

    typedef uint8_t  BYTE;
    typedef uint16_t WORD;
    typedef uint32_t DWORD;
    using TIME = uint8_t[5];

    enum class StatusCode : std::uint8_t {
        // status code
        kNormal            = 0,   // 正常，请求已正确完成
        kForbidVisitTcp    = 101, // 禁止TCP访问。T服务未开通或停止时，终端收到此错误码
        kVersionNotSupport = 102, // 版本不支持-服务器不支持终端协议版本
        kInvalidTuid       = 103, // 非法的终端(tuid未备案或已废弃)
        kCmdNotSupport     = 104, // 指令不支持
        kTokenTimeout      = 106, // 令牌密钥(实际为会话)已过期。终端收到该错误码，应立即重新登录
        kServerInnerError  = 200 // 内部错误 — 因为意外情况，服务器不能完成请求
    };

#pragma pack(1)
    struct MessageHeader {
        BYTE link_header;   // 消息头
        WORD port_version;  // 协议版本号
        BYTE status_code;   // 状态(错误)码
        BYTE ack_flag;      // 应答标识
        BYTE request_id[6]; // 请求ID号
        BYTE tuid[16];      // 终端ID号
        BYTE encrypt_flag;  // 加密标识 0:表示采用无加密; 1:表示使用128bitAES对MessageBody明文内容进行加密
        WORD body_length;   // MessageBody消息体的长度

        void serialize(std::vector<uint8_t> &data) const;
        bool parse(const std::vector<uint8_t> &data);

        void serialize_to_ipc_msg(std::vector<uint8_t> &data);
        bool parse_from_ips_msg(const std::vector<uint8_t> &data);
        void dump();
        uint8_t get_header_size();
        uint8_t get_ipc_header_size();
    };
#pragma pack()

    struct TLV {
        TLV(bool is_short);

        WORD type;          // 参数键的ID
        BYTE short_length;      // 短TLV value的长度1字节,
        WORD long_length;       // 长TLV value的长度2字节
        std::vector<BYTE> value;

        void serialize(std::vector<uint8_t> &data) const;
        bool parse(const std::vector<uint8_t> &data);
        bool is_short;
    };

    struct MessageBody {
        WORD random; // 随机数
        BYTE sid;   // 服务类型
        BYTE mid;    // 服务参数
        std::vector<TLV> content; // 消息具体内容

        void serialize(std::vector<uint8_t> &data) const;
        bool parse(const std::vector<uint8_t> &data, bool is_short_tlv = true);
    };

} // end of namespace tsp_client
