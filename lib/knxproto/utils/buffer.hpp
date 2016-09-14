/* knxproto
 * Various KNX protocol implementations
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */
#ifndef KNXPROTO_UTILS_BUFFER_HPP_
#define KNXPROTO_UTILS_BUFFER_HPP_

#include "../common.hpp"
#include "natseq.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

KNXPROTO_NS_BEGIN

template <typename Type>
struct BufferElement {
	static constexpr
	size_t Size = sizeof(Type);

	static inline
	bool get(const uint8_t* buffer, Type& output) {
		return memcpy(std::addressof(output), buffer, Size) == std::addressof(output);
	}

	static inline
	bool put(uint8_t* buffer, const Type& input) {
		return memcpy(buffer, std::addressof(input), Size) == buffer;
	}
};

template <typename Type> constexpr
size_t BufferElement<Type>::Size;

template <typename Type>
struct BufferElement<const Type> {
	static constexpr
	size_t Size = BufferElement<Type>::Size;

	static inline
	bool put(uint8_t* buffer, const Type& input) {
		return BufferElement<Type>::put(buffer, input);
	}
};

template <typename Type> constexpr
size_t BufferElement<const Type>::Size;

template <typename Type>
struct BufferElement<Type&>: BufferElement<Type> {};

template <typename Type>
struct BufferElement<Type&&>: BufferElement<Type> {};

namespace internal {
	template <size_t... Offsets>
	struct BufferInteraction {
		template <typename... Types> static inline
		bool put(uint8_t* buffer, const Types&... elems) {
			static_assert(
				sizeof...(Types) == sizeof...(Offsets),
				"Varying number of offsets and buffer elements"
			);

			bool results[sizeof...(Types)] {
				BufferElement<Types>::put(buffer + Offsets, elems)...
			};

			for (size_t i = 0; i < sizeof...(Types); i++)
				if (!results[i])
					return false;

			return true;
		}

		template <typename... Types> static inline
		bool get(const uint8_t* buffer, Types&... elems) {
			static_assert(
				sizeof...(Types) == sizeof...(Offsets),
				"Varying number of offsets and buffer elements"
			);

			bool results[sizeof...(Types)] {
				BufferElement<Types>::get(buffer + Offsets, elems)...
			};

			for (size_t i = 0; i < sizeof...(Types); i++)
				if (!results[i])
					return false;

			return true;
		}
	};
}

template <typename... Types> inline
bool put(uint8_t* buffer, size_t length, const Types&... elems) {
	if (length < SumSeq<BufferElement<Types>::Size...>::Result)
		return false;

	using BufferInteraction =
		typename MakeOffsetSeq<BufferElement<Types>::Size...>::template Relay<
			internal::BufferInteraction
		>;

	return BufferInteraction::put(buffer, elems...);
}

template <typename... Types> inline
bool get(const uint8_t* buffer, size_t length, Types&... elems) {
	if (length < SumSeq<BufferElement<Types>::Size...>::Result)
		return false;

	using BufferInteraction =
		typename MakeOffsetSeq<BufferElement<Types>::Size...>::template Relay<
			internal::BufferInteraction
		>;

	return BufferInteraction::get(buffer, elems...);
}

using Buffer = std::vector<uint8_t>;

template <typename... Types> inline
bool put(Buffer& buffer, const Types&... elems) {
	return put(buffer.data(), buffer.size(), elems...);
}

template <typename... Types> inline
bool get(const Buffer& buffer, Types&... elems) {
	return get(buffer.data(), buffer.size(), elems...);
}

KNXPROTO_NS_END

#endif
