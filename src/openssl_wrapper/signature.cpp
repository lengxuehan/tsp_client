//
// Created by wuting.xu on 2023/11/22.
//
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "openssl_wrapper/signature.h"

int openssl_sha256(const unsigned char *data, uint32_t data_size, unsigned char *out, uint32_t *out_size) {
    int nResult;

    SHA256_CTX sha256_ctx;

    nResult = SHA256_Init(&sha256_ctx);
    if (nResult < 0) {
        return nResult;
    }
    nResult = SHA256_Update(&sha256_ctx, data, data_size);
    if (nResult < 0) {
        return nResult;
    }
    nResult = SHA256_Final((uint8_t *) out, &sha256_ctx);
    if (nResult >= 0) {
        *out_size = SHA256_DIGEST_LENGTH;
    }

    return nResult; // 1 for success, 0 otherwise
}

int openssl_md5(const unsigned char *data, uint32_t data_size, unsigned char *out, uint32_t *out_size) {
    int nResult;

    MD5_CTX md5_ctx;

    nResult = MD5_Init(&md5_ctx);
    if (nResult < 0) {
        return nResult;
    }
    nResult = MD5_Update(&md5_ctx, data, data_size);
    if (nResult < 0) {
        return nResult;
    }
    nResult = MD5_Final((uint8_t *) out, &md5_ctx);
    if (nResult >= 0) {
        *out_size = MD5_DIGEST_LENGTH;
    }

    return nResult; // 1 for success, 0 otherwise
}
