#define WATAP_IMPL_STANDARD
#include "watap_impl_standard_interface.h"

namespace watap::impl::standard
{
  std::optional<UINT32> ParseUint( binary_stream &Stream )
  {
    auto [Value, Offset] = leb128::DecodeUnsigned(Stream.CurrentPtr());
    if (Stream.Get<UINT8>(Offset))
      return static_cast<UINT32>(Value);
    return std::nullopt;
  }

  /* Function type parsing function.
   * ARGUMENTS:
   *   - stream to parse data from:
   *       binary_stream &Stream;
   * RETURNS:
   *   (std::optional<function_type>) Parsed function type, if parsed.
   */
  std::optional<function_signature> ParseFunctionType( binary_stream &Stream )
  {
    if (auto TPtr = Stream.Get<UINT8>(); !TPtr || *TPtr != bin::FUNCTION_BEGIN)
      return std::nullopt;
    
    function_signature Result;

    UINT32 ArgumentCount = 0;
    WATAP_SET_OR_RETURN(ArgumentCount, ParseUint(Stream), std::nullopt);

    Result.ArgumentTypes.reserve(ArgumentCount);

    while (ArgumentCount--)
      WATAP_CALL_OR_RETURN(Result.ArgumentTypes.push_back, Stream.Get<bin::value_type>(), std::nullopt);

    SIZE_T ReturnTypeCount = 0;
    WATAP_SET_OR_RETURN(ReturnTypeCount, ParseUint(Stream), std::nullopt);

    if (ReturnTypeCount-- > 1)
      WATAP_SET_OR_RETURN(Result.ReturnType, Stream.Get<bin::value_type>(), std::nullopt);

    while (ReturnTypeCount--)
      Stream.Get<bin::value_type>();
    return std::move(Result);
  } /* End of 'binary_stream' structure */

  template <typename type>
    std::optional<std::span<const type>> ParseVec( binary_stream &Stream )
    {
      SIZE_T Count;
      WATAP_SET_OR_RETURN(Count, ParseUint(Stream), std::nullopt);

      auto Begin = reinterpret_cast<const type *>(Stream.CurrentPtr());
      Stream.Skip(Count * sizeof(type));
      if (!Stream)
        return std::nullopt;

      return std::span<const type>(Begin, Count);
    } /* End of 'ParseString' function */

  std::optional<std::string> ParseString( binary_stream &Stream )
  {
    if (auto CharVec = ParseVec<CHAR>(Stream))
      return std::string {CharVec->begin(), CharVec->end()};
    return std::nullopt;
  } /* End of 'ParseString' function */

  inline std::optional<bin::limits> ParseLimits( binary_stream &Stream )
  {
    bin::limit_type LimitType;
    bin::limits Result;
    WATAP_SET_OR_RETURN(LimitType, Stream.Get<bin::limit_type>(), std::nullopt);

    switch (LimitType)
    {
    case bin::limit_type::eMinMax:
      WATAP_SET_OR_RETURN(Result.Max, ParseUint(Stream), std::nullopt);
      [[fallthrough]];

    case bin::limit_type::eMin:
      WATAP_SET_OR_RETURN(Result.Min, ParseUint(Stream), std::nullopt);
      break;

    }

    return Result;
  }

  std::optional<import_info> ParseImport( binary_stream &Stream )
  {
    import_info Result;

    WATAP_SET_OR_RETURN(Result.ModuleName, ParseString(Stream), std::nullopt);
    WATAP_SET_OR_RETURN(Result.ImportName, ParseString(Stream), std::nullopt);
    WATAP_SET_OR_RETURN(Result.ImportType, Stream.Get<bin::import_export_type>(), std::nullopt);

    switch(Result.ImportType)
    {
    case bin::import_export_type::eFunction:
    {
      WATAP_SET_OR_RETURN(Result.Function, ParseUint(Stream), std::nullopt);
      break;
    }

    case bin::import_export_type::eTable:
    {
      WATAP_SET_OR_RETURN(Result.Table.ValueReferenceType, Stream.Get<bin::reference_type>(), std::nullopt);
      WATAP_SET_OR_RETURN(Result.Table.Limits, ParseLimits(Stream), std::nullopt);
      break;
    }

    case bin::import_export_type::eMemory:
    {
      WATAP_SET_OR_RETURN(Result.Memory, ParseLimits(Stream), std::nullopt);
      break;
    }

    case bin::import_export_type::eGlobal:
    {
      WATAP_SET_OR_RETURN(Result.Variable, Stream.Get<bin::global_type>(), std::nullopt);
      break;
    }

    default:
      return std::nullopt;
    }

    return Result;
  } /* End of 'ParseImport' function */

  /* Module data by sections splitting function.
   * ARGUMENTS:
   *   - module data:
   *       std::span<const BYTE> Data;
   * RETURNS:
   *   (std::optional<std::map<bin::section_id, std::span<const BYTE>>>) Module section datas;
   */
  std::optional<std::map<bin::section_id, std::span<const BYTE>>> ParseSections( std::span<const BYTE> Data )
  {
    binary_stream Stream {Data};

    // Read and validate module header
    {
      bin::module_header ModuleHeader {};
      WATAP_SET_OR_RETURN(ModuleHeader, Stream.Get<bin::module_header>(), std::nullopt);

      if (!ModuleHeader.Validate())
        return std::nullopt;
    }

    std::map<bin::section_id, std::span<const BYTE>> Sections;

    while (TRUE)
    {
      bin::section_id SectionId = bin::section_id::eFunction;
      WATAP_SET_OR_BREAK(SectionId, Stream.Get<bin::section_id>());

      SIZE_T SectionSize;
      WATAP_SET_OR_RETURN(SectionSize, ParseUint(Stream), std::nullopt);


      switch (SectionId)
      {
      case bin::section_id::eCustom   :
        break;

      case bin::section_id::eType     :
      case bin::section_id::eImport   :
      case bin::section_id::eFunction :
      case bin::section_id::eTable    :
      case bin::section_id::eMemory   :
      case bin::section_id::eGlobal   :
      case bin::section_id::eExport   :
      case bin::section_id::eStart    :
      case bin::section_id::eElement  :
      case bin::section_id::eCode     :
      case bin::section_id::eData     :
      case bin::section_id::eDataCount:
        Sections.emplace(SectionId, std::span<const BYTE>(Stream.CurrentPtr(), SectionSize));
        break;

      default: // Invalid section error
        return std::nullopt;
      }
      Stream.Skip(SectionSize);
    }

    return std::move(Sections);
  } /* End of 'ParseSections' function */


  /* Module source create function.
   * ARGUMENTS:
   *   - module source descriptor:
   *       const module_source_info &Info;
   * RETURNS:
   *   (module_source *) Created module source pointer;
   */ 
  module_source * interface_impl::CreateModuleSource( const module_source_info &Info )
  {
    if (std::holds_alternative<std::string_view>(Info))
      return nullptr;
    std::span<const BYTE> Data = std::get<std::span<const BYTE>>(Info);

    std::unique_ptr<module_source_impl> Result {new module_source_impl()};

    // Parse sections from code
    std::map<bin::section_id, std::span<const BYTE>> Sections;
    WATAP_SET_OR_RETURN(Sections, ParseSections(Data), nullptr);
  
    std::vector<function_signature> FunctionSignatures;
  
    if (auto FuncSignatureSectionIter = Sections.find(bin::section_id::eType); FuncSignatureSectionIter != Sections.end())
    {
      binary_stream Stream {FuncSignatureSectionIter->second};

      UINT32 FunctionSignatureCount = 0;
      WATAP_SET_OR_RETURN(FunctionSignatureCount, ParseUint(Stream), nullptr);
  
      FunctionSignatures.reserve(FunctionSignatureCount);
      while (FunctionSignatureCount--)
        WATAP_CALL_OR_RETURN(FunctionSignatures.push_back, ParseFunctionType(Stream), nullptr);
    }

    std::vector<UINT32> DefinedFunctionSignatures;

    if (auto SectionIter = Sections.find(bin::section_id::eFunction); SectionIter != Sections.end())
    {
      binary_stream Stream {SectionIter->second};

      UINT32 FunctionCount = 0;
      WATAP_SET_OR_RETURN(FunctionCount, ParseUint(Stream), nullptr);

      while (FunctionCount--)
        WATAP_CALL_OR_RETURN(DefinedFunctionSignatures.push_back, ParseUint(Stream), nullptr);
    }

    /* Functions */
    if (auto SectionIter = Sections.find(bin::section_id::eCode); SectionIter != Sections.end())
    {
      binary_stream Stream {SectionIter->second};

      UINT32 FunctionCount = 0;
      WATAP_SET_OR_RETURN(FunctionCount, ParseUint(Stream), nullptr);

      for (UINT32 i = 0; i < FunctionCount; i++)
      {
        const function_signature &Signature = FunctionSignatures[DefinedFunctionSignatures[i]];
        function Function;

        UINT32 CodeSize = 0;
        WATAP_SET_OR_RETURN(CodeSize, ParseUint(Stream), nullptr);

        Function.ArgumentCount = Signature.ArgumentTypes.size();
        Function.Locals = Signature.ArgumentTypes;
        Function.ReturnType = Signature.ReturnType;

        const BYTE *FuncLocalsBegin = Stream.CurrentPtr();

        UINT32 LocalBatchCount = 0;
        WATAP_SET_OR_RETURN(LocalBatchCount, ParseUint(Stream), nullptr);
        while (LocalBatchCount--)
        {
          UINT32 BatchSize = 0;
          WATAP_SET_OR_RETURN(BatchSize, ParseUint(Stream), nullptr);
          
          bin::value_type ValueType = bin::value_type::eI32;
          WATAP_SET_OR_RETURN(ValueType, Stream.Get<bin::value_type>(), nullptr);

          while (BatchSize--)
            Function.Locals.push_back(ValueType);
        }

        const BYTE *FuncLocalsEnd = Stream.CurrentPtr();

        Function.Instructions.resize(CodeSize - (FuncLocalsEnd - FuncLocalsBegin));
        std::memcpy(Function.Instructions.data(), Stream.Get<BYTE>(Function.Instructions.size()), Function.Instructions.size());

        Result->Functions.push_back(std::move(Function));
      }
    }

    /* Exports */
    if (auto SectionIter = Sections.find(bin::section_id::eExport); SectionIter != Sections.end())
    {
      binary_stream Stream {SectionIter->second};

      UINT32 ExportCount = 0;
      WATAP_SET_OR_RETURN(ExportCount, ParseUint(Stream), nullptr);

      while (ExportCount--)
      {
        std::span<const BYTE> NameBytes;
        WATAP_SET_OR_RETURN(NameBytes, ParseVec<UINT8>(Stream), nullptr);

        bin::import_export_type ExportType;
        WATAP_SET_OR_RETURN(ExportType, Stream.Get<bin::import_export_type>(), nullptr);
        UINT32 ExportIndex;
        WATAP_SET_OR_RETURN(ExportIndex, ParseUint(Stream), nullptr);

        switch (ExportType)
        {
        case bin::import_export_type::eFunction:
        {
          Result->FunctionExportTable.emplace(std::string(NameBytes.front(), NameBytes.back()), ExportIndex);
          break;
        }
        }
      }
    }

    return Result.release();
  } /* End of 'CreateModuleSource' function */

  /* Module source destroy function.
   * ARGUMENTS:
   *   - module source pointer:
   *       module_source *ModuleSource;
   * RETURNS: None.
   */ 
  VOID interface_impl::DestroyModuleSource( module_source *ModuleSource )
  {
    if (auto Impl = dynamic_cast<module_source_impl *>(ModuleSource))
      delete Impl;
  } /* End of 'DestroyModuleSource' function */
} /* end of 'watap::impl::standard' namespace */

/* END OF 'watap_impl_standard_parser.h' FILE */