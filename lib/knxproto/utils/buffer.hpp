/* knxproto
 * Various KNX protocol implementations
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */
#ifndef KNXPROTO_UTILS_BUFFER_HPP_
#define KNXPROTO_UTILS_BUFFER_HPP_

#include "../common.hpp"
#include "natseq.hpp"

#include <arpa/inet.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>

KNXPROTO_NS_BEGIN

/**
 * Buffer element
 */
template <typename Type>
struct BufferElement {
	/**
	 * Number of bytes occupied inside the buffer
	 */
	static constexpr
	size_t Size = sizeof(Type);

	/**
	 * Retrieve the value from the buffer.
	 *
	 * \param buffer Source buffer
	 * \param output Output value
	 * \returns `true` on success
	 */
	static inline
	bool get(const uint8_t* buffer, Type& output) {
		return memcpy(std::addressof(output), buffer, Size) == std::addressof(output);
	}

	/**
	 * Put the value inside the buffer.
	 *
	 * \param buffer Target buffer
	 * \param input  Input value
	 * \returns `true` on success
	 */
	static inline
	bool put(uint8_t* buffer, const Type& input) {
		return memcpy(buffer, std::addressof(input), Size) == buffer;
	}
};

// Make sure 'Size' gets exported.
template <typename Type> constexpr
size_t BufferElement<Type>::Size;

// Redirect 'const Type' to 'Type'.
template <typename Type>
struct BufferElement<const Type>: BufferElement<Type> {};

// Redirect 'volatile Type' to 'Type'.
template <typename Type>
struct BufferElement<volatile Type>: BufferElement<Type> {};

// Redirect 'const volatile Type' to 'Type'.
template <typename Type>
struct BufferElement<const volatile Type>: BufferElement<Type> {};

// Redirect 'Type&' to 'Type'.
template <typename Type>
struct BufferElement<Type&>: BufferElement<Type> {};

// Redirect 'Type&&' to 'Type'.
template <typename Type>
struct BufferElement<Type&&>: BufferElement<Type> {};

namespace internal {
	// This struct shall receive the offsets/positions for each buffer element.
	template <size_t... Offsets>
	struct BufferInteraction {
		template <typename... Types> static inline
		bool put(uint8_t* buffer, const Types&... elems) {
			// Ensure the number of buffer elements and offsets match.
			static_assert(
				sizeof...(Types) == sizeof...(Offsets),
				"Varying number of offsets and buffer elements"
			);

			// Capture the results of each individual 'put' action within this array.
			bool results[sizeof...(Types)] {
				BufferElement<Types>::put(buffer + Offsets, elems)...
			};

			// Find the failed 'put' result, if any exists.
			for (size_t i = 0; i < sizeof...(Types); i++)
				if (!results[i])
					return false;

			return true;
		}

		template <typename... Types> static inline
		bool get(const uint8_t* buffer, Types&... elems) {
			// Ensure the number of buffer elements and offsets match.
			static_assert(
				sizeof...(Types) == sizeof...(Offsets),
				"Varying number of offsets and buffer elements"
			);

			// Capture the results of each individual 'put' action within this array.
			bool results[sizeof...(Types)] {
				BufferElement<Types>::get(buffer + Offsets, elems)...
			};

			// Find the failed 'put' result, if any exists.
			for (size_t i = 0; i < sizeof...(Types); i++)
				if (!results[i])
					return false;

			return true;
		}
	};
}

/**
 * Put a series of values into the buffer.
 *
 * \param buffer Target buffer
 * \param length Buffer length
 * \param elems  Input values
 * \returns `true` on success
 */
template <typename... Types> inline
bool put(uint8_t* buffer, size_t length, const Types&... elems) {
	// Sum of each buffer element's size must not exceed the buffer's size.
	if (length < SumSeq<BufferElement<Types>::Size...>::Result)
		return false;

	// Generate a sequence of partial sums, which is a sequence of offsets/positions for each buffer
	// element. Relay the resulting sequence to 'BufferInteraction'.
	using BufferInteraction =
		typename MakeOffsetSeq<BufferElement<Types>::Size...>::template Relay<
			internal::BufferInteraction
		>;

	return BufferInteraction::put(buffer, elems...);
}

/**
 * Retrieve a series of values from the buffer.
 *
 * \param buffer Source buffer
 * \param length Buffer length
 * \param elems  Output values
 * \returns `true` on success
 */
template <typename... Types> inline
bool get(const uint8_t* buffer, size_t length, Types&&... elems) {
	// Sum of each buffer element's size must not exceed the buffer's size.
	if (length < SumSeq<BufferElement<Types>::Size...>::Result)
		return false;

	// Generate a sequence of partial sums, which is a sequence of offsets/positions for each buffer
	// element. Relay the resulting sequence to 'BufferInteraction'.
	using BufferInteraction =
		typename MakeOffsetSeq<BufferElement<Types>::Size...>::template Relay<
			internal::BufferInteraction
		>;

	return BufferInteraction::get(buffer, static_cast<Types&>(elems)...);
}

// Convenient alias
using Buffer = std::vector<uint8_t>;

/**
 * Put a series of values into the buffer.
 *
 * \param buffer Target buffer
 * \param elems  Input values
 * \returns `true` on success
 */
template <typename... Types> inline
bool put(Buffer& buffer, const Types&... elems) {
	return put(buffer.data(), buffer.size(), elems...);
}

/**
 * Retrieve a series of values from the buffer.
 *
 * \param buffer Source buffer
 * \param elems  Output values
 * \returns `true` on success
 */
template <typename... Types> inline
bool get(const Buffer& buffer, Types&&... elems) {
	return get(buffer.data(), buffer.size(), std::forward<Types>(elems)...);
}

/**
 * Wrapper for `Type`, with the addtion that resulting buffer for that type shall exist in network
 * byte order.
 */
template <typename Type>
struct NetOrdered {
	Type& value;
};

/**
 * Specialization for elements which shall appear in network byte order in the buffer.
 */
template <typename Type>
struct BufferElement<NetOrdered<Type>> {
	static constexpr
	size_t Size = BufferElement<Type>::Size;

	static inline
	bool get(const uint8_t* buffer, NetOrdered<Type>& output) {
		if (htons(0x1337) != 0x1337) {
			uint8_t copy_buffer[Size];
			std::reverse_copy(buffer, buffer + Size, copy_buffer);

			return BufferElement<Type>::get(copy_buffer, output.value);
		} else {
			return BufferElement<Type>::get(buffer, output.value);
		}
	}

	static inline
	bool put(uint8_t* buffer, const NetOrdered<Type>& input) {
		if (!BufferElement<Type>::put(buffer, input.value))
			return false;

		if (htons(0x1337) != 0x1337)
			std::reverse(buffer, buffer + Size);

		return true;
	}
};

// Make sure this gets exported
template <typename Type> constexpr
size_t BufferElement<NetOrdered<Type>>::Size;

/**
 * Conveniently construct `NetOrdered<Type>`.
 */
template <typename Type> inline
NetOrdered<Type> netOrdered(Type&& value) {
	return {static_cast<Type&>(value)};
}

/**
 * Empty structure which will occupy `N` byte in the buffer
 */
template <size_t N>
struct Space {};

/**
 * Specialization for `Space<N>`
 */
template <size_t N>
struct BufferElement<Space<N>> {
	static constexpr
	size_t Size = N;

	static inline
	bool get(const uint8_t*, const Space<N>&) {
		return true;
	}

	static inline
	bool put(uint8_t*, const Space<N>&) {
		return true;
	}
};

// Make sure this gets exported
template <size_t N> constexpr
size_t BufferElement<Space<N>>::Size;

/**
 * Template variable to avoid multiple non-sense constructions of `Space<N>`.
 */
template <size_t N> constexpr
Space<N> space {};

KNXPROTO_NS_END

#endif
