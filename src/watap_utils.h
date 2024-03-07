#ifndef __watap_utils_h_
#define __watap_utils_h_

#include "watap_def.h"

/* Project namespace // WASM Runtime namespace */
namespace watap
{
  /* Binary data walker */
  class binary_input_stream
  {
    const UINT8 *const Begin; // Data begin pointer
    const UINT8 *Ptr;         // Pointer
    SIZE_T DataSize;          // Data size
  public:

    /* Stream constructor.
     * ARGUMENTS:
     *   - data span:
     *       std::span<const BYTE> Data;
     */
    constexpr binary_input_stream( std::span<const UINT8> Data ) noexcept : Begin(Data.data()), Ptr(Data.data()), DataSize(Data.size_bytes())
    {

    } /* End of 'binary_stream' function */

    /* Value getting function.
     * ARGUMENTS:
     *   - count of values of type to get:
     *       SIZE_T Count = 1;
     * RETURNS:
     *   (const type *) Data pointer;
     */
    template <typename type> requires std::is_trivially_copyable_v<type>
      constexpr const type * Get( SIZE_T Count = 1 ) noexcept
      {
        auto Size = sizeof(type) * Count;

        if (Ptr - Begin + Size > DataSize)
        {
          Ptr = Begin + DataSize;
          return nullptr;
        }
        return reinterpret_cast<const type *>((Ptr += Size) - Size);
      } /* End of 'Get' function */

    /* Some data skipping function.
     * ARGUMENTS:
     *   - count of values to skip:
     *       SIZE_T Count = 1;
     * RETURNS: None.
     */
    template <typename type = UINT8>
      constexpr VOID Skip( SIZE_T Count = 1 ) noexcept
      {
        Ptr += sizeof(type) * Count;
      } /* End of 'Skip' function */

    constexpr const UINT8 * CurrentPtr( VOID ) const noexcept
    {
      return Ptr;
    } /* End of 'CurrentPtr' function */

    /* Valid checking function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (BOOL) TRUE if valid, FALSE otherwise;
     */
    constexpr operator BOOL( VOID ) const noexcept
    {
      return Ptr < Begin + DataSize;
    } /* End of 'BOOL' operator */
  }; /* End of 'binary_input_stream' class*/

  /* Binary output stream representation structure */
  class binary_output_stream
  {
    std::vector<BYTE> Dst; // Destination
  public:
    /* Writing function.
     * ARGUMENTS: None.
     * RETURNS: None.
     */
    template <typename type> requires std::is_trivially_copyable_v<type>
      constexpr VOID Write( const type *Data, SIZE_T Size )
      {
        Dst.insert(Dst.end(), reinterpret_cast<const BYTE *>(Data), reinterpret_cast<const BYTE *>(Data + Size));
      } /* End of 'Write' function */

    /* Data flushing function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (std::vector<BYTE>) Flushed data.
     */
    constexpr std::vector<BYTE> Flush( VOID ) noexcept
    {
      return std::move(Dst);
    } /* End of 'Flush' function */
  }; /* End of 'binary_output_stream' class */

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
     *       const UINT8 *Stream;
     * RETURNS:
     *   (std::pair<SIZE_T, UINT8>) Pair of scanned value and size of stream skip.
     */
    [[nodiscard]] constexpr std::pair<SIZE_T, UINT8> DecodeUnsigned( const UINT8 *Stream ) noexcept
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

    /* Signed LEB128 decoding function.
     * TEMPLATE ARGUMENTS:
     *   - size of signed number in bits:
     *       UINT32 BIT_SIZE;
     * ARGUMENTS:
     *   - byte stream to scan bytes from:
     *       const UINT8 *Stream;
     * RETURNS:
     *   (std::pair<SIZE_T, UINT8>) Pair of scanned value and size of stream skip.
     */
    template <UINT32 BIT_SIZE>
      [[nodiscard]] constexpr std::pair<SSIZE_T, UINT8> DecodeSigned( const UINT8 *Stream ) noexcept
      {
        SSIZE_T Result = 0;
        UINT8 Shift = 0;
        UINT8 Byte;

        do
        {
          Byte = *Stream++;
          Result |= (Byte & 0x7F) << Shift;
          Shift += 7;
        } while (Byte & 0x80);

        if ((Shift < BIT_SIZE) && (Byte & 0x40))
          Result |= (~0ull) << Shift;
        return {Result, Shift / 7};
      } /* End of 'DecodeSigned' function */
  } /* end of 'leb128' namespace */
} /* end of 'watap' namespace */

#endif // !defined(__watap_utils_h_)

/* END OF 'watap_utils.h' FILE */
