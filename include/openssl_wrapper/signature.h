//
// Created by wuting.xu on 2023/11/22.
//

#ifndef TSP_CLIENT_MD5_H
#define TSP_CLIENT_MD5_H

#include <stdint.h>
int openssl_sha256(const unsigned char *data,  uint32_t data_size, unsigned char *out, uint32_t *out_size);

int openssl_md5(const unsigned char *data,  uint32_t data_size, unsigned char *out, uint32_t *out_size);
#endif //TSP_CLIENT_MD5_H
