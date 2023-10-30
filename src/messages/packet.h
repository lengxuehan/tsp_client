/**
* @file package.h
* @brief Packet is the class providing Serializing or deserializing messages which send to tsp server.
* @author		wuting.xu
* @date		    2023/10/30
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once

#include <map>
#include <list>
#include <vector>
#include "utility/common.h"
namespace tsp_client {
class Packet {
public:
	explicit Packet(std::vector<uint8_t> &buf);
	Packet(const uint8_t *p, uint32_t size);
	~Packet();

	bool noerr() const;

    bool has_remain_bytes() const;

	Packet& append(const uint8_t * buffer, uint32_t len);

	Packet& operator<<(float value);
	Packet& operator>>(float &value);

	Packet& operator<<(double value);
	Packet& operator>>(double &value);

	Packet& operator<<(bool value);
	Packet& operator>>(bool &value);

	Packet& operator<<(int8_t value);
	Packet& operator>>(int8_t &value);

	Packet& operator<<(int16_t value);
	Packet& operator>>(int16_t &value);

	Packet& operator<<(int32_t value);
	Packet& operator>>(int32_t &value);

	Packet& operator<<(int64_t value);
	Packet& operator>>(int64_t &value);

	Packet& operator<<(uint8_t value);
	Packet& operator>>(uint8_t &value);

    Packet& serialize(const uint8_t* value, uint8_t count);
    Packet& parse(uint8_t* value, uint8_t count);

	Packet& operator<<(uint16_t value);
	Packet& operator>>(uint16_t &value);

	Packet& operator<<(uint32_t value);
	Packet& operator>>(uint32_t &value);

	Packet& operator<<(uint64_t value);
	Packet& operator>>(uint64_t &value);

	template <typename T>
	Packet& operator<<(const T &value) {
		return set(value);
	}

	template <typename T>
	Packet& operator>>(T &value) {
		return get(value);
	}

	template <typename T>
	Packet& operator>>(std::list<T> &list) {
		uint32_t size = 0;
		get(size);
		if (error_) {
			return *this;
		}

		for (uint32_t i = 0; i < size; ++i) {
			T v;
			get(v);
			if (error_) {
				return *this;
			}

			list.push_back(v);
		}

		return *this;
	}

	template <typename T>
	Packet& operator<<(const std::list<T> &list) {
		set(uint32_t(list.size()));
		for (auto it = list.begin(), end = list.end(); it != end; ++it) {
			set(*it);
		}

		return *this;
	}

	template <typename T>
	Packet& operator>>(std::vector<T> &list) {
		uint32_t size = 0;
		get(size);
		if (error_) {
			return *this;
		}

		for (uint32_t i = 0; i < size; ++i) {
			T v;
			get(v);
			if (error_) {
				return *this;
			}

			list.push_back(v);
		}

		return *this;
	}

	template <typename T>
	Packet& operator<<(const std::vector<T> &list) {
		set(uint32_t(list.size()));
		for (auto it = list.begin(), end = list.end(); it != end; ++it) {
			set(*it);
		}

		return *this;
	}

	template <typename K, typename V>
	Packet& operator>>(std::map<K, V> &m) {
		uint32_t size = 0;
		get(size);
		if (error_) {
			return *this;
		}

		for (uint32_t i = 0; i < size; ++i) {
			K k;
			V v;
			get(k).get(v);
			if (error_) {
				return *this;
			}

			m[k] = v;
		}

		return *this;
	}

	template <typename K, typename V>
	Packet& operator<<(const std::map<K, V> &m) {
		set(uint32_t(m.size()));
		for (auto it = m.begin(), end = m.end(); it != end; ++it) {
			set(it->first).set(it->second);
		}

		return *this;
	}

private:
	DISALLOW_EVIL_CONSTRUCTORS(Packet);

#define NUMERIC_TYPE_FUNC(type)	\
	Packet& set(type v) {	\
		return set((const void *)&v, uint16_t(sizeof(type)));	\
	}	\
	\
	Packet& get(type &v) {	\
		if (error_) {	\
			return *this;	\
		}	\
		return get(&v, sizeof(type));	\
	}

	NUMERIC_TYPE_FUNC(int8_t);
	NUMERIC_TYPE_FUNC(int16_t);
	NUMERIC_TYPE_FUNC(int32_t);
	NUMERIC_TYPE_FUNC(int64_t);
	NUMERIC_TYPE_FUNC(uint8_t);
	NUMERIC_TYPE_FUNC(uint16_t);
	NUMERIC_TYPE_FUNC(uint32_t);
	NUMERIC_TYPE_FUNC(uint64_t);
	NUMERIC_TYPE_FUNC(bool);
	NUMERIC_TYPE_FUNC(float);
	NUMERIC_TYPE_FUNC(double);

	template <typename T>
	Packet& set(const T &v) {
		v.Serialize(*this);
		return *this;
	}

	template <typename T>
	Packet& set(const T *v) {
		v->Serialize(*this);
		return *this;
	}

	Packet& get(std::vector<uint8_t> &buf);
	Packet& get(void *p, uint32_t size);
	Packet& set(const uint8_t *p, uint16_t size);
	Packet& set(const void *p, uint32_t size);

private:
	uint32_t		pos_;
	uint32_t		size_;
	const uint8_t	*ptr_;
	std::vector<uint8_t>	*buf_;
	bool		error_;
};
} // end of namespace  tsp_client