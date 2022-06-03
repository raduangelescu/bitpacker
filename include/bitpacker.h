#ifndef BIT_PACKER_H__
#define BIT_PACKER_H__

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

#define BitPackInt(Stream, Value, Min, Max)                                    \
  do {                                                                         \
    if (!SerializeIntInternal(Stream, Value)) {                                \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define BitPackFloat(Stream, Value)                                            \
  do {                                                                         \
    if (!SerializeFloatInternal(Stream, Value)) {                              \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define BitPackCompressedFloat(Stream, Value, Min, Max, Res)                   \
  do {                                                                         \
    if (!SerializeCompressedFloatInternal(Stream, Value, Min, Max, Res)) {     \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define BitPackAlign(Stream)                                                   \
  do {                                                                         \
    if (!Stream.SerializeAlign())                                              \
      return false;                                                            \
  } while (0)

#define BitPackBytes(Stream, Bytes, Length)                                    \
  do {                                                                         \
    if (!SerializeBytesInternal(Stream, Bytes, Length)) {                      \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define BitPackString(Stream, String)                                          \
  do {                                                                         \
    if (!SerializeStringInternal(Stream, String)) {                            \
      return false;                                                            \
    }                                                                          \
  } while (0)

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

  bool ReadAlign() {
    const int32_t RemainderBits = m_ScratchBits % 8;
    if (RemainderBits != 0) {

      uint32_t Value = 0;
      if (RemainderBits > static_cast<int32_t>(sizeof(uint8_t))) {
        return false;
      }
      ReadBits(Value,
               static_cast<uint32_t>(sizeof(uint8_t) -
                                     static_cast<uint32_t>(RemainderBits)));
      assert(m_ScratchBits % (sizeof(uint8_t)) == 0);
      if (Value != 0) {
        return false;
      }
    }
    return true;
  }

  bool ReadBytes(uint32_t &Size, uint8_t *Buffer) {
    // padding
    ReadAlign();
    const uint32_t ByteOffsetIndex = static_cast<uint32_t>(m_ScratchBits) /
                                     static_cast<uint32_t>(sizeof(uint8_t));
    uint8_t *BufferBytes = reinterpret_cast<uint8_t *>(m_Buffer);
    BufferBytes += ByteOffsetIndex + m_WordIndex * sizeof(uint32_t);
    memcpy(Buffer, BufferBytes, Size);
    m_WordIndex += Size / static_cast<uint32_t>(sizeof(uint64_t));
    return true;
  }

  bool ReadBits(uint32_t &value, const uint32_t bits) {
    if (m_TotalBits <= 0) {
      return false;
    }

    if (bits > c_NumBitsPerWord) {
      return false;
    }

    if (m_ScratchBits - static_cast<int32_t>(bits) < 0) {
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

  void WriteAlign() {

    const int32_t RemainderBits = m_ScratchBits % 8;
    if (RemainderBits != 0) {
      uint32_t Zero = 0;
      if (static_cast<int32_t>(sizeof(uint8_t)) < RemainderBits) {
        return;
      }
      WriteBits(Zero,
                static_cast<uint32_t>(sizeof(uint8_t) -
                                      static_cast<uint32_t>(RemainderBits)));
      assert((m_ScratchBits % sizeof(uint8_t)) == 0);
    }
  }

  bool WriteBytes(const uint32_t Size, const uint8_t *Buffer) {
    // padding
    WriteAlign();
    const uint32_t ByteOffsetIndex = m_ScratchBits / sizeof(uint8_t);
    uint8_t *BufferBytes = reinterpret_cast<uint8_t *>(m_Buffer);
    BufferBytes += ByteOffsetIndex + m_WordIndex * sizeof(uint32_t);
    memcpy(BufferBytes, Buffer, Size);
    m_WordIndex += Size / static_cast<uint32_t>(sizeof(uint64_t));
    return true;
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
    if (m_ScratchBits >= c_NumBitsPerWord) {
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

  bool SerializeBits(uint32_t &Value, const uint32_t Bits) {
    return m_Writer.WriteBits(Value, Bits);
  }

  bool SerializeBytes(const uint32_t Size, const uint8_t *Buffer) {
    return m_Writer.WriteBytes(Size, Buffer);
  }

  void Flush() { m_Writer.Flush(); }

private:
  BitWriter m_Writer;
};

class ReaderStream {
public:
  enum { IsWriting = 0 };
  enum { IsReading = 1 };

  ReaderStream(uint8_t *Buffer, uint32_t Bytes)
      : m_Reader(reinterpret_cast<uint32_t *>(Buffer), Bytes) {}

  bool SerializeBits(uint32_t &Value, const uint32_t Bits) {
    return m_Reader.ReadBits(Value, Bits);
  }

  bool SerializeBytes(uint32_t &Size, uint8_t *Buffer) {
    return m_Reader.ReadBytes(Size, Buffer);
  }

  void Flush() {}

private:
  BitReader m_Reader;
};

template <typename StreamType>
bool SerializeIntInternal(StreamType &Stream, uint32_t &Value) {
  uint32_t Tmp = 0;
  if constexpr (StreamType::IsWriting) {
    Tmp = Value;
  }
  bool Result = Stream.SerializeBits(Tmp, 32);
  if constexpr (StreamType::IsReading) {
    Value = Tmp;
  }
  return Result;
}

template <typename StreamType>
bool SerializeFloatInternal(StreamType &Stream, float &Value) {
  union FloatInt {
    float FloatValue;
    uint32_t IntValue;
  };
  FloatInt tmp = {0};
  if constexpr (StreamType::IsWriting) {
    tmp.FloatValue = Value;
  }
  bool Result = Stream.SerializeBits(tmp.IntValue, 32);
  if constexpr (StreamType::IsReading) {
    Value = tmp.FloatValue;
  }
  return Result;
}

template <typename StreamType>
bool SerializeCompressedFloatInternal(StreamType &Stream, float &Value,
                                      float Min, float Max, float Res) {
  const float Delta = Max - Min;
  const float Values = Delta / Res;
  const uint32_t MaxIntegerValue = static_cast<uint32_t>(ceil(Values));
  const uint32_t Bits =
      getNumberOfBitsForRange(0, static_cast<int32_t>(MaxIntegerValue));
  uint32_t IntegerValue = 0;
  if constexpr (StreamType::IsWriting) {
    float NormalizedValue = std::clamp((Value - Min) / Delta, 0.0f, 1.0f);
    IntegerValue = static_cast<uint32_t>(
        floor(NormalizedValue * static_cast<float>(MaxIntegerValue) + 0.5f));
  }
  if (!Stream.SerializeBits(IntegerValue, Bits)) {
    return false;
  }
  if constexpr (StreamType::IsReading) {
    const float NormalizedValue =
        static_cast<float>(IntegerValue) / static_cast<float>(MaxIntegerValue);
    Value = NormalizedValue * Delta + Min;
  }
  return true;
}
template <typename StreamType>
bool SerializeBytesInternal(StreamType &Stream, uint8_t *Buffer,
                            uint32_t Length) {
  if (!Stream.SerializeBytes(Length, Buffer)) {
    return false;
  }
  return true;
}

template <typename StreamType>
bool SerializeStringInternal(StreamType &Stream, char *String) {
  uint32_t Length;
  if constexpr (StreamType::IsWriting) {
    Length = static_cast<uint32_t>(strlen(String));
  }
  BitPackInt(Stream, Length, 0, UINT32_MAX);
  BitPackBytes(Stream, reinterpret_cast<uint8_t *>(String), Length);
  if constexpr (StreamType::IsReading) {
    String[Length] = '\0';
  }
  return true;
}

} // namespace BitPacker

#endif // BIT_PACKER_H__
