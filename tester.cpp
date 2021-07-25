#include "bitpacker.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace BitPacker;

int randomTestBit(const uint32_t Size) {
  const uint32_t MaxValueByte = getMaxValueForBytes(sizeof(uint8_t));
  auto ArrWrite = new uint32_t[Size];
  printf("generating pattern size %d\n", Size);
  for (uint32_t i = 0; i < Size; i++) {
    ArrWrite[i] = rand() % (MaxValueByte + 1);
  }
  auto ArrRead = new uint32_t[Size];
  auto Buffer = new uint32_t[Size];

  printf("writing buffer\n");
  BufferWriter MyBufferWritter(Buffer, Size);
  for (uint32_t i = 0; i < Size; i++) {
    MyBufferWritter.WriteBits(ArrWrite[i], sizeof(uint8_t) * c_NumBitsPerByte);
  }
  MyBufferWritter.Flush();
  printf("reading buffer\n");
  BufferReader MyBufferReader(Buffer, Size);
  for (uint32_t i = 0; i < Size; i++) {
    MyBufferReader.ReadBits(ArrRead[i], sizeof(uint8_t) * c_NumBitsPerByte);
  }

  int32_t res = memcmp(ArrWrite, ArrRead, Size * sizeof(uint32_t));

  delete[] Buffer;
  delete[] ArrWrite;
  delete[] ArrRead;

  return res;
}

int sanity_check_buffer_read() {
  printf("sanity check buffer read\n");
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BufferReader r(ArrFake, 0);
  return !r.ReadBits(Value, BitPacker::c_NumBitsPerWord) ? 0 : 1;
}

int sanity_check_buffer_write() {
  printf("sanity check buffer write\n");
  uint32_t Value = 0;
  uint32_t ArrFake[1];
  BufferWriter w(ArrFake, 0);
  return !w.WriteBits(Value, BitPacker::c_NumBitsPerWord) ? 0 : 1;
}

int main(int argc, char **argv) {
  const size_t c_Seed = 0xFEFE;
  srand(c_Seed);
  if (argc <= 1) {
    printf("no test selected \n");
    return 0;
  }
  const char *testType = argv[1];
  if (strcmp("bit", testType) == 0) {
    uint32_t size = atoi(argv[2]);
    return randomTestBit(size);
  }

  if (strcmp("sanity_bit_read", testType) == 0) {
    return sanity_check_buffer_read();
  }

  if (strcmp("sanity_bit_write", testType) == 0) {
    return sanity_check_buffer_write();
  }

  return 0;
}
