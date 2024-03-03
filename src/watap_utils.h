#ifndef __watap_utils_h_
#define __watap_utils_h_

#include "watap_def.h"

/* Project namespace // WASM Runtime namespace */
namespace watap
{
  /* Binary data walker */
  class binary_stream
  {
    const BYTE *const Begin;
    const BYTE *Ptr;
    SIZE_T DataSize;
  public:
    binary_stream( std::span<const BYTE> Data ) : Begin(Data.data()), Ptr(Data.data()), DataSize(Data.size_bytes())
    {

    }

    template <typename type>
      constexpr const type * Get( SIZE_T Count = 1 ) noexcept
      {
        auto Size = sizeof(type) * Count;

        if (Ptr - Begin + Size > DataSize)
        {
          Ptr = Begin + DataSize;
          return nullptr;
        }
        return reinterpret_cast<const type *>((Ptr += Size) - Size);
      }

    template <typename type = UINT8>
      constexpr VOID Skip( SIZE_T Count = 1 ) noexcept
      {
        Ptr += sizeof(type) * Count;
      }

    template <typename type>
      constexpr const type * operator()( VOID ) noexcept
      {
        if (Ptr - Begin + sizeof(type) >= DataSize)
          return (Ptr = Begin + DataSize), nullptr;
        return reinterpret_cast<const type *>((Ptr += sizeof(type)) - sizeof(type));
      }

    constexpr const BYTE * CurrentPtr( VOID ) const noexcept
    {
      return Ptr;
    }

    constexpr operator BOOL( VOID ) const noexcept
    {
      return Ptr < Begin + DataSize;
    }
  }; /* End of 'co_binary_walker' structure */

  /* LEB128 worker utility namespace */
  namespace leb128
  {
    /* Unsigned LEB128 encoding function.
     * ARGUMENTS:
     *   - value to encode:
     *       SIZE_T Value;
     * RETURNS:
     *   (std::pair<SIZE_T, UINT8>) Pair of resulting value and it's size in bytes.
     */
    constexpr inline std::pair<SIZE_T, UINT8> EncodeUnsigned( SIZE_T Value ) noexcept
    {
      SIZE_T Result = 0;
      UINT8 Count = 0;

      do
      {
        UINT8 Byte = Value & 0x7F;
        Value >>= 7;
        if (Value)
          Byte |= 0x80;
        Result = (Result << 8) | Byte;
        Count++;
      } while (Value);

      return {Result, Count};
    } /* End of 'EncodeUnsigned' function */

    /* Unsigned LEB128 decoding function.
     * ARGUMENTS:
     *   - byte stream to scan bytes from:
     *       const BYTE *Stream;
     * RETURNS:
     *   (std::pair<SIZE_T, UINT8>) Pair of scanned value and size of stream skip.
     */
    constexpr inline std::pair<SIZE_T, UINT8> DecodeUnsigned( const BYTE *Stream ) noexcept
    {
      SIZE_T Result = 0;
      UINT8 Shift = 0;
      UINT8 Byte;

      do
      {
        Byte = *Stream++;
        Result |= static_cast<SIZE_T>(Byte & 0x7F) << Shift;
        Shift += 7;
      } while (Byte & 0x80);

      return {Result, Shift / 7};
    } /* End of 'DecodeUnsigned' function */
  } /* end of 'leb128' namespace */
} /* end of 'watap' namespace */

#endif // !defined(__watap_utils_h_)
