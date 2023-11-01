#include "packet.h"
#include <cstring>

namespace tsp_client {
    Packet::Packet(std::vector<uint8_t> &buf)
            : pos_(0), size_(0), ptr_(nullptr), buf_(&buf), error_(false) {
        buf_->reserve(256);
    }

    Packet::Packet(const uint8_t *p, uint32_t size)
            : pos_(0), size_(size), ptr_(p), buf_(nullptr), error_(false) {
    }

    Packet::~Packet() {
    }

    Packet &Packet::operator<<(double value) {
        return set(value);
    }

    Packet &Packet::operator>>(double &value) {
        return get(value);
    }

    Packet &Packet::operator<<(float value) {
        return set(value);
    }

    Packet &Packet::operator>>(float &value) {
        return get(value);
    }

    Packet &Packet::operator<<(bool value) {
        return set(value);
    }

    Packet &Packet::operator>>(bool &value) {
        return get(value);
    }

    Packet &Packet::operator<<(int8_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(int8_t &value) {
        return get(value);
    }

    Packet &Packet::operator<<(int16_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(int16_t &value) {
        return get(value);
    }

    Packet &Packet::operator<<(int32_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(int32_t &value) {
        return get(value);
    }

    Packet &Packet::operator<<(int64_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(int64_t &value) {
        return get(value);
    }

    Packet &Packet::operator<<(uint8_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(uint8_t &value) {
        return get(value);
    }

    Packet &Packet::serialize(const uint8_t* value, uint8_t count) {
        for(uint8_t i = 0; i < count; ++i){
            *this << *(value + i);
        }
        return *this;
    }

    Packet &Packet::parse(uint8_t* value, uint8_t count) {
        for(uint8_t i = 0; i < count; ++i){
            *this >> *(value + i);
        }
        return *this;
    }

    Packet &Packet::operator<<(uint16_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(uint16_t &value) {
        return get(value);
    }

    Packet &Packet::operator<<(uint32_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(uint32_t &value) {
        return get(value);
    }

    Packet &Packet::operator<<(uint64_t value) {
        return set(value);
    }

    Packet &Packet::operator>>(uint64_t &value) {
        return get(value);
    }

    Packet &Packet::append(const uint8_t *buffer, uint32_t len) {
        return set((const void *) buffer, len);
    }

    bool Packet::noerr() const {
        return !error_;
    }

    bool Packet::has_remain_bytes() const {
        return size_ == pos_ + 1;
    }

    Packet &Packet::set(const uint8_t *p, uint16_t size) {
        set(&size, sizeof(uint16_t));
        return set((const void *) p, uint32_t(size));
    }

    Packet &Packet::set(const void *p, uint32_t size) {
        if (size > buf_->capacity() - pos_) {
            buf_->reserve(buf_->capacity() + size * 2);
            return set(p, size);
        }

        buf_->resize(pos_ + size);
        memcpy(&((*buf_)[pos_]), p, size);
        pos_ += size;

        return *this;
    }

    Packet &Packet::get(std::vector<uint8_t> &buf) {
        uint16_t size = 0;
        get(size);
        if (error_) {
            return *this;
        }

        buf.reserve(size);
        buf.resize(size);

        get(&buf[0], size);

        return *this;
    }

    Packet &Packet::get(void *p, uint32_t size) {
        if (error_) {
            return *this;
        }

        if (size > size_ - pos_) {
            error_ = true;
            return *this;
        }

        memcpy(p, ptr_ + pos_, size);
        pos_ += size;

        return *this;
    }
}

