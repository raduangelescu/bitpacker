#include "bitpacker.h"
#include <catch2/catch.hpp>
#include <cstring>
#include <limits>

using namespace BitPacker;

int randomTestBit(const uint32_t Size) {
  constexpr uint32_t MaxValueByte = getMaxValueForBytes(sizeof(uint8_t));
  auto ArrWrite = new uint32_t[Size];
  for (uint32_t i = 0; i < Size; i++) {
    ArrWrite[i] = static_cast<uint32_t>(rand()) % (MaxValueByte + 1);
  }
  auto ArrRead = new uint32_t[Size];
  auto Buffer = new uint32_t[Size];

  BitWriter MyBufferWritter(Buffer, Size);
  for (uint32_t i = 0; i < Size; i++) {
    MyBufferWritter.WriteBits(ArrWrite[i], sizeof(uint8_t) * c_NumBitsPerByte);
  }
  MyBufferWritter.Flush();
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
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitReader r(ArrFake, 0);
  return !r.ReadBits(Value, BitPacker::c_NumBitsPerWord) ? 0 : 1;
}

int sanityCheckBufferReadAbove32() {
  const uint32_t HugeNumberOfBits = 128;
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitReader w(ArrFake, 1);
  return !w.ReadBits(Value, HugeNumberOfBits) ? 0 : 1;
}

int sanityCheckBufferWrite() {
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitWriter w(ArrFake, 0);
  return !w.WriteBits(Value, BitPacker::c_NumBitsPerWord) ? 0 : 1;
}

int sanityCheckBufferWriteAbove32() {
  const uint32_t HugeNumberOfBits = 128;
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BitWriter w(ArrFake, 1);
  return !w.WriteBits(Value, HugeNumberOfBits) ? 0 : 1;
}

bool AreSame(float a, float b) {
  return std::fabs(a - b) < std::numeric_limits<float>::epsilon();
}

struct STestRandomStruct {
  uint32_t Id;
  uint32_t ValueUnsignedInt;
  float ValueFloat;
  float ValueFloatCompressed;
  STestRandomStruct() {
    Id = static_cast<uint32_t>(rand());
    ValueUnsignedInt = static_cast<uint32_t>(rand());
    ValueFloat = static_cast<float>(rand()) / static_cast<float>(rand() + 1);
    ValueFloatCompressed = 1.2f;
  }

  template <typename Stream> bool Serialize(Stream &stream) {
    BitPackInt(stream, Id, 0, UINT32_MAX);
    BitPackInt(stream, ValueUnsignedInt, 0, UINT32_MAX);
    BitPackFloat(stream, ValueFloat);
    BitPackCompressedFloat(stream, ValueFloatCompressed, 0.0f, 100.0f, 0.01f);
    stream.Flush();
    return true;
  }

  bool operator==(const STestRandomStruct &other) {
    return Id == other.Id && ValueUnsignedInt == other.ValueUnsignedInt &&
           AreSame(ValueFloat, other.ValueFloat) &&
           AreSame(ValueFloatCompressed, other.ValueFloatCompressed);
  }
};

bool structSerializeTest() {
  const uint32_t Bytes = sizeof(STestRandomStruct);
  uint8_t Buffer[Bytes];

  STestRandomStruct RandomStructInitial;
  STestRandomStruct RandomStructRead;
  WriterStream WriteStream(Buffer, Bytes);
  ReaderStream ReadStream(Buffer, Bytes);
  RandomStructInitial.Serialize(WriteStream);
  RandomStructRead.Serialize(ReadStream);
  return RandomStructInitial == RandomStructRead;
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

TEST_CASE("Bits Required Util", "[Util]") {
  REQUIRE(getNumberOfBitsForRange(0, 256) == 9);
  REQUIRE(getNumberOfBitsForRange(0, 127) == 7);
  REQUIRE(getNumberOfBitsForRange(5, 127) == 7);
  REQUIRE(getNumberOfBitsForRange(121, 121) == 0);
  REQUIRE(getNumberOfBitsForRange(121, 122) == 1);
}

TEST_CASE("Stream Read And Write Random Structure", "[StreamRead/Write]") {
  REQUIRE(structSerializeTest() == true);
  REQUIRE(structSerializeTest() == true);
  REQUIRE(structSerializeTest() == true);
  REQUIRE(structSerializeTest() == true);
  REQUIRE(structSerializeTest() == true);
}
