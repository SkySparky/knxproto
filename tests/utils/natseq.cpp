#include <knxproto/utils/natseq.hpp>
#include <catch.hpp>
#include <type_traits>

using namespace knxproto;

TEST_CASE("NatSeq::OffsetBy") {
	SECTION("empty") {
		using B = NatSeq<>;
		using A = B::OffsetBy<10>;

		REQUIRE((std::is_same<A, B>::value));
	}

	SECTION("non-empty") {
		using A = NatSeq<1, 0, 2, 0>::OffsetBy<2>;
		using B = NatSeq<3, 2, 4, 2>;

		REQUIRE((std::is_same<A, B>::value));
	}
}

template <size_t...>
struct NatSeqTest {};

TEST_CASE("NatSeq::Relay") {
	SECTION("to self") {
		using B = NatSeq<0, 1, 2, 3>;
		using A = B::Relay<NatSeq>;

		REQUIRE((std::is_same<A, B>::value));
	}

	SECTION("to other") {
		using A = NatSeq<0, 1, 2, 3>::Relay<NatSeqTest>;
		using B = NatSeqTest<0, 1, 2, 3>;

		REQUIRE((std::is_same<A, B>::value));
	}
}

TEST_CASE("MakeOffsetSeq") {
	SECTION("empty") {
		using A = MakeOffsetSeq<>;
		using B = NatSeq<>;

		REQUIRE((std::is_same<A, B>::value));
	}

	SECTION("non-empty") {
		using A = MakeOffsetSeq<2, 4, 2, 8>;
		using B = NatSeq<0, 2, 6, 8>;

		REQUIRE((std::is_same<A, B>::value));
	}
}

TEST_CASE("SumSeq") {
	SECTION("empty") {
		REQUIRE(SumSeq<>::Result == 0);
	}

	SECTION("non-empty") {
		REQUIRE((SumSeq<1, 0, 2, 0, 3>::Result == 6));
	}
}
