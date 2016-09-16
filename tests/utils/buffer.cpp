#include <knxproto/utils/buffer.hpp>
#include <catch.hpp>

using namespace knxproto;

TEST_CASE("put/get") {
	Buffer buf(15);

	uint8_t a1 = 0xAB, a2;
	uint16_t b1 = 0xCDEF, b2;
	uint32_t c1 = 0xABCDEF98, c2;
	uint64_t d1 = 0x76543210FEDCBA89, d2;

	REQUIRE(put(buf, a1, b1, c1, d1));
	REQUIRE(get(buf, a2, b2, c2, d2));

	REQUIRE(a1 == a2);
	REQUIRE(b1 == b2);
	REQUIRE(c1 == c2);
	REQUIRE(d1 == d2);
}

TEST_CASE("netOrdered") {
	Buffer buf(sizeof(uint16_t));

	uint16_t x = 0x1337;
	REQUIRE(put(buf, netOrdered(x)));

	uint16_t y;
	REQUIRE(get(buf, y));
	REQUIRE(y == htons(x));

	REQUIRE(get(buf, netOrdered(y)));
	REQUIRE(x == y);
}

TEST_CASE("space") {
	Buffer buf(sizeof(uint16_t) * 2);

	uint16_t x = 0x1337;
	REQUIRE(put(buf, space<sizeof(uint16_t)>, x));

	uint16_t y;
	REQUIRE(get(buf, space<sizeof(uint16_t)>, y));
	REQUIRE(x == y);
}
