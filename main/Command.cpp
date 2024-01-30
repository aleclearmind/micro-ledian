#include "Command.h"

namespace Command {

struct cursor_t {
  uint32_t Column;
  uint32_t Line;

  void operator++() { ++Column; }

  cursor_t operator+(size_t Columns) const {
    cursor_t Result = *this;
    Result.Column += Columns;
    assert(Result.verify());
    return Result;
  }

  bool verify() const {
    // TODO: fix
    return Column < 80 and Line < 22;
  }
};

enum class BufferType { FixedSize, Array };

class Context {
public:
  bool SaidHello = false;
  cursor_t WriteCursor;

  Context() : WriteCursor({0, 0}) {}
};

class Helo {
public:
  static constexpr const char *Name = "Helo";
  static constexpr char ID = 1;
  static constexpr BufferType Type = BufferType::FixedSize;
  using FixedType = std::array<uint8_t, 4>;

private:
  Context &C;

public:
  Helo(Context &C) : C(C) {}

  void parse(const FixedType *Object) {
    static FixedType Reference = {'H', 'E', 'L', 'O'};
    assert(0 == memcmp(Object, &Reference, sizeof(FixedType)));
    C.SaidHello = true;
  }
};

class UpdateRange {
public:
  static constexpr const char *Name = "UpdateRange";
  static constexpr char ID = 2;
  static constexpr BufferType Type = BufferType::Array;
  using ArrayType = LEDDescriptor;

private:
  Context &C;
  cursor_t LocalWriteCursor;

public:
  UpdateRange(Context &C) : C(C), LocalWriteCursor(C.WriteCursor) {
    assert(LocalWriteCursor.verify());
  }

public:
  void preparse(size_t Elements) {
    assert(C.SaidHello);
    assert((LocalWriteCursor + (Elements - 1)).verify());
  }

  void parseOne(const LEDDescriptor *Object) {
    assert(Object->verify());
    LEDs.set(LocalWriteCursor.Column, LocalWriteCursor.Line,
             Object->Color.toRGBColor(), Object->Blink);
    ++LocalWriteCursor;
  }
};

class MoveCursor {
public:
  static constexpr const char *Name = "MoveCursor";
  static constexpr char ID = 3;
  static constexpr BufferType Type = BufferType::FixedSize;
  using FixedType = cursor_t;

private:
  Context &C;

public:
  MoveCursor(Context &C) : C(C) {}

  void parse(const cursor_t *Object) {
    assert(C.SaidHello);
    log("MoveCursor(Column: %ld, Line: %ld)\n", Object->Column, Object->Line);
    assert(Object->verify());
    C.WriteCursor = *Object;
  }
};

struct ConfigureMessage {
  uint8_t Debug = 0;

  bool verify() const { return Debug < 2; }
};

class Configure {
public:
  static constexpr const char *Name = "Configure";
  static constexpr char ID = 4;
  static constexpr BufferType Type = BufferType::FixedSize;
  using FixedType = ConfigureMessage;

private:
  Context &C;

public:
  Configure(Context &C) : C(C) {}

  void parse(const ConfigureMessage *Object) {
    assert(C.SaidHello);
    log("Configure(Debug: %ld)\n", Object->Debug);
    assert(Object->verify());

    EnableDebug = Object->Debug;
  }
};

Context C;
using identifier_t = uint8_t;
using length_t = uint32_t;

static constexpr size_t MaxReadSize = sizeof(cursor_t);

ArrayRef<uint8_t> read(size_t Size);

template <typename T> T *read() {
  return reinterpret_cast<T *>(read(sizeof(T)).Data);
}

template <typename T> void parseFixedSize(length_t Length) {
  Trace TT(event_ids::ParseFixedSize, Length);
  assert(sizeof(typename T::FixedType) == Length);
  T Instance(C);
  Instance.parse(read<typename T::FixedType>());
}

template <typename T> void parseArray(length_t Length) {
  Trace TT(event_ids::ParseArray, Length);
  constexpr size_t ElementSize = sizeof(typename T::ArrayType);
  assert(Length % ElementSize == 0);
  length_t Elements = Length / ElementSize;
  log("Got array of %ld elements\n", Elements);
  T Instance(C);
  {
    Trace TTT(event_ids::PreParse);
    Instance.preparse(Elements);
  }
  for (length_t I = 0; I < Elements; ++I) {
    Trace TTT(event_ids::ParseOne, I);
    Instance.parseOne(read<typename T::ArrayType>());
  }
}

template <typename T> void dispatch(length_t Length) {
  log("Got command %s\n", T::Name);

  if constexpr (T::Type == BufferType::FixedSize) {
    parseFixedSize<T>(Length);
  } else if constexpr (T::Type == BufferType::Array) {
    parseArray<T>(Length);
  } else {
    abort();
  }
}

ArrayRef<uint8_t> read(size_t Size) {
  Trace T(event_ids::Read, Size);
  assert(Size <= MaxReadSize);
  static uint8_t ReadBuffer[MaxReadSize];
  size_t ReadData = fread(&ReadBuffer, 1, Size, stdin);
  assert(ReadData == Size);

#if 0
  static size_t UnackedBytes = 0;
  UnackedBytes += ReadData;
  while (UnackedBytes >= 16) {
    puts("A16");
    UnackedBytes -= 16;
  }
#endif

#ifdef VERBOSE
  for (size_t I = 0; I < Size; ++I) {
    log("%.2x ", ReadBuffer[I]);
  }
  puts("");
#endif

  return ArrayRef{&ReadBuffer[0], Size};
}

bool hasData() {
  Trace T(event_ids::HasData);
  // TODO
  return true;
}

void parse() {
  Trace T(event_ids::Parse);

  if (hasData()) {
    identifier_t ID = *read<identifier_t>();
    length_t Length = *read<length_t>();
    log("ID: %d length: %ld\n", ID, Length);

    switch (ID) {
    case Helo::ID:
      dispatch<Helo>(Length);
      break;

    case UpdateRange::ID:
      dispatch<UpdateRange>(Length);
      break;

    case MoveCursor::ID:
      dispatch<MoveCursor>(Length);
      break;
    }

    puts("ACK");
  }
}

}
