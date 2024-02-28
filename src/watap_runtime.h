#ifndef __watap_storage_h_
#define __watap_storage_h_

#include "watap_bin.h"

/* Project namespace // WASM Runtime implementation namespace */
namespace watap
{
  /* Section data representation structure */
  struct section_data
  {
    section_id Id;           // Section type id
    std::vector<UINT8> Data; // Section contents
  }; /* End of 'section_data' structure */

  /* WASM Module representation structure */
  class module
  {
  public:
  }; /* End of 'module' structure */

  class co_binary_walker
  {
    const BYTE *const Begin;
    const BYTE *Ptr;
    SIZE_T DataSize;
  public:
    co_binary_walker( std::span<const BYTE> Data ) : Begin(Data.data()), Ptr(Data.data()), DataSize(Data.size_bytes())
    {

    }

    template <typename type>
      constexpr const type * Get( SIZE_T Count = 1 ) noexcept
      {
        auto Size = sizeof(type) * Count;

        if (Ptr - Begin + Size >= DataSize)
          return (Ptr = Begin + DataSize), nullptr;
        return reinterpret_cast<const type *>((Ptr += Size) - Size);
      }

    template <typename type>
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

  /* Section parsing function.
   * ARGUMENTS:
   *   - WASM module binary:
   *       std::span<const BYTE> WasmBinary;
   * RETURNS: None.
   */
  inline VOID ParseSections( std::span<const BYTE> WasmBinary )
  {
    auto BinaryWalker = co_binary_walker(WasmBinary);

    const module_header *ModuleHeader = BinaryWalker.Get<module_header>();

    if (!ModuleHeader || !ModuleHeader->Validate())
      return;

    std::cout
              <<             "WASM Binary module."
      << '\n' << std::format("Version: {:X}", ModuleHeader->Version)
      << '\n' <<             "Sections:"
      << '\n';

    while (TRUE)
    {
      section_id SectionType;
      if (auto Ptr = BinaryWalker.Get<section_id>())
        SectionType = *Ptr;
      else
        break;

      auto [Size, Offset] = leb128::DecodeUnsigned(BinaryWalker.CurrentPtr());
      BinaryWalker.Skip<UINT8>(Offset);

      const CHAR *SectionName;
      switch (SectionType)
      {
      case section_id::eCustom    : SectionName = "Custom   "; break;
      case section_id::eType      : SectionName = "Type     "; break;
      case section_id::eImport    : SectionName = "Import   "; break;
      case section_id::eFunction  : SectionName = "Function "; break;
      case section_id::eTable     : SectionName = "Table    "; break;
      case section_id::eMemory    : SectionName = "Memory   "; break;
      case section_id::eGlobal    : SectionName = "Global   "; break;
      case section_id::eExport    : SectionName = "Export   "; break;
      case section_id::eStart     : SectionName = "Start    "; break;
      case section_id::eElement   : SectionName = "Element  "; break;
      case section_id::eCode      : SectionName = "Code     "; break;
      case section_id::eData      : SectionName = "Data     "; break;
      case section_id::eDataCount : SectionName = "DataCount"; break;
      default                     : SectionName = "<unknown>"; break;
      }

      std::cout << std::format("  {} {}b\n", SectionName, Size);
      BinaryWalker.Skip<UINT8>(Size);
    }
  } /* End of 'ParseSections' function */

  /* virtual machine representation structure */
  class virtual_machine
  {
  public:
  }; /* End of 'virtual_machine' class */
} /* end of 'watap' namespace */

#endif // !defined(__watap_storage_h_)
