/* knxproto
 * Various KNX protocol implementations
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */
#ifndef KNXPROTO_UTILS_NATSEQ_HPP_
#define KNXPROTO_UTILS_NATSEQ_HPP_

#include <cstddef>

namespace knxproto {
	template <size_t... Seq>
	struct NatSeq {
		template <size_t Offset>
		using OffsetBy = NatSeq<Offset + Seq...>;

		template <template <size_t...> class Receiver>
		using Relay = Receiver<Seq...>;
	};

	namespace internal {
		template <size_t, typename, size_t...>
		struct MakeOffsetSeq {};

		template <size_t Pos, typename ResultSeq>
		struct MakeOffsetSeq<Pos, ResultSeq> {
			using Result = ResultSeq;
		};

		template <size_t Pos, size_t... OffsetSeq, size_t Head, size_t... Tail>
		struct MakeOffsetSeq<Pos, NatSeq<OffsetSeq...>, Head, Tail...>:
			MakeOffsetSeq<Pos + Head, NatSeq<OffsetSeq..., Pos>, Tail...>
		{};
	}

	template <size_t... Seq>
	using MakeOffsetSeq = typename internal::MakeOffsetSeq<0, NatSeq<>, Seq...>::Result;

	namespace internal {
		template <size_t, size_t...>
		struct SumSeq;

		template <size_t Carry>
		struct SumSeq<Carry> {
			static constexpr
			size_t Result = Carry;
		};

		template <size_t Carry> constexpr
		size_t SumSeq<Carry>::Result;

		template <size_t Carry, size_t Head, size_t... Tail>
		struct SumSeq<Carry, Head, Tail...>:
			SumSeq<Carry + Head, Tail...> {};
	}

	template <size_t... Seq>
	using SumSeq = internal::SumSeq<0, Seq...>;
}

#endif
