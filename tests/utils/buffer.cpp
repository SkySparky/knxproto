#include <knxproto/utils/buffer.hpp>
#include <catch.hpp>

using namespace knxproto;

TEST_CASE("put/get") {
	Buffer buf(15);

	uint8_t a1 = 120, a2;
	uint16_t b1 = 30000, b2;
	uint32_t c1 = 1200000, c2;
	uint64_t d1 = 300000000000, d2;

	REQUIRE(put(buf, a1, b1, c1, d1));
	REQUIRE(get(buf, a2, b2, c2, d2));

	REQUIRE(a1 == a2);
	REQUIRE(b1 == b2);
	REQUIRE(c1 == c2);
	REQUIRE(d1 == d2);
}
