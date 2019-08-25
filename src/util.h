#ifndef UTIL_H
#define UTIL_H

/* This file defined various helper functions. */

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace StrUtil {

template <typename T>
std::size_t strlen(const T *strarg) {
	if (!strarg)
		return 0; //strarg is NULL pointer
	const T *str = strarg;
	for (; *str; ++str);
	return str-strarg;
}

template <typename T>
void strcpy(const T *source, T *target, std::size_t maxLen) {
	std::size_t size = std::min<std::size_t>(StrUtil::strlen(source), maxLen-1);
	std::memcpy(target, source, size*sizeof(T));
	target[size] = 0;
}

} // namespace StrUtil

#endif
