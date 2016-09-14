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

struct Test {
	uint16_t value;
};

TEST_CASE("_") {
	Buffer buf(sizeof(Test) * 2);

	Test x;
	REQUIRE(get(buf, x));

	REQUIRE(get(buf, Test {}));
}
