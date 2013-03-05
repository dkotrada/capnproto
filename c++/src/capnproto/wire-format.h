// Copyright (c) 2013, Kenton Varda <temporal@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file is NOT intended for use by clients, except in generated code.
//
// This file defines low-level, non-type-safe classes for interpreting the raw Cap'n Proto wire
// format.  Code generated by the Cap'n Proto compiler uses these classes, as does other parts of
// the Cap'n proto library which provide a higher-level interface for dynamic introspection.

#ifndef CAPNPROTO_WIRE_FORMAT_H_
#define CAPNPROTO_WIRE_FORMAT_H_

#include "macros.h"
#include "type-safety.h"

namespace capnproto {
  class SegmentReader;
  class SegmentBuilder;
}

namespace capnproto {
namespace internal {

class FieldDescriptor;
typedef Id<uint8_t, FieldDescriptor> FieldNumber;
enum class FieldSize: uint8_t;

class StructBuilder;
class StructReader;
class ListBuilder;
class ListReader;
struct WireReference;
struct WireHelpers;

// -------------------------------------------------------------------

template <typename T>
class WireValue {
  // Wraps a primitive value as it appears on the wire.  Namely, values are little-endian on the
  // wire, because little-endian is the most common endianness in modern CPUs.
  //
  // TODO:  On big-endian systems, inject byte-swapping here.  Most big-endian CPUs implement
  //   dedicated instructions for this, so use those rather than writing a bunch of shifts and
  //   masks.  Note that GCC has e.g. __builtin__bswap32() for this.
  //
  // Note:  In general, code that depends cares about byte ordering is bad.  See:
  //     http://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html
  //   Cap'n Proto is special because it is essentially doing compiler-like things, fussing over
  //   allocation and layout of memory, in order to squeeze out every last drop of performance.

public:
  WireValue() = default;
  CAPNPROTO_ALWAYS_INLINE(WireValue(T value)): value(value) {}

  CAPNPROTO_ALWAYS_INLINE(T get() const) { return value; }
  CAPNPROTO_ALWAYS_INLINE(void set(T newValue)) { value = newValue; }

private:
  T value;
};

class StructBuilder {
public:
  inline StructBuilder(): segment(nullptr), data(nullptr), references(nullptr) {}

  static StructBuilder initRoot(SegmentBuilder* segment, word* location, const word* defaultValue);

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(T getDataField(ElementCount offset) const);
  // Gets the data field value of the given type at the given offset.  The offset is measured in
  // multiples of the field size, determined by the type.

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(void setDataField(
      ElementCount offset, typename NoInfer<T>::Type value) const);
  // Sets the data field value at the given offset.

  StructBuilder initStructField(WireReferenceCount refIndex, const word* typeDefaultValue) const;
  // Initializes the struct field at the given index in the reference segment.  If it is already
  // initialized, the previous value is discarded or overwritten.  The struct is initialized to
  // match the given default value (a trusted message).  This must be the default value for the
  // *type*, not the specific field, and in particular its reference segment is expected to be
  // all nulls (only the data segment is copied).  Use getStructField() if you want the struct
  // to be initialized as a copy of the field's default value (which may have non-null references).

  StructBuilder getStructField(WireReferenceCount refIndex, const word* defaultValue) const;
  // Gets the struct field at the given index in the reference segment.  If the field is not already
  // initialized, it is initialized as a deep copy of the given default value (a trusted message).

  ListBuilder initListField(WireReferenceCount refIndex, FieldSize elementSize,
                            ElementCount elementCount) const;
  // Allocates a new list of the given size for the field at the given index in the reference
  // segment, and return a pointer to it.  All elements are initialized to zero.

  ListBuilder initStructListField(WireReferenceCount refIndex, ElementCount elementCount,
                                  const word* elementDefaultValue) const;
  // Allocates a new list of the given size for the field at the given index in the reference
  // segment, and return a pointer to it.  Each element is initialized as a copy of
  // elementDefaultValue.  As with initStructField(), this should be the default value for the
  // *type*, with all-null references.

  ListBuilder getListField(WireReferenceCount refIndex, const word* defaultValue) const;
  // Gets the already-allocated list field for the given reference index.  If the list is not
  // already allocated, it is allocated as a deep copy of the given default value (a trusted
  // message).  If the default value is null, an empty list is used.

  StructReader asReader() const;
  // Gets a StructReader pointing at the same memory.

private:
  SegmentBuilder* segment;     // Memory segment in which the struct resides.
  word* data;                  // Pointer to the encoded data.
  WireReference* references;   // Pointer to the encoded references.

  inline StructBuilder(SegmentBuilder* segment, word* data, WireReference* references)
      : segment(segment), data(data), references(references) {}

  friend class ListBuilder;
  friend struct WireHelpers;
};

class StructReader {
public:
  inline StructReader()
      : segment(nullptr), data(nullptr), references(nullptr), fieldCount(0), dataSize(0),
        referenceCount(0), bit0Offset(0 * BITS), recursionLimit(0) {}

  static StructReader readRootTrusted(const word* location, const word* defaultValue);
  static StructReader readRoot(const word* location, const word* defaultValue,
                               SegmentReader* segment, int recursionLimit);

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(
      T getDataField(ElementCount offset, typename NoInfer<T>::Type defaultValue) const);
  // Get the data field value of the given type at the given offset.  The offset is measured in
  // multiples of the field size, determined by the type.  Returns the default value if the offset
  // is past the end of the struct's data segment.

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(T getDataFieldCheckingNumber(
      FieldNumber fieldNumber, ElementCount offset, typename NoInfer<T>::Type defaultValue) const);
  // Like getDataField() but also returns the default if the field number not less than the field
  // count.  This is needed in cases where the field was packed into a hole preceding other fields
  // with later numbers, and therefore the offset being in-bounds alone does not prove that the
  // struct contains the field.

  StructReader getStructField(WireReferenceCount refIndex, const word* defaultValue) const;
  // Get the struct field at the given index in the reference segment, or the default value if not
  // initialized.  defaultValue will be interpreted as a trusted message -- it must point at a
  // struct reference, which in turn points at the struct value.

  ListReader getListField(WireReferenceCount refIndex, FieldSize expectedElementSize,
                          const word* defaultValue) const;
  // Get the list field at the given index in the reference segment, or the default value if not
  // initialized.  The default value is allowed to be null, in which case an empty list is used.

private:
  SegmentReader* segment;  // Memory segment in which the struct resides.

  const void* data;
  const WireReference* references;

  FieldNumber fieldCount;              // Number of fields the struct is reported to have.
  WordCount8 dataSize;                 // Size of data segment.
  WireReferenceCount8 referenceCount;  // Size of the reference segment.

  BitCount8 bit0Offset;
  // A special hack:  When accessing a boolean with field number zero, pretend its offset is this
  // instead of the usual zero.  This is needed to allow a boolean list to be upgraded to a list
  // of structs.

  int recursionLimit;
  // Limits the depth of message structures to guard against stack-overflow-based DoS attacks.
  // Once this reaches zero, further pointers will be pruned.

  inline StructReader(SegmentReader* segment, const void* data, const WireReference* references,
                      FieldNumber fieldCount, WordCount dataSize, WireReferenceCount referenceCount,
                      BitCount bit0Offset, int recursionLimit)
      : segment(segment), data(data), references(references), fieldCount(fieldCount),
        dataSize(dataSize), referenceCount(referenceCount), bit0Offset(bit0Offset),
        recursionLimit(recursionLimit) {}

  friend class ListReader;
  friend class StructBuilder;
  friend struct WireHelpers;
};

// -------------------------------------------------------------------

class ListBuilder {
public:
  inline ListBuilder(): segment(nullptr), ptr(nullptr), elementCount(0) {}

  inline ElementCount size();
  // The number of elements in the list.

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(T getDataElement(ElementCount index) const);
  // Get the element of the given type at the given index.

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(void setDataElement(
      ElementCount index, typename NoInfer<T>::Type value) const);
  // Set the element at the given index.

  StructBuilder getStructElement(
      ElementCount index, decltype(WORDS/ELEMENTS) elementSize, WordCount structDataSize) const;
  // Get the struct element at the given index.  elementSize is the size, in 64-bit words, of
  // each element.

  ListBuilder initListElement(
      WireReferenceCount index, FieldSize elementSize, ElementCount elementCount) const;
  // Create a new list element of the given size at the given index.  All elements are initialized
  // to zero.

  ListBuilder initStructListElement(WireReferenceCount index, ElementCount elementCount,
                                    const word* elementDefaultValue) const;
  // Allocates a new list of the given size for the field at the given index in the reference
  // segment, and return a pointer to it.  Each element is initialized as a copy of
  // elementDefaultValue.  As with StructBuilder::initStructListElement(), this should be the
  // default value for the *type*, with all-null references.

  ListBuilder getListElement(WireReferenceCount index, FieldSize elementSize) const;
  // Get the existing list element at the given index.  Returns an empty list if the element is
  // not initialized.

  ListReader asReader(FieldSize elementSize) const;
  // Get a ListReader pointing at the same memory.  Use this version only for non-struct lists.

  ListReader asReader(FieldNumber fieldCount, WordCount dataSize,
                      WireReferenceCount referenceCount) const;
  // Get a ListReader pointing at the same memory.  Use this version only for struct lists.

private:
  SegmentBuilder* segment;  // Memory segment in which the list resides.
  word* ptr;  // Pointer to the beginning of the list.
  ElementCount elementCount;  // Number of elements in the list.

  inline ListBuilder(SegmentBuilder* segment, word* ptr, ElementCount size)
      : segment(segment), ptr(ptr), elementCount(size) {}

  friend class StructBuilder;
  friend struct WireHelpers;
};

class ListReader {
public:
  inline ListReader()
      : segment(nullptr), ptr(nullptr), elementCount(0),
        stepBits(0 * BITS / ELEMENTS), structFieldCount(0), structDataSize(0),
        structReferenceCount(0), recursionLimit(0) {}

  inline ElementCount size();
  // The number of elements in the list.

  template <typename T>
  CAPNPROTO_ALWAYS_INLINE(T getDataElement(ElementCount index) const);
  // Get the element of the given type at the given index.

  StructReader getStructElement(ElementCount index, const word* defaultValue) const;
  // Get the struct element at the given index.

  ListReader getListElement(WireReferenceCount index, FieldSize expectedElementSize,
                            const word* defaultValue) const;
  // Get the list element at the given index.

private:
  SegmentReader* segment;  // Memory segment in which the list resides.

  const void* ptr;
  // Pointer to the data.  If null, use defaultReferences.  (Never null for data lists.)
  // Must be aligned appropriately for the elements.

  ElementCount elementCount;  // Number of elements in the list.

  decltype(BITS / ELEMENTS) stepBits;
  // The distance between elements, in bits.  This is usually the element size, but can be larger
  // if the sender upgraded a data list to a struct list.  It will always be aligned properly for
  // the type.  Unsigned so that division by a constant power of 2 is efficient.

  FieldNumber structFieldCount;
  WordCount structDataSize;
  WireReferenceCount structReferenceCount;
  // If the elements are structs, the properties of the struct.  The field and reference counts are
  // only used to check for field presence; the data size is also used to compute the reference
  // pointer.

  int recursionLimit;
  // Limits the depth of message structures to guard against stack-overflow-based DoS attacks.
  // Once this reaches zero, further pointers will be pruned.

  inline ListReader(SegmentReader* segment, const void* ptr, ElementCount elementCount,
                    decltype(BITS / ELEMENTS) stepBits, int recursionLimit)
      : segment(segment), ptr(ptr), elementCount(elementCount), stepBits(stepBits),
        structFieldCount(0), structDataSize(0), structReferenceCount(0),
        recursionLimit(recursionLimit) {}
  inline ListReader(SegmentReader* segment, const void* ptr, ElementCount elementCount,
                    decltype(BITS / ELEMENTS) stepBits,
                    FieldNumber structFieldCount, WordCount structDataSize,
                    WireReferenceCount structReferenceCount, int recursionLimit)
      : segment(segment), ptr(ptr), elementCount(elementCount), stepBits(stepBits),
        structFieldCount(structFieldCount), structDataSize(structDataSize),
        structReferenceCount(structReferenceCount), recursionLimit(recursionLimit) {}

  friend class StructReader;
  friend class ListBuilder;
  friend struct WireHelpers;
};

// =======================================================================================
// Internal implementation details...

template <typename T>
inline T StructBuilder::getDataField(ElementCount offset) const {
  return reinterpret_cast<WireValue<T>*>(data)[offset / ELEMENTS].get();
}

template <>
inline bool StructBuilder::getDataField<bool>(ElementCount offset) const {
  BitCount boffset = offset * (1 * BITS / ELEMENTS);
  byte* b = reinterpret_cast<byte*>(data) + boffset / BITS_PER_BYTE;
  return (*reinterpret_cast<uint8_t*>(b) & (1 << (boffset % BITS_PER_BYTE / BITS))) != 0;
}

template <typename T>
inline void StructBuilder::setDataField(
    ElementCount offset, typename NoInfer<T>::Type value) const {
  reinterpret_cast<WireValue<T>*>(data)[offset / ELEMENTS].set(value);
}

template <>
inline void StructBuilder::setDataField<bool>(ElementCount offset, bool value) const {
  BitCount boffset = offset * (1 * BITS / ELEMENTS);
  byte* b = reinterpret_cast<byte*>(data) + boffset / BITS_PER_BYTE;
  uint bitnum = boffset % BITS_PER_BYTE / BITS;
  *reinterpret_cast<uint8_t*>(b) = (*reinterpret_cast<uint8_t*>(b) & ~(1 << bitnum))
                                 | (static_cast<uint8_t>(value) << bitnum);
}

// -------------------------------------------------------------------

template <typename T>
T StructReader::getDataField(ElementCount offset, typename NoInfer<T>::Type defaultValue) const {
  if (offset * bytesPerElement<T>() < dataSize * BYTES_PER_WORD) {
    return reinterpret_cast<const WireValue<T>*>(data)[offset / ELEMENTS].get();
  } else {
    return defaultValue;
  }
}

template <>
inline bool StructReader::getDataField<bool>(ElementCount offset, bool defaultValue) const {
  BitCount boffset = offset * (1 * BITS / ELEMENTS);

  // This branch should always be optimized away when inlining.
  if (boffset == 0 * BITS) boffset = bit0Offset;

  if (boffset < dataSize * BITS_PER_WORD) {
    const byte* b = reinterpret_cast<const byte*>(data) + boffset / BITS_PER_BYTE;
    return (*reinterpret_cast<const uint8_t*>(b) & (1 << (boffset % BITS_PER_BYTE / BITS))) != 0;
  } else {
    return defaultValue;
  }
}

template <typename T>
T StructReader::getDataFieldCheckingNumber(
    FieldNumber fieldNumber, ElementCount offset, typename NoInfer<T>::Type defaultValue) const {
  // Intentionally use & rather than && to reduce branches.
  if ((fieldNumber < fieldCount) &
      (offset * bytesPerElement<T>() < dataSize * BYTES_PER_WORD)) {
    return reinterpret_cast<const WireValue<T>*>(data)[offset / ELEMENTS].get();
  } else {
    return defaultValue;
  }
}

template <>
inline bool StructReader::getDataFieldCheckingNumber<bool>(
    FieldNumber fieldNumber, ElementCount offset, bool defaultValue) const {
  BitCount boffset = offset * (1 * BITS / ELEMENTS);

  // This branch should always be optimized away when inlining.
  if (boffset == 0 * BITS) boffset = bit0Offset;

  // Intentionally use & rather than && to reduce branches.
  if ((fieldNumber < fieldCount) & (boffset < dataSize * BITS_PER_WORD)) {
    const byte* b = reinterpret_cast<const byte*>(data) + boffset / BITS_PER_BYTE;
    return (*reinterpret_cast<const uint8_t*>(b) & (1 << (boffset % BITS_PER_BYTE / BITS))) != 0;
  } else {
    return defaultValue;
  }
}

// -------------------------------------------------------------------

inline ElementCount ListBuilder::size() { return elementCount; }

template <typename T>
inline T ListBuilder::getDataElement(ElementCount index) const {
  return reinterpret_cast<WireValue<T>*>(ptr)[index / ELEMENTS].get();
}

template <>
inline bool ListBuilder::getDataElement<bool>(ElementCount index) const {
  BitCount bindex = index * (1 * BITS / ELEMENTS);
  byte* b = reinterpret_cast<byte*>(ptr) + bindex / BITS_PER_BYTE;
  return (*reinterpret_cast<uint8_t*>(b) & (1 << (bindex % BITS_PER_BYTE / BITS))) != 0;
}

template <typename T>
inline void ListBuilder::setDataElement(ElementCount index, typename NoInfer<T>::Type value) const {
  reinterpret_cast<WireValue<T>*>(ptr)[index / ELEMENTS].set(value);
}

template <>
inline void ListBuilder::setDataElement<bool>(ElementCount index, bool value) const {
  BitCount bindex = index * (1 * BITS / ELEMENTS);
  byte* b = reinterpret_cast<byte*>(ptr) + bindex / BITS_PER_BYTE;
  uint bitnum = bindex % BITS_PER_BYTE / BITS;
  *reinterpret_cast<uint8_t*>(b) = (*reinterpret_cast<uint8_t*>(b) & ~(1 << bitnum))
                                 | (static_cast<uint8_t>(value) << bitnum);
}

// -------------------------------------------------------------------

inline ElementCount ListReader::size() { return elementCount; }

template <typename T>
inline T ListReader::getDataElement(ElementCount index) const {
  return *reinterpret_cast<const T*>(
      reinterpret_cast<const byte*>(ptr) + index * stepBits / BITS_PER_BYTE);
}

template <>
inline bool ListReader::getDataElement<bool>(ElementCount index) const {
  BitCount bindex = index * stepBits;
  const byte* b = reinterpret_cast<const byte*>(ptr) + bindex / BITS_PER_BYTE;
  return (*reinterpret_cast<const uint8_t*>(b) & (1 << (bindex % BITS_PER_BYTE / BITS))) != 0;
}

}  // namespace internal
}  // namespace capnproto

#endif  // CAPNPROTO_WIRE_FORMAT_H_
