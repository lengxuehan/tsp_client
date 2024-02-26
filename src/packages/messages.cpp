#include "messages.h"
#include "packet.h"
#include "tb_log.h"
#include "common/common.h"

namespace tsp_client {

    void MessageHeader::serialize(std::vector<uint8_t> &data) const {
        Packet pack(data);
        pack << link_header << port_version << status_code << ack_flag;
        pack.serialize(request_id, 6).serialize(tuid, 16);
        pack << encrypt_flag << body_length;
    }

    bool MessageHeader::parse(const std::vector<uint8_t> &data) {
        Packet pack(data.data(), data.size());
        pack >> link_header >> port_version >> status_code >> ack_flag;
        pack.parse(request_id, 6).parse(tuid, 16);
        pack >> encrypt_flag >> body_length;
        return pack.noerr();
    }

    uint8_t MessageHeader::get_ipc_header_size() {
        return  11U;
    }

    void MessageHeader::dump() {
        std::string str_request_id = convert_to_hex_string(request_id, 6);
        std::string str_tuid = convert_to_hex_string(tuid, 16);
        TB_LOG_INFO("MessageHeader::dump status_code:%d, ack_flag:%d, request_id:%s, tuid:%s",
                    status_code, ack_flag, str_request_id.c_str(), str_tuid.c_str());
        TB_LOG_INFO("MessageHeader::dump tuid:%s encrypt_flag:%d, body_length:%d",
                    str_tuid.c_str(), encrypt_flag, body_length);
    }

    uint8_t MessageHeader::get_header_size() {
        return 30U;
    }

    void MessageHeader::serialize_to_ipc_msg(std::vector<uint8_t> &data) {
        Packet pack(data);
        pack << status_code << ack_flag;
        pack.serialize(request_id, 6);
        pack << encrypt_flag << body_length;
    }

    bool MessageHeader::parse_from_ips_msg(const std::vector<uint8_t> &data) {
        Packet pack(data.data(), data.size());
        pack >> status_code >> ack_flag;
        pack.parse(request_id, 6);
        pack >> encrypt_flag >> body_length;
        return pack.noerr();
    }

    TLV::TLV(bool is_short) {
        this->is_short = is_short;
    }

    void TLV::serialize(std::vector<uint8_t> &data) const {
        Packet pack(data);
        pack << type;
        if(is_short){
            pack << short_length;
        } else {
            pack << long_length;
        }
        pack.serialize(value.data(), value.size());
    }

    bool TLV::parse(const std::vector<uint8_t> &data) {
        Packet pack(data.data(), data.size());
        pack >> type;
        if(is_short){
            pack >> short_length;
            value.resize(short_length);
        } else {
            pack >> long_length;
            value.resize(long_length);
        }
        pack.serialize(value.data(), value.size());
        return pack.noerr();
    }

    void MessageBody::serialize(std::vector<uint8_t> &data) const {
        Packet pack(data);
        pack << random << sid << mid;

        for(auto it : content) {
            std::vector<uint8_t> tlv;
            it.serialize(tlv);
            pack.append(tlv.data(), tlv.size());
        }
    }

    bool MessageBody::parse(const std::vector<uint8_t> &data, bool is_short_tlv) {
        Packet pack(data.data(), data.size());
        pack >> random >> sid >> mid;
        BYTE type[2];
        while (pack.has_remain_bytes()){
            TLV tlv{is_short_tlv};
            pack >> tlv.type;

            if(tlv.is_short){
                pack >> tlv.short_length;
                tlv.value.resize(tlv.short_length);
            } else {
                pack >> tlv.long_length;
                tlv.value.resize(tlv.long_length);
            }
            pack.serialize(tlv.value.data(), tlv.value.size());
            content.emplace_back(tlv);
        }
        return pack.noerr();
    }
}



