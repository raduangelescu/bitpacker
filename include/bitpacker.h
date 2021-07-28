#ifndef BIT_PACKER_H__
#define BIT_PACKER_H__

#include <cstdint>

namespace BitPacker {

static const uint32_t c_NumBitsPerWord = 32;
static const uint32_t c_NumBitsPerByte = 8;

static const uint64_t c_LUTMask[] = {
    0000000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f,
    0x0000003f, 0x0000007f, 0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff, 0x0001ffff,
    0x0003ffff, 0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff,
    0x3fffffff, 0x7fffffff, 0xffffffff};

constexpr uint32_t getNumberOfBits(uint32_t x) {
  return x < 2 ? x : 1 + getNumberOfBits(x >> 1);
}

constexpr uint32_t getNumberOfBitsForRange(uint32_t min, uint32_t max) {
  return getNumberOfBits(max - min);
}

constexpr uint32_t getMaxValueForBits(uint32_t bits) { return (1 << bits) - 1; }

constexpr uint32_t getMaxValueForBytes(uint32_t bytes) {
  return (1 << (bytes * c_NumBitsPerByte)) - 1;
}

class BitReader {
  uint64_t m_Scratch;
  int32_t m_ScratchBits;
  int32_t m_TotalBits;
  uint32_t m_WordIndex;
  uint32_t *m_Buffer;

public:
  BitReader(uint32_t *buffer, const uint32_t size)
      : m_Scratch(0), m_ScratchBits(0), m_TotalBits(size * c_NumBitsPerWord),
        m_WordIndex(0), m_Buffer(buffer) {}

  bool ReadBits(uint32_t &value, const uint32_t bits) {
    if (m_TotalBits <= 0) {
      return false;
    }

    if (bits > c_NumBitsPerWord) {
      return false;
    }

    if (m_ScratchBits <= 0) {
      m_Scratch = ((uint64_t)m_Buffer[m_WordIndex] << m_ScratchBits);
      m_ScratchBits += c_NumBitsPerWord;
      m_WordIndex += 1;
    }

    value = (uint32_t)(m_Scratch & (uint64_t)c_LUTMask[bits]);
    m_Scratch = m_Scratch >> bits;
    m_ScratchBits -= bits;
    m_TotalBits -= bits;

    return true;
  }

  bool WouldReadPastEnd(const int32_t bits) const {
    return (m_TotalBits - bits) >= 0;
  }
};

class BitWriter {
  uint64_t m_Scratch;
  uint32_t m_ScratchBits;
  uint32_t m_WordIndex;
  uint32_t *m_Buffer;
  uint32_t m_BufferSize;

public:
  BitWriter(uint32_t *buffer, const uint32_t size)
      : m_Scratch(0), m_ScratchBits(0), m_WordIndex(0), m_Buffer(buffer),
        m_BufferSize(size) {}

  bool IsBufferOverrun(const uint32_t bits) const {
    if (m_ScratchBits + bits < c_NumBitsPerWord) {
      return false;
    }

    if (m_WordIndex + 1 < m_BufferSize) {
      return false;
    }

    return true;
  }

  void Flush() {
    m_Buffer[m_WordIndex] = (uint32_t)m_Scratch;
    m_ScratchBits = 0;
  }

  bool WriteBits(const uint32_t value, const uint32_t bits) {

    if (m_BufferSize <= 0) {
      return false;
    }

    if (bits > c_NumBitsPerWord) {
      return false;
    }

    const uint64_t writeMask = (c_LUTMask[bits]) << m_ScratchBits;
    m_Scratch = m_Scratch | (((uint64_t)value << m_ScratchBits) & writeMask);
    m_ScratchBits += bits;
    if (m_ScratchBits > c_NumBitsPerWord) {
      m_Buffer[m_WordIndex] = (uint32_t)m_Scratch;
      m_Scratch = m_Scratch >> c_NumBitsPerWord;
      m_ScratchBits -= c_NumBitsPerWord;
      m_WordIndex++;

      if (m_WordIndex >= m_BufferSize) {
        return false;
      }
    }
    return true;
  }
};
class WriterStream {
public:
  enum { IsWriting = 1 };
  enum { IsReading = 0 };
  WriterStream(uint8_t *buffer, uint32_t bytes)
      : m_Writer((uint32_t *)buffer, bytes) {}

  bool serializeInteger(int32_t Value, int32_t Min, int32_t Max) {
    if (Min > Max) {
      return false;
    }

    if (Value < Min) {
      return false;
    }

    if (Value > Max) {
      return false;
    }

    const int Bits = getNumberOfBitsForRange(Min, Max);
    uint32_t UnsignedValue = Value - Min;
    bool Result = m_Writer.WriteBits(UnsignedValue, Bits);
    return Result;
  }

private:
  BitWriter m_Writer;
};

class ReaderStream {
public:
  enum { IsWriting = 0 };
  enum { IsReading = 1 };

  ReaderStream(const uint8_t *Buffer, int Bytes)
      : m_Reader((uint32_t *)Buffer, Bytes) {}

  bool SerializeInteger(int32_t &Value, int32_t Min, int32_t Max) {
    if (Max < Min) {
      return false;
    }
    const int Bits = getNumberOfBitsForRange(Min, Max);
    if (m_Reader.WouldReadPastEnd(Bits)) {
      return false;
    }
    uint32_t UnsignedValue;
    bool Result = m_Reader.ReadBits(UnsignedValue, Bits);
    if (!Result) {
      return false;
    }
    Value = (int32_t)UnsignedValue + Min;
    return true;
  }

private:
  BitReader m_Reader;
};

} // namespace BitPacker

#endif //BIT_PACKER_H__