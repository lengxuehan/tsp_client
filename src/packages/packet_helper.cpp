#include "packet_helper.h"
#include "include/packages/messages.h"

namespace tsp_client {

    uint32_t PackHelper::get_message_header_size() {
        return sizeof(MessageHeader);
    }

    uint32_t PackHelper::parse_message_header(const uint8_t *data, uint32_t size) {
        if (size < sizeof(MessageHeader)) {
            return 0;
        }
        MessageHeader *header = (MessageHeader *) data;
        return header->body_length;
    }
}

