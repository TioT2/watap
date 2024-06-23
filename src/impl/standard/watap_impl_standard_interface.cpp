#define WATAP_IMPL_STANDARD
#include "watap_impl_standard_interface.h"

// #define WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED)         \
//   {                                                            \
//     if (TypeStack.empty())                                     \
//       return compile_status::eNoOperands;                      \
//     if (TypeStack.top() != (EXPECTED))                         \
//       return compile_status::eInvalidOperandType;              \
//     TypeStack.pop();                                           \
//   }
// 
// #define WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(EXPECTED_LHS, EXPECTED_RHS, RESULT) \
//   {                                                                                   \
//     WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED_LHS)                                \
//     WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED_RHS)                                \
//     TypeStack.push(RESULT);                                                           \
//   }
// 
// #define WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(EXPECTED, RESULT) \
//   {                                                                \
//     WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED)                 \
//     TypeStack.push(RESULT);                                        \
//   }

namespace watap::impl::standard
{
  /* UINT32 from value parsing function.
   * ARGUMENTS:
   *   - binary stream:
   *       binary_input_stream &Stream;
   * RETURNS:
   *   (std::optional<UINT32>) Parsed UINT32.
   */
  std::optional<UINT32> ParseUint( binary_input_stream &Stream )
  {
    auto [Value, Offset] = leb128::DecodeUnsigned(Stream.CurrentPtr());
    if (Stream.Get<UINT8>(Offset))
      return static_cast<UINT32>(Value);
    return std::nullopt;
  } /* End of 'ParseUint' function */

  /* Function type parsing function.
   * ARGUMENTS:
   *   - stream to parse data from:
   *       binary_stream &Stream;
   * RETURNS:
   *   (std::optional<function_type>) Parsed function type, if parsed.
   */
  std::optional<function_signature> ParseFunctionType( binary_input_stream &Stream )
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

    if (ReturnTypeCount-- > 0)
      WATAP_SET_OR_RETURN(Result.ReturnType, Stream.Get<bin::value_type>(), std::nullopt);

    while (ReturnTypeCount--)
      Stream.Get<bin::value_type>();
    return std::move(Result);
  } /* End of 'binary_stream' structure */

  template <typename type>
    std::optional<std::span<const type>> ParseVec( binary_input_stream &Stream )
    {
      SIZE_T Count;
      WATAP_SET_OR_RETURN(Count, ParseUint(Stream), std::nullopt);

      auto Begin = reinterpret_cast<const type *>(Stream.CurrentPtr());
      Stream.Skip(Count * sizeof(type));
      if (!Stream)
        return std::nullopt;

      return std::span<const type>(Begin, Count);
    } /* End of 'ParseString' function */

  std::optional<std::string> ParseString( binary_input_stream &Stream )
  {
    if (auto CharVec = ParseVec<CHAR>(Stream))
      return std::string {CharVec->begin(), CharVec->end()};
    return std::nullopt;
  } /* End of 'ParseString' function */

  std::optional<bin::limits> ParseLimits( binary_input_stream &Stream )
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

  /* Module data by sections splitting function.
   * ARGUMENTS:
   *   - module data:
   *       std::span<const UINT8> Data;
   * RETURNS:
   *   (std::optional<std::map<bin::section_id, std::span<const UINT8>>>) Module section datas;
   */
  std::optional<std::map<bin::section_id, std::span<const UINT8>>> ParseSections( std::span<const UINT8> Data )
  {
    binary_input_stream Stream {Data};

    // Read and validate module header
    {
      bin::module_header ModuleHeader {};
      WATAP_SET_OR_RETURN(ModuleHeader, Stream.Get<bin::module_header>(), std::nullopt);

      if (!ModuleHeader.Validate())
        return std::nullopt;
    }

    std::map<bin::section_id, std::span<const UINT8>> Sections;

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
        Sections.emplace(SectionId, std::span<const UINT8>(Stream.CurrentPtr(), SectionSize));
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
  source * interface_impl::CreateSource( const source_info &Info )
  {
    if (std::holds_alternative<std::string_view>(Info))
      return nullptr;
    std::span<const UINT8> Data = std::get<std::span<const UINT8>>(Info);

    std::unique_ptr<source_impl> Result {new source_impl()};

    // Parse sections from code
    std::map<bin::section_id, std::span<const UINT8>> Sections;
    WATAP_SET_OR_RETURN(Sections, ParseSections(Data), nullptr);
  
    /* Signature section */
    if (auto SectionIter = Sections.find(bin::section_id::eType); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};

      UINT32 FunctionSignatureCount = 0;
      WATAP_SET_OR_RETURN(FunctionSignatureCount, ParseUint(Stream), nullptr);
  
      Result->FunctionSignatures.reserve(FunctionSignatureCount);
      while (FunctionSignatureCount--)
        WATAP_CALL_OR_RETURN(Result->FunctionSignatures.push_back, ParseFunctionType(Stream), nullptr);
    }

    /* Function section */
    if (auto SectionIter = Sections.find(bin::section_id::eFunction); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};

      UINT32 FunctionCount = 0;
      WATAP_SET_OR_RETURN(FunctionCount, ParseUint(Stream), nullptr);

      while (FunctionCount--)
        WATAP_CALL_OR_RETURN(Result->FunctionSignatureIndices.push_back, ParseUint(Stream), nullptr);
    }

    /* Table section */
    if (auto SectionIter = Sections.find(bin::section_id::eTable); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};

      std::span<const bin::table_type> Tables;
      WATAP_SET_OR_RETURN(Tables, ParseVec<bin::table_type>(Stream), nullptr);
      Result->Tables = {Tables.begin(), Tables.end()};
    }

    /* Global section */
    if (auto SectionIter = Sections.find(bin::section_id::eGlobal); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};


    }

    /* Import section */
    if (auto SectionIter = Sections.find(bin::section_id::eImport); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};

      UINT32 ImportCount = 0;
      WATAP_SET_OR_RETURN(ImportCount, ParseUint(Stream), nullptr);

      for (UINT32 i = 0; i < ImportCount; i++)
      {
        import_name Name;
        import_element Element;

        /* Parse name and element */
        WATAP_SET_OR_RETURN(Name.ModuleName, ParseString(Stream), nullptr);
        WATAP_SET_OR_RETURN(Name.Name, ParseString(Stream), nullptr);
        WATAP_SET_OR_RETURN(Element.Type, Stream.Get<bin::import_export_type>(), nullptr);

        switch (Element.Type)
        {
        case bin::import_export_type::eFunction:
          WATAP_SET_OR_RETURN(Element.Function.TypeIndex, ParseUint(Stream), nullptr);
          break;

        case bin::import_export_type::eTable:
          WATAP_SET_OR_RETURN(Element.Table.ReferenceType, Stream.Get<bin::reference_type>(), nullptr);
          WATAP_SET_OR_RETURN(Element.Table.Limits, ParseLimits(Stream), nullptr);
          break;

        case bin::import_export_type::eMemory:
          WATAP_SET_OR_RETURN(Element.Memory.Limits, ParseLimits(Stream), nullptr);
          break;

        case bin::import_export_type::eGlobal:
          WATAP_SET_OR_RETURN(Element.Global.Type, Stream.Get<bin::value_type>(), nullptr);
          WATAP_SET_OR_RETURN(Element.Global.Mutability, Stream.Get<bin::mutability>(), nullptr);
          break;

        default:
          // Error: unknown type
          return nullptr;
        }

        Result->RequiredImports[Name] = Element;
      }
    }

    /* Code section */
    if (auto SectionIter = Sections.find(bin::section_id::eCode); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};

      UINT32 FunctionCount = 0;
      WATAP_SET_OR_RETURN(FunctionCount, ParseUint(Stream), nullptr);

      for (UINT32 i = 0; i < FunctionCount; i++)
      {
        const function_signature &Signature = Result->FunctionSignatures[Result->FunctionSignatureIndices[i]];

        UINT32 CodeSize = 0;
        WATAP_SET_OR_RETURN(CodeSize, ParseUint(Stream), nullptr);

        Result->Functions.push_back(raw_function_data
        {
          .SignatureIndex = Result->FunctionSignatureIndices[i],
          .Instructions = {Stream.CurrentPtr(), Stream.CurrentPtr() + static_cast<SIZE_T>(CodeSize)},
        });

        Stream.Skip(CodeSize);
      }
    }

    /* Export section */
    if (auto SectionIter = Sections.find(bin::section_id::eExport); SectionIter != Sections.end())
    {
      binary_input_stream Stream {SectionIter->second};

      UINT32 ExportCount = 0;
      WATAP_SET_OR_RETURN(ExportCount, ParseUint(Stream), nullptr);

      while (ExportCount--)
      {
        std::string Name;
        WATAP_SET_OR_RETURN(Name, ParseString(Stream), nullptr);

        bin::import_export_type ExportType;
        WATAP_SET_OR_RETURN(ExportType, Stream.Get<bin::import_export_type>(), nullptr);
        UINT32 ExportIndex;
        WATAP_SET_OR_RETURN(ExportIndex, ParseUint(Stream), nullptr);

        switch (ExportType)
        {
        case bin::import_export_type::eFunction:
        {
          Result->FunctionExportTable.emplace(std::move(Name), ExportIndex);
          break;
        }
        }
      }
    }

    /* Return pointer to actual module */
    return Result.release();
  } /* End of 'CreateSource' function */

  /* Module source destroy function.
   * ARGUMENTS:
   *   - module source pointer:
   *       module_source *ModuleSource;
   * RETURNS: None.
   */ 
  VOID interface_impl::DestroySource( source *ModuleSource )
  {
    if (auto Impl = dynamic_cast<source_impl *>(ModuleSource))
      delete Impl;
  } /* End of 'DestroyModuleSource' function */
} /* end of 'watap::impl::standard' namespace */

/* END OF 'watap_impl_standard_parser.h' FILE */
