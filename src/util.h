#ifndef UTIL_H
#define UTIL_H

namespace StrUtil {

template <typename T>
size_t strlen(const T* strarg) {
	if (!strarg)
		return 0; //strarg is NULL pointer
	const T* str = strarg;
	for (; *str; ++str);
	return str-strarg;
}

//std::memcpy(name, name_utf16, std::min<size_t>(StrUtil::strlen(name_utf16), nameLen));

template <typename T>
void strcpy(const T* source, T* target, size_t maxLen) {
	size_t size = std::min<size_t>(StrUtil::strlen(source), maxLen-1);
	std::memcpy(target, source, size);
	target[size] = 0;
}

}

#endif
