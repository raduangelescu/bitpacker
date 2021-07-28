#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>

#include "bitpacker.h"

using namespace BitPacker;

int randomTestBit(const uint32_t Size) {
  constexpr uint32_t MaxValueByte = getMaxValueForBytes(sizeof(uint8_t));
  auto ArrWrite = new uint32_t[Size];
  spdlog::info("Generating pattern size {0:d}");
  for (uint32_t i = 0; i < Size; i++) {
    ArrWrite[i] = rand() % (MaxValueByte + 1);
  }
  auto ArrRead = new uint32_t[Size];
  auto Buffer = new uint32_t[Size];

  spdlog::info("Writing buffer");
  BitWriter MyBufferWritter(Buffer, Size);
  for (uint32_t i = 0; i < Size; i++) {
    MyBufferWritter.WriteBits(ArrWrite[i], sizeof(uint8_t) * c_NumBitsPerByte);
  }
  MyBufferWritter.Flush();
  spdlog::info("Reading buffer");
  BitReader MyBufferReader(Buffer, Size);
  for (uint32_t i = 0; i < Size; i++) {
    MyBufferReader.ReadBits(ArrRead[i], sizeof(uint8_t) * c_NumBitsPerByte);
  }

  int32_t res = memcmp(ArrWrite, ArrRead, Size * sizeof(uint32_t));

  delete[] Buffer;
  delete[] ArrWrite;
  delete[] ArrRead;

  return res;
}

int sanityCheckBufferRead() {
  spdlog::info("Sanity check buffer read");
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitReader r(ArrFake, 0);
  return !r.ReadBits(Value, BitPacker::c_NumBitsPerWord) ? 0 : 1;
}

int sanityCheckBufferReadAbove32() {
  spdlog::info("Sanity check buffer read big number of bits");
  const uint32_t HugeNumberOfBits = 128;
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitReader w(ArrFake, 1);
  return !w.ReadBits(Value, HugeNumberOfBits) ? 0 : 1;
}

int sanityCheckBufferWrite() {
  spdlog::info("Sanity check buffer write");
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitWriter w(ArrFake, 0);
  return !w.WriteBits(Value, BitPacker::c_NumBitsPerWord) ? 0 : 1;
}

int sanityCheckBufferWriteAbove32() {
  spdlog::info("Sanity check buffer write big number of bits");
  const uint32_t HugeNumberOfBits = 128;
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitWriter w(ArrFake, 1);
  return !w.WriteBits(Value, HugeNumberOfBits) ? 0 : 1;
}

TEST_CASE("Random Test Bit", "[bitWriter/Reader]") {
  REQUIRE(randomTestBit(1) == 0);
  REQUIRE(randomTestBit(13) == 0);
  REQUIRE(randomTestBit(10) == 0);
  REQUIRE(randomTestBit(100) == 0);
  REQUIRE(randomTestBit(1000) == 0);
}

TEST_CASE("Sanity Check Buffer Read", "[Reader]") {
  REQUIRE(sanityCheckBufferRead() == 0);
  REQUIRE(sanityCheckBufferReadAbove32() == 0);
}
TEST_CASE("Sanity Check Buffer Write", "[BitWriter/Reader]") {
  REQUIRE(sanityCheckBufferWrite() == 0);
  REQUIRE(sanityCheckBufferWriteAbove32() == 0);
}

TEST_CASE("Bits Required Util", "[util]") {
  REQUIRE(getNumberOfBitsForRange(0, 256) == 9);
  REQUIRE(getNumberOfBitsForRange(0, 127) == 7);
  REQUIRE(getNumberOfBitsForRange(5, 127) == 7);
  REQUIRE(getNumberOfBitsForRange(121, 121) == 0);
  REQUIRE(getNumberOfBitsForRange(121, 122) == 1);
}
