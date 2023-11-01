/**
* @file cmd_def.h
* @brief cmd defines.
* @author		wuting.xu
* @date		    2023/10/30
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once

namespace tsp_client {

    template <T First, T Second>
    struct UniqueTrait {};

#define CMD_DEFINE(first_value, cmd_name, data_type, second_value)	\
	template <> struct UniqueTrait<first_value, second_value> {};	\
	struct cmd_name {	\
		const static uint16 no = (uint16(first_value) << 8) + second_value;	\
		cmd_name() {	\
		}	\
		data_type	data;	\
	}

#define CMD_DECLARE(cmd_name, data_type, n)	\
	template <> struct UniqueTrait<uint8_t(n >> 8), uint8_t(n) > {};	\
	struct cmd_name {	\
		const static uint16 no = n;	\
		cmd_name() {	\
		}	\
		data_type	data;	\
	}

} // end of namespace tsp_client

