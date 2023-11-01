/**
* @file package_helper.h
* @author		wuting.xu
* @date		    2023/11/1
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once

#include <vector>
#include <stdint.h>

namespace tsp_client {
class PackHelper {
public:
    static uint8_t get_message_header_size();
    static uint32_t parse_message_header(const uint8_t *data, uint32_t size);

};
} // end of namespace  tsp_client