#include "include/packages/messages.h"
#include "packet.h"

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

    TLV::TLV(bool is_short) {
        this->is_short = is_short;
    }

    void TLV::serialize(std::vector<uint8_t> &data) const {
        Packet pack(data);
        pack.serialize(type, 2);
        if(is_short){
            pack << short_length;
        } else {
            pack << long_length;
        }
        pack.serialize(value.data(), value.size());
    }

    bool TLV::parse(const std::vector<uint8_t> &data) {
        Packet pack(data.data(), data.size());
        pack.parse(type, 2);
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
        pack << random << side << mid;

        for(auto it : content) {
            std::vector<uint8_t> tlv;
            it.serialize(tlv);
            pack.append(tlv.data(), tlv.size());
        }
    }

    bool MessageBody::parse(const std::vector<uint8_t> &data, bool is_short_tlv) {
        Packet pack(data.data(), data.size());
        pack >> random >> side >> mid;
        BYTE type[2];
        while (pack.has_remain_bytes()){
            TLV tlv{is_short_tlv};
            pack.parse(type, 2);
            tlv.type[0] = type[0];
            tlv.type[1] = type[1];

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



