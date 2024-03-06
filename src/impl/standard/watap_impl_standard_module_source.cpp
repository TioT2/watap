#define WATAP_IMPL_STANDARD
#include "watap_impl_standard_interface.h"

#define WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED)         \
  {                                                            \
    if (TypeStack.empty())                                     \
      return compile_status::eNoOperands;                      \
    if (TypeStack.top() != (EXPECTED))                         \
      return compile_status::eInvalidOperandType;              \
    TypeStack.pop();                                           \
  }

#define WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(EXPECTED_LHS, EXPECTED_RHS, RESULT) \
  {                                                                                   \
    WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED_LHS)                                \
    WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED_RHS)                                \
    TypeStack.push(RESULT);                                                           \
  }

#define WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(EXPECTED, RESULT) \
  {                                                                \
    WATAP_STANDARD_COMPILE_TYPE_VALIDATE(EXPECTED)                 \
    TypeStack.push(RESULT);                                        \
  }

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

    if (ReturnTypeCount-- > 0)
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

  class instruction_decoder
  {
  public:
  };  /* End of 'code' class */

  /* Module data by sections splitting function.
   * ARGUMENTS:
   *   - module data:
   *       std::span<const UINT8> Data;
   * RETURNS:
   *   (std::optional<std::map<bin::section_id, std::span<const UINT8>>>) Module section datas;
   */
  std::optional<std::map<bin::section_id, std::span<const UINT8>>> ParseSections( std::span<const UINT8> Data )
  {
    binary_stream Stream {Data};

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

  /* Function compilation status */
  enum class compile_status
  {
    eOk,                       // Ok
    eNoOperands,               // Error on stack (go under zero)
    eInvalidOperandType,       // Invalid operand type
    eExtensionNotPresent,      // Extension not present (e.g. vector extension, that already have own )
    eInvalidCallParemeters,    // Invalid call parameters
    eUnknownNullReferenceType, // opRefNull argument invalid type
  }; /* End of 'instruction_parsing_status' enumeration */

  /* Function descriptor type */
  struct function_header
  {
    std::vector<bin::value_type> Locals;       // Local variable set
    UINT32 ArgumentCount;                      // Count of locals initially consumed from stack as arguments
    std::optional<bin::value_type> ReturnType; // Local variable set

    UINT32 CodeBeginOffset;                    // Function code offset in code
    UINT32 CodeEndOffset;                      // Code end offset
  }; /* End of 'function_header' structure */

  /* Compilation result representation structure */
  struct compile_result
  {
    compile_status Status;      // Compilation status
    std::vector<BYTE> Bytecode; // TAP Bytecode
  }; /* End of 'compile_result' structure */

  /* TODO TC4 WASM COMPILER
   * ADD call semantics validation
   * ADD Type validation
   * ADD Integer decoding
   */

  /* Function instructions parsing function.
   * ARGUMENTS:
   *   - function signature list:
   *       function &Function:
   *   - raw wasm instructions:
   *       std::span<const BYTE> Instructions;
   */
  compile_status CompileCode( std::span<const function_signature> SignatureList, std::span<const BYTE> Instructions )
  {
    const BYTE *InstructionPointer = Instructions.data();
    const BYTE *InstructionEnd = InstructionPointer + Instructions.size();
    std::vector<BYTE> OutInstructions;

    // Current top value
    std::stack<bin::value_type> TypeStack;
    BOOL Interrupted = FALSE;

    while (InstructionPointer < InstructionEnd)
    {
      bin::instruction Instruction = static_cast<bin::instruction>(*InstructionPointer);

      switch (Instruction)
      {
        case bin::instruction::eUnreachable        :
          break;

        case bin::instruction::eNop                :
          break;

        case bin::instruction::eBlock              :
          break;

        case bin::instruction::eLoop               :
        case bin::instruction::eIf                 :
        case bin::instruction::eElse               :
        case bin::instruction::eExpressionEnd      :
        case bin::instruction::eBr                 :
        case bin::instruction::eBrIf               :
        case bin::instruction::eReturn             :
        case bin::instruction::eBrTable            :
        case bin::instruction::eCall               :
        case bin::instruction::eCallIndirect       :

        case bin::instruction::eDrop               :
        case bin::instruction::eSelect             :
        case bin::instruction::eSelectTyped        :

        case bin::instruction::eLocalGet           :
        case bin::instruction::eLocalSet           :
        case bin::instruction::eLocalTee           :
        case bin::instruction::eGlobalGet          :
        case bin::instruction::eGlobalSet          :

        case bin::instruction::eTableGet           :
        case bin::instruction::eTableSet           :

        case bin::instruction::eI32Load            : TypeStack.push(bin::value_type::eI32); break;
        case bin::instruction::eI64Load            : TypeStack.push(bin::value_type::eI64); break;
        case bin::instruction::eF32Load            : TypeStack.push(bin::value_type::eF32); break;
        case bin::instruction::eF64Load            : TypeStack.push(bin::value_type::eF64); break;

        case bin::instruction::eI32Load8S          : TypeStack.push(bin::value_type::eI32); break;
        case bin::instruction::eI32Load8U          : TypeStack.push(bin::value_type::eI32); break;
        case bin::instruction::eI32Load16S         : TypeStack.push(bin::value_type::eI32); break;
        case bin::instruction::eI32Load16U         : TypeStack.push(bin::value_type::eI32); break;

        case bin::instruction::eI64Load8S          : TypeStack.push(bin::value_type::eI64); break;
        case bin::instruction::eI64Load8U          : TypeStack.push(bin::value_type::eI64); break;
        case bin::instruction::eI64Load16S         : TypeStack.push(bin::value_type::eI64); break;
        case bin::instruction::eI64Load16U         : TypeStack.push(bin::value_type::eI64); break;
        case bin::instruction::eI64Load32S         : TypeStack.push(bin::value_type::eI64); break;
        case bin::instruction::eI64Load32U         : TypeStack.push(bin::value_type::eI64); break;

        case bin::instruction::eI32Store           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eI32) break;
        case bin::instruction::eI64Store           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eI64) break;
        case bin::instruction::eF32Store           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eF32) break;
        case bin::instruction::eF64Store           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eF64) break;

        case bin::instruction::eI32Store8          :
        case bin::instruction::eI32Store16         :
          WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eI32)
          break;

        case bin::instruction::eI64Store8          :
        case bin::instruction::eI64Store16         :
        case bin::instruction::eI64Store32         :
          WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eI64)
          break;

        case bin::instruction::eMemorySize         : break;
        case bin::instruction::eMemoryGrow         : break;

        case bin::instruction::eI32Const           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eI32) break;
        case bin::instruction::eI64Const           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eI64) break;
        case bin::instruction::eF32Const           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eF32) break;
        case bin::instruction::eF64Const           : WATAP_STANDARD_COMPILE_TYPE_VALIDATE(bin::value_type::eF64) break;

        case bin::instruction::eI32Eqz             :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI32)

        case bin::instruction::eI32Eq              :
        case bin::instruction::eI32Ne              :
        case bin::instruction::eI32LtS             :
        case bin::instruction::eI32LtU             :
        case bin::instruction::eI32GtS             :
        case bin::instruction::eI32GtU             :
        case bin::instruction::eI32LeS             :
        case bin::instruction::eI32LeU             :
        case bin::instruction::eI32GeS             :
        case bin::instruction::eI32GeU             :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI32, bin::value_type::eI32)
          break;

        case bin::instruction::eI64Eqz             :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI32)
          break;

        case bin::instruction::eI64Eq              :
        case bin::instruction::eI64Ne              :
        case bin::instruction::eI64LtS             :
        case bin::instruction::eI64LtU             :
        case bin::instruction::eI64GtS             :
        case bin::instruction::eI64GtU             :
        case bin::instruction::eI64LeS             :
        case bin::instruction::eI64LeU             :
        case bin::instruction::eI64GeS             :
        case bin::instruction::eI64GeU             :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI32)
          break;

        case bin::instruction::eF32Eq              :
        case bin::instruction::eF32Ne              :
        case bin::instruction::eF32Lt              :
        case bin::instruction::eF32Gt              :
        case bin::instruction::eF32Le              :
        case bin::instruction::eF32Ge              :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eI32)
          break;

        case bin::instruction::eF64Eq              :
        case bin::instruction::eF64Ne              :
        case bin::instruction::eF64Lt              :
        case bin::instruction::eF64Gt              :
        case bin::instruction::eF64Le              :
        case bin::instruction::eF64Ge              :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eF64, bin::value_type::eI32)
          break;

        case bin::instruction::eI32Clz             :
        case bin::instruction::eI32Ctz             :
        case bin::instruction::eI32Popcnt          :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI32)
          break;

        case bin::instruction::eI32Add             :
        case bin::instruction::eI32Sub             :
        case bin::instruction::eI32Mul             :
        case bin::instruction::eI32DivS            :
        case bin::instruction::eI32DivU            :
        case bin::instruction::eI32RemS            :
        case bin::instruction::eI32RemU            :
        case bin::instruction::eI32And             :
        case bin::instruction::eI32Or              :
        case bin::instruction::eI32Xor             :
        case bin::instruction::eI32Shl             :
        case bin::instruction::eI32ShrS            :
        case bin::instruction::eI32ShrU            :
        case bin::instruction::eI32Rotl            :
        case bin::instruction::eI32Rotr            :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI64)
          break;

        case bin::instruction::eI64Clz             :
        case bin::instruction::eI64Ctz             :
        case bin::instruction::eI64Popcnt          :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI64)
          break;

        case bin::instruction::eI64Add             :
        case bin::instruction::eI64Sub             :
        case bin::instruction::eI64Mul             :
        case bin::instruction::eI64DivS            :
        case bin::instruction::eI64DivU            :
        case bin::instruction::eI64RemS            :
        case bin::instruction::eI64RemU            :
        case bin::instruction::eI64And             :
        case bin::instruction::eI64Or              :
        case bin::instruction::eI64Xor             :
        case bin::instruction::eI64Shl             :
        case bin::instruction::eI64ShrS            :
        case bin::instruction::eI64ShrU            :
        case bin::instruction::eI64Rotl            :
        case bin::instruction::eI64Rotr            :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI64)
          break;

        case bin::instruction::eF32Abs             :
        case bin::instruction::eF32Neg             :
        case bin::instruction::eF32Ceil            :
        case bin::instruction::eF32Floor           :
        case bin::instruction::eF32Trunc           :
        case bin::instruction::eF32Nearest         :
        case bin::instruction::eF32Sqrt            :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eF32)
          break;

        case bin::instruction::eF32Add             :
        case bin::instruction::eF32Sub             :
        case bin::instruction::eF32Mul             :
        case bin::instruction::eF32Div             :
        case bin::instruction::eF32Min             :
        case bin::instruction::eF32Max             :
        case bin::instruction::eF32CopySign        :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eF32)
          break;

        case bin::instruction::eF64Abs             :
        case bin::instruction::eF64Neg             :
        case bin::instruction::eF64Ceil            :
        case bin::instruction::eF64Floor           :
        case bin::instruction::eF64Trunc           :
        case bin::instruction::eF64Nearest         :
        case bin::instruction::eF64Sqrt            :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eF32)
          break;

        case bin::instruction::eF64Add             :
        case bin::instruction::eF64Sub             :
        case bin::instruction::eF64Mul             :
        case bin::instruction::eF64Div             :
        case bin::instruction::eF64Min             :
        case bin::instruction::eF64Max             :
        case bin::instruction::eF64CopySign        :
          WATAP_STANDARD_COMPILE_BINARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eF32)
          break;

        case bin::instruction::eI32WrapI64         : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI64) break;

        case bin::instruction::eI32TruncF32S       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eI32) break;
        case bin::instruction::eI32TruncF32U       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eI32) break;
        case bin::instruction::eI32TruncF64S       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eI32) break;
        case bin::instruction::eI32TruncF64U       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eI32) break;

        case bin::instruction::eI64ExtendI32S      : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI64) break;
        case bin::instruction::eI64ExtendI32U      : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI64) break;
        case bin::instruction::eI64TruncF32S       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eI64) break;
        case bin::instruction::eI64TruncF32U       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eI64) break;
        case bin::instruction::eI64TruncF64S       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eI64) break;
        case bin::instruction::eI64TruncF64U       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eI64) break;

        case bin::instruction::eF32ConvertI32S     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eF32) break;
        case bin::instruction::eF32ConvertI32U     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eF32) break;
        case bin::instruction::eF32ConvertI64S     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eF32) break;
        case bin::instruction::eF32ConvertI64U     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eF32) break;
        case bin::instruction::eF32DemoteF64       : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eF32) break;

        case bin::instruction::eF64ConvertI32S     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eF64) break;
        case bin::instruction::eF64ConvertI32U     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eF64) break;
        case bin::instruction::eF64ConvertI64S     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eF64) break;
        case bin::instruction::eF64ConvertI64U     : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eF64) break;
        case bin::instruction::eF64PromoteF32      : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eF64) break;

        case bin::instruction::eI32ReinterpretF32  : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF32, bin::value_type::eI32) break;
        case bin::instruction::eI64ReinterpretF64  : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eF64, bin::value_type::eI64) break;
        case bin::instruction::eF32ReinterpretI32  : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eF32) break;
        case bin::instruction::eF64ReinterpretI64  : WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eF64) break;

        case bin::instruction::eI32Extend8S        :
        case bin::instruction::eI32Extend16S       :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI32, bin::value_type::eI32)
          break;

        case bin::instruction::eI64Extend8S        :
        case bin::instruction::eI64Extend16S       :
        case bin::instruction::eI64Extend32S       :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI64)
          break;

        case bin::instruction::eRefNull           :
        {
          const UINT8 Null[] {0, 0, 0, 0};
          auto [Type, Offset] = leb128::DecodeUnsigned(InstructionPointer);

          switch ((bin::reference_type)Type)
          {
          case bin::reference_type::eExternRef:
          case bin::reference_type::eFuncRef:
            TypeStack.push(bin::value_type::eExternRef);

            InstructionPointer += Offset;

            OutInstructions.push_back(static_cast<BYTE>(bin::instruction::eI32Const));
            OutInstructions.insert(OutInstructions.end(), Null, Null + 4);
            break;

          default:
            return compile_status::eUnknownNullReferenceType;
          }
        }

        case bin::instruction::eRefIsNull         :
          WATAP_STANDARD_COMPILE_UNARY_OP_VALIDATE(bin::value_type::eI64, bin::value_type::eI32)
          break;

        case bin::instruction::eRefFunc           :
        {
          const UINT8 Null[] {0, 0, 0, 0};
          auto [Type, Offset] = leb128::DecodeUnsigned(InstructionPointer);

          switch ((bin::reference_type)Type)
          {
          case bin::reference_type::eExternRef:
          case bin::reference_type::eFuncRef:
            TypeStack.push(bin::value_type::eExternRef);

            InstructionPointer += Offset;

            OutInstructions.push_back(static_cast<BYTE>(bin::instruction::eI32Const));
            OutInstructions.insert(OutInstructions.end(), Null, Null + 4);
            break;

          default:
            return compile_status::eUnknownNullReferenceType;
          }
        }

        case bin::instruction::eSystem            :
        case bin::instruction::eVector            :
          break;
      }
    }

    return compile_status::eOk;
  } /* End of 'ExpandFnInstructions' function */

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
    std::span<const UINT8> Data = std::get<std::span<const UINT8>>(Info);

    std::unique_ptr<module_source_impl> Result {new module_source_impl()};

    // Parse sections from code
    std::map<bin::section_id, std::span<const UINT8>> Sections;
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

    /* Code section */
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

        Function.ArgumentCount = (UINT32)Signature.ArgumentTypes.size();
        Function.Locals = Signature.ArgumentTypes;
        Function.ReturnType = Signature.ReturnType;

        const UINT8 *FuncLocalsBegin = Stream.CurrentPtr();

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

        const UINT8 *FuncLocalsEnd = Stream.CurrentPtr();

        Function.Instructions.resize(CodeSize - (FuncLocalsEnd - FuncLocalsBegin));
        std::memcpy(Function.Instructions.data(), Stream.Get<UINT8>(Function.Instructions.size()), Function.Instructions.size());

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
        std::span<const UINT8> NameBytes;
        WATAP_SET_OR_RETURN(NameBytes, ParseVec<UINT8>(Stream), nullptr);

        bin::import_export_type ExportType;
        WATAP_SET_OR_RETURN(ExportType, Stream.Get<bin::import_export_type>(), nullptr);
        UINT32 ExportIndex;
        WATAP_SET_OR_RETURN(ExportIndex, ParseUint(Stream), nullptr);

        switch (ExportType)
        {
        case bin::import_export_type::eFunction:
        {
          Result->FunctionExportTable.emplace(std::string(NameBytes.begin(), NameBytes.end()), ExportIndex);
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
