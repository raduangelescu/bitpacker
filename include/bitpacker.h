#ifndef BIT_PACKER_H__
#define BIT_PACKER_H__

#include <algorithm>
#include <cassert>
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

constexpr uint32_t getNumberOfBitsForRange(int32_t min, int32_t max) {
  assert(max >= min);
  return getNumberOfBits(static_cast<uint32_t>(max - min));
}

constexpr uint32_t getMaxValueForBits(uint32_t bits) {
  return static_cast<uint32_t>((1 << bits) - 1);
}

constexpr uint32_t getMaxValueForBytes(uint32_t bytes) {
  return static_cast<uint32_t>((1 << (bytes * c_NumBitsPerByte)) - 1);
}

class BitReader {
  uint64_t m_Scratch;
  int32_t m_ScratchBits;
  int32_t m_TotalBits;
  uint32_t m_WordIndex;
  uint32_t *m_Buffer;

public:
  BitReader(uint32_t *buffer, const uint32_t size)
      : m_Scratch(0), m_ScratchBits(0),
        m_TotalBits(static_cast<int32_t>(size * c_NumBitsPerWord)),
        m_WordIndex(0), m_Buffer(buffer) {}

  bool ReadBits(uint32_t &value, const uint32_t bits) {
    if (m_TotalBits <= 0) {
      return false;
    }

    if (bits > c_NumBitsPerWord) {
      return false;
    }

    if (m_ScratchBits <= 0) {
      m_Scratch =
          (static_cast<uint64_t>(m_Buffer[m_WordIndex]) << m_ScratchBits);
      m_ScratchBits += static_cast<int32_t>(c_NumBitsPerWord);
      m_WordIndex += 1;
    }

    value = static_cast<uint32_t>(m_Scratch &
                                  static_cast<uint64_t>(c_LUTMask[bits]));
    m_Scratch = m_Scratch >> bits;
    m_ScratchBits -= static_cast<int32_t>(bits);
    m_TotalBits -= static_cast<int32_t>(bits);

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
    m_Buffer[m_WordIndex] = static_cast<uint32_t>(m_Scratch);
    m_ScratchBits = 0;
  }

  bool WriteBits(const uint32_t Value, const uint32_t Bits) {

    if (m_BufferSize <= 0) {
      return false;
    }

    if (Bits > c_NumBitsPerWord) {
      return false;
    }

    const uint64_t writeMask = (c_LUTMask[Bits]) << m_ScratchBits;
    m_Scratch =
        m_Scratch | (static_cast<uint64_t>(Value) << m_ScratchBits & writeMask);
    m_ScratchBits += Bits;
    if (m_ScratchBits > c_NumBitsPerWord) {
      m_Buffer[m_WordIndex] = static_cast<uint32_t>(m_Scratch);
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
  WriterStream(uint8_t *Buffer, uint32_t Bytes)
      : m_Writer(reinterpret_cast<uint32_t *>(Buffer), Bytes) {}

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

    const uint32_t Bits = getNumberOfBitsForRange(Min, Max);
    uint32_t UnsignedValue = static_cast<uint32_t>(Value - Min);
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

  ReaderStream(uint8_t *Buffer, uint32_t Bytes)
      : m_Reader(reinterpret_cast<uint32_t *>(Buffer), Bytes) {}

  bool serializeInteger(int32_t &Value, int32_t Min, int32_t Max) {
    if (Max < Min) {
      return false;
    }
    const uint32_t Bits = getNumberOfBitsForRange(Min, Max);
    if (m_Reader.WouldReadPastEnd(static_cast<int32_t>(Bits))) {
      return false;
    }
    uint32_t UnsignedValue;
    bool Result = m_Reader.ReadBits(UnsignedValue, Bits);
    if (!Result) {
      return false;
    }
    Value = static_cast<int32_t>(UnsignedValue) + Min;
    return true;
  }

private:
  BitReader m_Reader;
};

template <typename StreamType>
bool SerializeFloatInternal(StreamType &Stream, float &Value) {
  union FloatInt {
    float FloatValue;
    uint32_t IntValue;
  };
  FloatInt tmp;
  if (StreamType::IsWriting) {
    tmp.FloatValue = Value;
  }
  bool Result = Stream.SerializeBits(tmp.IntValue, 32);
  if (StreamType::IsReading) {
    Value = tmp.FloatValue;
  }
  return Result;
}

template <typename StreamType>
bool SerializeCompressedFloatInternal(StreamType &Stream, float &Value,
                                      float Min, float Max, float Res) {
  const float Delta = Max - Min;
  const float Values = Delta / Res;
  const uint32_t MaxIntegerValue = (uint32_t)ceil(Values);
  const int Bits = getNumberOfBitsForRange(0, MaxIntegerValue);
  uint32_t IntegerValue = 0;
  if (StreamType::IsWriting) {
    float NormalizedValue = std::clamp((Value - Min) / Delta, 0.0f, 1.0f);
    IntegerValue = (uint32_t)floor(NormalizedValue * MaxIntegerValue + 0.5f);
  }
  if (!Stream.SerializeBits(IntegerValue, Bits)) {
    return false;
  }
  if (StreamType::IsReading) {
    const float NormalizedValue = IntegerValue / float(MaxIntegerValue);
    Value = NormalizedValue * Delta + Min;
  }
  return true;
}

} // namespace BitPacker

#define BitPackInt(Stream, Value, Min, Max)                                    \
  do {                                                                         \
    assert(min < max);                                                         \
    int32_t Int32Value;                                                        \
    if (Stream::IsWriting) {                                                   \
      assert(Value >= Min);                                                    \
      assert(Value <= Max);                                                    \
      Int32Value = static_cast<int32_t>(Value);                                \
    }                                                                          \
    if (!stream.serializeInteger(Int32Value, Min, Max)) {                      \
      return false;                                                            \
    }                                                                          \
    if (Stream::IsReading) {                                                   \
      Value = Int32Value;                                                      \
      if (Value < Min || Value > Max) {                                        \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
  } while (0)

#define BitPackFloat(Stream, Value)                                            \
  do {                                                                         \
    if (!SerializeFloatInternal(Stream, Value)) {                              \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define BitPackCompressedFloat(stream, Value, min, max)                        \
  do {                                                                         \
    if (!SerializeFloatInternal(stream, Value, min, max)) {                    \
      return false;                                                            \
    }                                                                          \
  } while (0)
#endif // BIT_PACKER_H__
