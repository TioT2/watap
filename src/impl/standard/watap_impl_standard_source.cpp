#define WATAP_IMPL_STANDARD

#include "watap_impl_standard_interface.h"

namespace watap::impl::standard
{
  struct compilation_context
  {
    std::span<const UINT32> FunctionSignatureIndices;       // Function signature indices
    std::span<const function_signature> FunctionSignatures; // Funciton signatures

    const UINT8 *PStart;              // Start instructin pointer
    const UINT8 *PCurrentInstruction; // Current instruction pointer
    const UINT8 *PEnd;                // End pointer

    binary_input_stream InStream;                           // Input stream
    std::stack<bin::value_type> TypeStack;                  // Type stack
    std::vector<compiled_instruction> Instructions;         // Output instructions
    std::vector<bin::value_type> LocalTypes;                // Local frame

    /* Instruction passing function.
     * ARGUMENTS:
     *   - instruction:
     *       bin::instruction Instruction;
     *   - additional data:
     *       UINT8 AdditionalData = 0;
     * RETURNS: None.
     */
    VOID PassInstruction( bin::instruction Instruction, UINT8 AdditionalData = 0 )
    {
      Instructions.push_back(compiled_instruction {Instruction, AdditionalData});
    } /* End of 'PassInstruction' function */

    VOID PassU16( UINT16 Constant )
    {
      Instructions.push_back(compiled_instruction { .InstructionID = Constant });
    }

    VOID PassU32( UINT32 Constant )
    {
      Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant      ) });
      Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 16) });
    }

    VOID PassU64( UINT64 Constant )
    {
      Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant      ) });
      Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 16) });
      Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 32) });
      Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 48) });
    }

    BOOL CheckType( bin::value_type Type )
    {
      return TypeStack.top() == Type;
    }

    VOID UnaryOperator( bin::instruction Instruction, bin::value_type SrcType, bin::value_type DstType )
    {
      if (TypeStack.empty())
        throw compile_status::eNoOperandsForUnary;

      if (TypeStack.top() != SrcType)
        throw compile_status::eInvalidOperandType;
      TypeStack.pop();
      TypeStack.push(DstType);
      Instructions.push_back(compiled_instruction {Instruction});
    }

    VOID UnaryOperatorNoAdd( bin::value_type SrcType, bin::value_type DstType )
    {
      if (TypeStack.empty())
        throw compile_status::eNoOperandsForUnary;

      if (TypeStack.top() != SrcType)
        throw compile_status::eInvalidOperandType;

      TypeStack.pop();
      TypeStack.push(DstType);
    }

    VOID BinaryOperator( bin::instruction Instruction, bin::value_type LhsType, bin::value_type RhsType, bin::value_type DstType )
    {
      if (TypeStack.size() < 2)
        throw compile_status::eNoOperandsForBinary;

      if (TypeStack.top() != LhsType)
        throw compile_status::eInvalidOperandType;
      TypeStack.pop();

      if (TypeStack.top() != RhsType)
        throw compile_status::eInvalidOperandType;
      TypeStack.pop();

      TypeStack.push(DstType);
      Instructions.push_back(compiled_instruction {Instruction});
    }

    struct block_compilation_result
    {
      compile_status Status;                                            // Compilation status
      std::optional<bin::instruction> EndingInstruction = std::nullopt; // Block compilation ending byte
    }; /* End of 'block_compilation_result' structure */

    /* Block type representation structure */
    using block_type = std::variant<std::nullopt_t, bin::value_type, UINT32>;

    /* Instruciton getting function.
     * ARGUMENTRS: None.
     * RETURNS: None.
     */
    std::optional<bin::instruction> TryGetInstruction( VOID ) noexcept
    {
      if (PEnd < PCurrentInstruction + 1)
        return *reinterpret_cast<const bin::instruction *>(PCurrentInstruction);
      return std::nullopt;
    } /* End of 'GetInstruction' function */

    SIZE_T ParseUnsigned( VOID )
    {
      auto [U, Offset] = leb128::DecodeUnsigned(PCurrentInstruction);
      PCurrentInstruction += Offset;
      return U;
    } /* End of 'ParseUint' functions */

    template <UINT32 BIT_COUNT>
      SIZE_T ParseSigned( VOID )
      {
        auto [U, Offset] = leb128::DecodeSigned<BIT_COUNT>(PCurrentInstruction);
        PCurrentInstruction += Offset;
        return U;
      } /* End of 'ParseInt' function */

    template <typename t>
      t ParseRaw( VOID )
      {
        PCurrentInstruction += sizeof(t);
        return *reinterpret_cast<const t *>(PCurrentInstruction - sizeof(t));
      }

    /* Block compilation function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (std::optional<UINT8>) Block-ending byte.
     */
    block_compilation_result CompileBlock( block_type BlockType )
    {
      binary_input_stream Stream {std::span<const UINT8>()};
      auto TargetStackSize = TypeStack.size();

      if (auto *PTypeIndex = std::get_if<UINT32>(&BlockType))
      {
        if (*PTypeIndex >= FunctionSignatures.size())
          return { .Status = compile_status::eInvalidFunctionTypeIndex };
        std::span ArgTypes = FunctionSignatures[*PTypeIndex].ArgumentTypes;

      }

      std::optional<bin::value_type> TargetType = std::nullopt;

      // R - ridabiliti
      if (auto Res = std::visit([&]( auto Arg ) -> std::optional<block_compilation_result>
        {
          using arg_t = decltype(Arg);

          if constexpr (std::same_as<arg_t, bin::value_type>)
          {
            TargetType = Arg;
            TargetStackSize += 1;
          }
          else if constexpr (std::same_as<arg_t, UINT32>)
          {
            if (Arg >= FunctionSignatures.size())
              return block_compilation_result { .Status = compile_status::eInvalidFunctionTypeIndex };
            const function_signature &Signature = FunctionSignatures[Arg];
            
            TargetType = Signature.ReturnType;

            // Validate type stack
            Signature.ArgumentTypes;

            TargetStackSize += Signature.ReturnType.has_value() - Signature.ArgumentTypes.size();
          }

          return std::nullopt;
        }, BlockType))
        return *Res;

      while (auto InstructionOpt = TryGetInstruction())
      {
        const bin::instruction Instruction = *InstructionOpt;

        try
        {
          switch (Instruction)
          {
          case bin::instruction::eUnreachable   :
          case bin::instruction::eNop           :
          case bin::instruction::eReturn        :
            PassInstruction(Instruction);
            break;

          case bin::instruction::eMemoryGrow    :
          case bin::instruction::eMemorySize    :
            PassInstruction(Instruction);
            TypeStack.push(bin::value_type::eI32);
            break;

          // TODO TC4 WASM
          case bin::instruction::eBlock         :
          case bin::instruction::eLoop          :
          case bin::instruction::eIf            :
          case bin::instruction::eElse          :
          case bin::instruction::eExpressionEnd :
          case bin::instruction::eBr            :
          case bin::instruction::eBrIf          :
          case bin::instruction::eBrTable       :
            throw compile_status::eUnsupportedFeature;

          // Match type stack with called function signature
          case bin::instruction::eCall          :
            {
              const UINT32 FunctionIndex = ParseUnsigned();

              if (FunctionIndex >= FunctionSignatureIndices.size())
                throw compile_status::eInvalidFunctionIndex;

              const auto &CallSignature = FunctionSignatures[FunctionSignatureIndices[FunctionIndex]];

              if (TypeStack.size() < CallSignature.ArgumentTypes.size())
                throw compile_status::eNoFunctionArguments;

              for (const auto Arg : CallSignature.ArgumentTypes)
              {
                if (TypeStack.top() != Arg)
                  throw compile_status::eInvalidFunctionArgumentsType;
                TypeStack.pop();
              }

              if (CallSignature.ReturnType)
                TypeStack.push(*CallSignature.ReturnType);
              PassInstruction(Instruction);
              PassU32(static_cast<UINT32>(FunctionIndex));
              break;
            }

          case bin::instruction::eCallIndirect  :
            throw compile_status::eUnsupportedFeature;

          case bin::instruction::eDrop          :
            {
              if (TypeStack.empty())
                  throw compile_status::eNoOperandsForUnary;

              PassInstruction(bin::instruction::eDrop, static_cast<UINT8>(bin::GetValueTypeSize(TypeStack.top())));
              TypeStack.pop();
              break;
            }

          case bin::instruction::eSelect        :
            throw compile_status::eUnsupportedFeature;

          case bin::instruction::eSelectTyped   :
            throw compile_status::eUnsupportedFeature;

          case bin::instruction::eLocalGet  :
            {
              const UINT32 LocalIndex = ParseUnsigned();

              if (LocalTypes.size() >= LocalIndex)
                throw compile_status::eInvalidLocalIndex;

              TypeStack.push(LocalTypes[LocalIndex]);

              PassInstruction(bin::instruction::eLocalGet);
              PassU16(static_cast<UINT16>(LocalIndex));
              break;
            }

          case bin::instruction::eLocalSet  :
          case bin::instruction::eLocalTee  :
            {
              const UINT32 LocalIndex = ParseUnsigned();

              if (LocalTypes.size() >= LocalIndex)
                throw compile_status::eInvalidLocalIndex;

              if (TypeStack.top() != LocalTypes[LocalIndex])
                throw compile_status::eInvalidOperandType;

              if (Instruction != bin::instruction::eLocalTee)
                TypeStack.pop();

              PassInstruction(bin::instruction::eLocalSet, static_cast<UINT8>(bin::GetValueTypeSize(LocalTypes[LocalIndex])));
              PassU16(static_cast<UINT16>(LocalIndex));
              break;
            }

          case bin::instruction::eGlobalGet ://         = 0x23, // Load global
          case bin::instruction::eGlobalSet ://         = 0x24, // Store global

          case bin::instruction::eTableGet : //          = 0x25, // Get value from table
          case bin::instruction::eTableSet : //          = 0x26, // Set value into table
            throw compile_status::eUnsupportedFeature;

          case bin::instruction::eI32Load    :
          case bin::instruction::eI64Load    :
          case bin::instruction::eF32Load    :
          case bin::instruction::eF64Load    :
          case bin::instruction::eI32Load8S  :
          case bin::instruction::eI32Load8U  :
          case bin::instruction::eI32Load16S :
          case bin::instruction::eI32Load16U :
          case bin::instruction::eI64Load8S  :
          case bin::instruction::eI64Load8U  :
          case bin::instruction::eI64Load16S :
          case bin::instruction::eI64Load16U :
          case bin::instruction::eI64Load32S :
          case bin::instruction::eI64Load32U :
            {
              bin::value_type StackType;

              switch (Instruction)
              {
              case bin::instruction::eI32Load    : StackType = bin::value_type::eI32; break;
              case bin::instruction::eI64Load    : StackType = bin::value_type::eI64; break;
              case bin::instruction::eF32Load    : StackType = bin::value_type::eF32; break;
              case bin::instruction::eF64Load    : StackType = bin::value_type::eF64; break;
              case bin::instruction::eI32Load8S  : StackType = bin::value_type::eI32; break;
              case bin::instruction::eI32Load8U  : StackType = bin::value_type::eI32; break;
              case bin::instruction::eI32Load16S : StackType = bin::value_type::eI32; break;
              case bin::instruction::eI32Load16U : StackType = bin::value_type::eI32; break;
              case bin::instruction::eI64Load8S  : StackType = bin::value_type::eI64; break;
              case bin::instruction::eI64Load8U  : StackType = bin::value_type::eI64; break;
              case bin::instruction::eI64Load16S : StackType = bin::value_type::eI64; break;
              case bin::instruction::eI64Load16U : StackType = bin::value_type::eI64; break;
              case bin::instruction::eI64Load32S : StackType = bin::value_type::eI64; break;
              case bin::instruction::eI64Load32U : StackType = bin::value_type::eI64; break;
              }

              TypeStack.push(StackType);

              PassInstruction(Instruction);
              PassU32(ParseUnsigned());
              break; // = 0x2B, // Load F64 from variable
            }

          case bin::instruction::eI32Store   :
          case bin::instruction::eI64Store   :
          case bin::instruction::eF32Store   :
          case bin::instruction::eF64Store   :
          case bin::instruction::eI32Store8  :
          case bin::instruction::eI32Store16 :
          case bin::instruction::eI64Store8  :
          case bin::instruction::eI64Store16 :
          case bin::instruction::eI64Store32 :
            {
              ParseUnsigned();

              bin::value_type RequiredType;
              switch (Instruction)
              {
              case bin::instruction::eI32Store   : RequiredType = bin::value_type::eI32; break;
              case bin::instruction::eI64Store   : RequiredType = bin::value_type::eI64; break;
              case bin::instruction::eF32Store   : RequiredType = bin::value_type::eF32; break;
              case bin::instruction::eF64Store   : RequiredType = bin::value_type::eF64; break;
              case bin::instruction::eI32Store8  : RequiredType = bin::value_type::eI32; break;
              case bin::instruction::eI32Store16 : RequiredType = bin::value_type::eI32; break;
              case bin::instruction::eI64Store8  : RequiredType = bin::value_type::eI64; break;
              case bin::instruction::eI64Store16 : RequiredType = bin::value_type::eI64; break;
              case bin::instruction::eI64Store32 : RequiredType = bin::value_type::eI64; break;
              }

              if (TypeStack.empty())
                throw compile_status::eNoOperandsForUnary;

              if (TypeStack.top() != RequiredType)
                throw compile_status::eInvalidOperandType;

              TypeStack.pop();
              PassInstruction(Instruction);
              break;
            }

          case bin::instruction::eI32Const          :
            {
              TypeStack.push(bin::value_type::eI32);
              PassInstruction(bin::instruction::eI32Const);
              PassU32(ParseSigned<32>());
              break;
            }

          case bin::instruction::eF32Const          :
            {
              TypeStack.push(bin::value_type::eF32);
              PassInstruction(bin::instruction::eF32Const);
              PassU32(ParseRaw<UINT32>());
              break;
            }

          case bin::instruction::eI64Const          :
            {
              TypeStack.push(bin::value_type::eI32);
              PassInstruction(bin::instruction::eI64Const);
              PassU64(ParseSigned<64>());
              break;
            }

          case bin::instruction::eF64Const          :
            {
              TypeStack.push(bin::value_type::eF64);
              PassInstruction(bin::instruction::eF64Const);
              PassU64(ParseRaw<FLOAT64>());
              break;
            }

          case bin::instruction::eI32Eqz             :
            UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32);
            break;

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
            BinaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32, bin::value_type::eI32);
            break;

          case bin::instruction::eI64Eqz             :
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
            BinaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI32);
            break;

          case bin::instruction::eF32Eq              :
          case bin::instruction::eF32Ne              :
          case bin::instruction::eF32Lt              :
          case bin::instruction::eF32Gt              :
          case bin::instruction::eF32Le              :
          case bin::instruction::eF32Ge              :
            BinaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eI32);
            break;


          case bin::instruction::eF64Eq              :
          case bin::instruction::eF64Ne              :
          case bin::instruction::eF64Lt              :
          case bin::instruction::eF64Gt              :
          case bin::instruction::eF64Le              :
          case bin::instruction::eF64Ge              :
            BinaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eF64, bin::value_type::eI32);
            break;

          case bin::instruction::eI32Clz             :
          case bin::instruction::eI32Ctz             :
          case bin::instruction::eI32Popcnt          :
            UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32);
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
            BinaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32, bin::value_type::eI32);
            break;

          case bin::instruction::eI64Clz             :
          case bin::instruction::eI64Ctz             :
          case bin::instruction::eI64Popcnt          :
            UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64);
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
            BinaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI64);
            break;


          case bin::instruction::eF32Abs             :
          case bin::instruction::eF32Neg             :
          case bin::instruction::eF32Ceil            :
          case bin::instruction::eF32Floor           :
          case bin::instruction::eF32Trunc           :
          case bin::instruction::eF32Nearest         :
          case bin::instruction::eF32Sqrt            :
            UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32);
            break;

          case bin::instruction::eF32Add             :
          case bin::instruction::eF32Sub             :
          case bin::instruction::eF32Mul             :
          case bin::instruction::eF32Div             :
          case bin::instruction::eF32Min             :
          case bin::instruction::eF32Max             :
          case bin::instruction::eF32CopySign        :
            BinaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eF32);
            break;

          case bin::instruction::eF64Abs             :
          case bin::instruction::eF64Neg             :
          case bin::instruction::eF64Ceil            :
          case bin::instruction::eF64Floor           :
          case bin::instruction::eF64Trunc           :
          case bin::instruction::eF64Nearest         :
          case bin::instruction::eF64Sqrt            :
            UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32);
            break;

          case bin::instruction::eF64Add             :
          case bin::instruction::eF64Sub             :
          case bin::instruction::eF64Mul             :
          case bin::instruction::eF64Div             :
          case bin::instruction::eF64Min             :
          case bin::instruction::eF64Max             :
          case bin::instruction::eF64CopySign        :
            BinaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eF32);
            break;

          case bin::instruction::eI32WrapI64         :
            UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI32);
            break;

          case bin::instruction::eI32TruncF32S       :
          case bin::instruction::eI32TruncF32U       :
            UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eI32);
            break;

          case bin::instruction::eI32TruncF64S       :
          case bin::instruction::eI32TruncF64U       :
            UnaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eI32);
            break;

          case bin::instruction::eI64ExtendI32S      :
          case bin::instruction::eI64ExtendI32U      :
            UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI64);
            break;

          case bin::instruction::eI64TruncF32S       :
          case bin::instruction::eI64TruncF32U       :
            UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eI64);
            break;

          case bin::instruction::eI64TruncF64S       :
          case bin::instruction::eI64TruncF64U       :
            UnaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eI64);
            break;

          case bin::instruction::eF32ConvertI32S     :
          case bin::instruction::eF32ConvertI32U     :
            UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eF32);
            break;

          case bin::instruction::eF32ConvertI64S     :
          case bin::instruction::eF32ConvertI64U     :
            UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eF32);
            break;

          case bin::instruction::eF32DemoteF64       :
            UnaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eF32);
            break;

          case bin::instruction::eF64ConvertI32S     :
          case bin::instruction::eF64ConvertI32U     :
            UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eF64);
            break;

          case bin::instruction::eF64ConvertI64S     :
          case bin::instruction::eF64ConvertI64U     :
            UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eF64);
            break;

          case bin::instruction::eF64PromoteF32      :
            UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF64);
            break;

          case bin::instruction::eI32ReinterpretF32  : UnaryOperatorNoAdd(bin::value_type::eF32, bin::value_type::eI32); break;
          case bin::instruction::eI64ReinterpretF64  : UnaryOperatorNoAdd(bin::value_type::eF64, bin::value_type::eI64); break;
          case bin::instruction::eF32ReinterpretI32  : UnaryOperatorNoAdd(bin::value_type::eI32, bin::value_type::eF32); break;
          case bin::instruction::eF64ReinterpretI64  : UnaryOperatorNoAdd(bin::value_type::eI64, bin::value_type::eF64); break;

          case bin::instruction::eI32Extend8S        :
          case bin::instruction::eI32Extend16S       :
            UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32);
            break;

          case bin::instruction::eI64Extend8S        :
          case bin::instruction::eI64Extend16S       :
          case bin::instruction::eI64Extend32S       :
            UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64);
            break;

          case bin::instruction::eRefNull   :
          {
            TypeStack.push(static_cast<bin::value_type>(ParseRaw<bin::reference_type>()));
            PassInstruction(bin::instruction::eRefNull);
            break;
          }

          case bin::instruction::eRefIsNull :
          {
            if (TypeStack.empty())
              throw compile_status::eNoOperandsForUnary;

            if (TypeStack.top() != bin::value_type::eFuncRef && TypeStack.top() != bin::value_type::eExternRef)
              throw compile_status::eInvalidOperandType;
            TypeStack.pop();
            TypeStack.push(bin::value_type::eI32);
            PassInstruction(bin::instruction::eRefIsNull);
            break;
          }

          case bin::instruction::eRefFunc   :
          {
            TypeStack.push(bin::value_type::eFuncRef);
            PassInstruction(bin::instruction::eRefFunc);
            PassU32(ParseSigned<32>());
            break;
          }

          case bin::instruction::eSystem :
          case bin::instruction::eVector :
            throw compile_status::eUnsupportedFeature;
          }
        }
        catch (compile_status Err)
        {
          return { .Status = Err };
        }
      }

      if (TypeStack.size() != TargetStackSize)
        return { .Status = compile_status::eStackNotEmpty };
      return { .Status = compile_status::eOk };

      // /* Validate function reutrn type and/or stack size */
      // if (auto Ty = Signature.ReturnType)
      // {
      //   if (TypeStack.empty() || TypeStack.top() != Ty)
      //     return compile_status::eWrongReturnValueType;
      // }
      // else
      //   if (!TypeStack.empty())
      //     return compile_status::eStackNotEmpty;
      // 
      // Functions[FunctionIndex] = Function;
      // return compile_status::eOk;
    } /* Block compilation function */
  }; /* End of 'CompileBlock' function */

  /* Just in time compilation function.
   * ARGUMENTS:
   *   - function to compile index:
   *       UINT32 FunctionIndex;
   * RETURNS:
   *   (jit_compile_status) Compilation status.
   */
  compile_status source_impl::CompileJIT( UINT32 FunctionIndex )
  {
    raw_function_data *const RawData = std::get_if<raw_function_data>(&Functions[FunctionIndex]);

    if (RawData == nullptr)
      return compile_status::eOk;

    std::stack<bin::value_type> TypeStack;

    auto &Signature = FunctionSignatures[RawData->SignatureIndex];

    binary_input_stream Stream {RawData->Instructions};

    compiled_function_data Function;

    std::vector<bin::value_type> LocalTypes;

    // Sizes of locals
    for (auto Type : Signature.ArgumentTypes)
      LocalTypes.push_back(Type);
    std::vector<UINT32> LocalSizes;                 // Sizes of arguments

    // Parse local batches
    {
      UINT32 LocalBatchCount = 0;
      WATAP_SET_OR_RETURN(LocalBatchCount, bin_util::ParseUint(Stream), compile_status::eLocalParsingError);
      while (LocalBatchCount--)
      {
        UINT32 BatchSize = 0;
        WATAP_SET_OR_RETURN(BatchSize, bin_util::ParseUint(Stream), compile_status::eLocalParsingError);

        bin::value_type ValueType = bin::value_type::eI32;
        WATAP_SET_OR_RETURN(ValueType, Stream.Get<bin::value_type>(), compile_status::eLocalParsingError);

        while (BatchSize--)
          LocalTypes.push_back(ValueType);
      }
    }

    for (auto LocalType : LocalTypes)
      Function.LocalSizes.push_back(static_cast<UINT32>(bin::GetValueTypeSize(LocalType)));
    Function.ArgumentCount = static_cast<UINT32>(Signature.ArgumentTypes.size());
    Function.ReturnSize = static_cast<UINT32>(Signature.ReturnType ? bin::GetValueTypeSize(*Signature.ReturnType) : 0);

    // Parse actual instructions

    const UINT8 *InstructionPointer = RawData->Instructions.data();
    const UINT8 *InstructionEnd = InstructionPointer + RawData->Instructions.size();

    auto PassInstruction = [&]( bin::instruction Instruction, UINT8 AdditionalData = 0 )
      {
        Function.Instructions.push_back(compiled_instruction {Instruction, AdditionalData});
      };

    auto PassU16 = [&]( UINT16 Constant )
      {
        Function.Instructions.push_back(compiled_instruction { .InstructionID = Constant });
      };

    auto PassU32 = [&]( UINT32 Constant )
      {
        Function.Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant      ) });
        Function.Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 16) });
      };

    auto PassU64 = [&]( UINT64 Constant )
      {
        Function.Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant      ) });
        Function.Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 16) });
        Function.Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 32) });
        Function.Instructions.push_back(compiled_instruction { .InstructionID = static_cast<UINT16>(Constant >> 48) });
      };

    auto CheckType = [&]( bin::value_type Type ) -> BOOL
      {
        return TypeStack.top() == Type;
      };

    auto UnaryOperator = [&]( bin::instruction Instruction, bin::value_type SrcType, bin::value_type DstType )
      {
        if (TypeStack.empty())
          throw compile_status::eNoOperandsForUnary;

        if (TypeStack.top() != SrcType)
          throw compile_status::eInvalidOperandType;
        TypeStack.pop();
        TypeStack.push(DstType);
        Function.Instructions.push_back(compiled_instruction {Instruction});
      };

    auto UnaryOperatorNoAdd = [&]( bin::value_type SrcType, bin::value_type DstType )
      {
        if (TypeStack.empty())
          throw compile_status::eNoOperandsForUnary;

        if (TypeStack.top() != SrcType)
          throw compile_status::eInvalidOperandType;

        TypeStack.pop();
        TypeStack.push(DstType);
      };

    auto BinaryOperator = [&]( bin::instruction Instruction, bin::value_type LhsType, bin::value_type RhsType, bin::value_type DstType )
      {
        if (TypeStack.size() < 2)
          throw compile_status::eNoOperandsForBinary;

        if (TypeStack.top() != LhsType)
          throw compile_status::eInvalidOperandType;
        TypeStack.pop();

        if (TypeStack.top() != RhsType)
          throw compile_status::eInvalidOperandType;
        TypeStack.pop();

        TypeStack.push(DstType);
        Function.Instructions.push_back(compiled_instruction {Instruction});
      };

    while (InstructionPointer < InstructionEnd)
    {
      bin::instruction Instruction = *reinterpret_cast<const bin::instruction *>(InstructionPointer);

      InstructionPointer += 1;

      try
      {
        switch (Instruction)
        {
        case bin::instruction::eUnreachable   :
        case bin::instruction::eNop           :
        case bin::instruction::eReturn        :
          PassInstruction(Instruction);
          break;

        case bin::instruction::eMemoryGrow    :
        case bin::instruction::eMemorySize    :
          PassInstruction(Instruction);
          TypeStack.push(bin::value_type::eI32);
          break;

        // TODO TC4 WASM
        case bin::instruction::eBlock         :
        case bin::instruction::eLoop          :
        case bin::instruction::eIf            :
        case bin::instruction::eElse          :
        case bin::instruction::eExpressionEnd :
        case bin::instruction::eBr            :
        case bin::instruction::eBrIf          :
        case bin::instruction::eBrTable       :
          throw compile_status::eUnsupportedFeature;

        // Match type stack with called function signature
        case bin::instruction::eCall          :
          {
            auto [FunctionIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            if (FunctionIndex >= Functions.size())
              throw compile_status::eInvalidFunctionIndex;

            auto &CallSignature = FunctionSignatures[FunctionSignatureIndices[FunctionIndex]];

            if (TypeStack.size() < CallSignature.ArgumentTypes.size())
              throw compile_status::eNoFunctionArguments;

            for (auto Arg : CallSignature.ArgumentTypes)
            {
              if (TypeStack.top() != Arg)
                throw compile_status::eInvalidFunctionArgumentsType;
              TypeStack.pop();
            }

            if (CallSignature.ReturnType)
              TypeStack.push(*CallSignature.ReturnType);
            PassInstruction(Instruction);
            PassU32(static_cast<UINT32>(FunctionIndex));
            break;
          }

        case bin::instruction::eCallIndirect  :
          throw compile_status::eUnsupportedFeature;

        case bin::instruction::eDrop          :
          {
            if (TypeStack.empty())
                throw compile_status::eNoOperandsForUnary;

            PassInstruction(bin::instruction::eDrop, static_cast<UINT8>(bin::GetValueTypeSize(TypeStack.top())));
            TypeStack.pop();
            break;
          }

        case bin::instruction::eSelect        :
          throw compile_status::eUnsupportedFeature;

        case bin::instruction::eSelectTyped   :
          throw compile_status::eUnsupportedFeature;

        case bin::instruction::eLocalGet  :
          {
            auto [LocalIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            if (LocalTypes.size() >= LocalIndex)
              throw compile_status::eInvalidLocalIndex;

            TypeStack.push(LocalTypes[LocalIndex]);

            PassInstruction(bin::instruction::eLocalGet);
            PassU16(static_cast<UINT16>(LocalIndex));
            break;
          }

        case bin::instruction::eLocalSet  :
        case bin::instruction::eLocalTee  :
          {
            auto [LocalIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            if (Function.LocalSizes.size() >= LocalIndex)
              throw compile_status::eInvalidLocalIndex;

            if (TypeStack.top() != LocalTypes[LocalIndex])
              throw compile_status::eInvalidOperandType;

            if (Instruction != bin::instruction::eLocalTee)
              TypeStack.pop();

            PassInstruction(bin::instruction::eLocalSet, static_cast<UINT8>(bin::GetValueTypeSize(LocalTypes[LocalIndex])));
            PassU16(static_cast<UINT16>(LocalIndex));
            break;
          }

        case bin::instruction::eGlobalGet ://         = 0x23, // Load global
        case bin::instruction::eGlobalSet ://         = 0x24, // Store global

        case bin::instruction::eTableGet : //          = 0x25, // Get value from table
        case bin::instruction::eTableSet : //          = 0x26, // Set value into table
          throw compile_status::eUnsupportedFeature;

        case bin::instruction::eI32Load    :
        case bin::instruction::eI64Load    :
        case bin::instruction::eF32Load    :
        case bin::instruction::eF64Load    :
        case bin::instruction::eI32Load8S  :
        case bin::instruction::eI32Load8U  :
        case bin::instruction::eI32Load16S :
        case bin::instruction::eI32Load16U :
        case bin::instruction::eI64Load8S  :
        case bin::instruction::eI64Load8U  :
        case bin::instruction::eI64Load16S :
        case bin::instruction::eI64Load16U :
        case bin::instruction::eI64Load32S :
        case bin::instruction::eI64Load32U :
          {
            auto [Ptr, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            bin::value_type StackType;

            switch (Instruction)
            {
            case bin::instruction::eI32Load    : StackType = bin::value_type::eI32; break;
            case bin::instruction::eI64Load    : StackType = bin::value_type::eI64; break;
            case bin::instruction::eF32Load    : StackType = bin::value_type::eF32; break;
            case bin::instruction::eF64Load    : StackType = bin::value_type::eF64; break;
            case bin::instruction::eI32Load8S  : StackType = bin::value_type::eI32; break;
            case bin::instruction::eI32Load8U  : StackType = bin::value_type::eI32; break;
            case bin::instruction::eI32Load16S : StackType = bin::value_type::eI32; break;
            case bin::instruction::eI32Load16U : StackType = bin::value_type::eI32; break;
            case bin::instruction::eI64Load8S  : StackType = bin::value_type::eI64; break;
            case bin::instruction::eI64Load8U  : StackType = bin::value_type::eI64; break;
            case bin::instruction::eI64Load16S : StackType = bin::value_type::eI64; break;
            case bin::instruction::eI64Load16U : StackType = bin::value_type::eI64; break;
            case bin::instruction::eI64Load32S : StackType = bin::value_type::eI64; break;
            case bin::instruction::eI64Load32U : StackType = bin::value_type::eI64; break;
            }

            TypeStack.push(StackType);

            PassInstruction(Instruction);
            PassU32(static_cast<UINT32>(Ptr));
            break; // = 0x2B, // Load F64 from variable
          }

        case bin::instruction::eI32Store   :
        case bin::instruction::eI64Store   :
        case bin::instruction::eF32Store   :
        case bin::instruction::eF64Store   :
        case bin::instruction::eI32Store8  :
        case bin::instruction::eI32Store16 :
        case bin::instruction::eI64Store8  :
        case bin::instruction::eI64Store16 :
        case bin::instruction::eI64Store32 :
          {
            auto [Ptr, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            bin::value_type RequiredType;
            switch (Instruction)
            {
            case bin::instruction::eI32Store   : RequiredType = bin::value_type::eI32; break;
            case bin::instruction::eI64Store   : RequiredType = bin::value_type::eI64; break;
            case bin::instruction::eF32Store   : RequiredType = bin::value_type::eF32; break;
            case bin::instruction::eF64Store   : RequiredType = bin::value_type::eF64; break;
            case bin::instruction::eI32Store8  : RequiredType = bin::value_type::eI32; break;
            case bin::instruction::eI32Store16 : RequiredType = bin::value_type::eI32; break;
            case bin::instruction::eI64Store8  : RequiredType = bin::value_type::eI64; break;
            case bin::instruction::eI64Store16 : RequiredType = bin::value_type::eI64; break;
            case bin::instruction::eI64Store32 : RequiredType = bin::value_type::eI64; break;
            }

            if (TypeStack.empty())
              throw compile_status::eNoOperandsForUnary;

            if (TypeStack.top() != RequiredType)
              throw compile_status::eInvalidOperandType;

            TypeStack.pop();
            PassInstruction(Instruction);
            break;
          }

        case bin::instruction::eI32Const          :
          {
            auto [Value, Offset] = leb128::DecodeSigned<32>(InstructionPointer);
            InstructionPointer += Offset;

            TypeStack.push(bin::value_type::eI32);
            PassInstruction(bin::instruction::eI32Const);
            PassU32(static_cast<UINT32>(Value));
            break;
          }

        case bin::instruction::eF32Const          :
          {
            TypeStack.push(bin::value_type::eF32);
            PassInstruction(bin::instruction::eF32Const);
            PassU32(*reinterpret_cast<const UINT32 *>(InstructionPointer));
            InstructionPointer += 4;
            break;
          }

        case bin::instruction::eI64Const          :
          {
            auto [Value, Offset] = leb128::DecodeSigned<64>(InstructionPointer);
            InstructionPointer += Offset;

            TypeStack.push(bin::value_type::eI32);
            PassInstruction(bin::instruction::eI64Const);
            PassU64(Value);
            break;
          }

        case bin::instruction::eF64Const          :
          {
            TypeStack.push(bin::value_type::eF64);
            PassInstruction(bin::instruction::eF64Const);
            PassU64(*reinterpret_cast<const UINT64 *>(InstructionPointer));
            InstructionPointer += 4;
            break;
          }

        case bin::instruction::eI32Eqz             :
          UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32);
          break;

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
          BinaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32, bin::value_type::eI32);
          break;

        case bin::instruction::eI64Eqz             :
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
          BinaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI32);
          break;

        case bin::instruction::eF32Eq              :
        case bin::instruction::eF32Ne              :
        case bin::instruction::eF32Lt              :
        case bin::instruction::eF32Gt              :
        case bin::instruction::eF32Le              :
        case bin::instruction::eF32Ge              :
          BinaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eI32);
          break;


        case bin::instruction::eF64Eq              :
        case bin::instruction::eF64Ne              :
        case bin::instruction::eF64Lt              :
        case bin::instruction::eF64Gt              :
        case bin::instruction::eF64Le              :
        case bin::instruction::eF64Ge              :
          BinaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eF64, bin::value_type::eI32);
          break;

        case bin::instruction::eI32Clz             :
        case bin::instruction::eI32Ctz             :
        case bin::instruction::eI32Popcnt          :
          UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32);
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
          BinaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32, bin::value_type::eI32);
          break;

        case bin::instruction::eI64Clz             :
        case bin::instruction::eI64Ctz             :
        case bin::instruction::eI64Popcnt          :
          UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64);
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
          BinaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64, bin::value_type::eI64);
          break;


        case bin::instruction::eF32Abs             :
        case bin::instruction::eF32Neg             :
        case bin::instruction::eF32Ceil            :
        case bin::instruction::eF32Floor           :
        case bin::instruction::eF32Trunc           :
        case bin::instruction::eF32Nearest         :
        case bin::instruction::eF32Sqrt            :
          UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32);
          break;

        case bin::instruction::eF32Add             :
        case bin::instruction::eF32Sub             :
        case bin::instruction::eF32Mul             :
        case bin::instruction::eF32Div             :
        case bin::instruction::eF32Min             :
        case bin::instruction::eF32Max             :
        case bin::instruction::eF32CopySign        :
          BinaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eF32);
          break;

        case bin::instruction::eF64Abs             :
        case bin::instruction::eF64Neg             :
        case bin::instruction::eF64Ceil            :
        case bin::instruction::eF64Floor           :
        case bin::instruction::eF64Trunc           :
        case bin::instruction::eF64Nearest         :
        case bin::instruction::eF64Sqrt            :
          UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32);
          break;

        case bin::instruction::eF64Add             :
        case bin::instruction::eF64Sub             :
        case bin::instruction::eF64Mul             :
        case bin::instruction::eF64Div             :
        case bin::instruction::eF64Min             :
        case bin::instruction::eF64Max             :
        case bin::instruction::eF64CopySign        :
          BinaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF32, bin::value_type::eF32);
          break;

        case bin::instruction::eI32WrapI64         :
          UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI32);
          break;

        case bin::instruction::eI32TruncF32S       :
        case bin::instruction::eI32TruncF32U       :
          UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eI32);
          break;

        case bin::instruction::eI32TruncF64S       :
        case bin::instruction::eI32TruncF64U       :
          UnaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eI32);
          break;

        case bin::instruction::eI64ExtendI32S      :
        case bin::instruction::eI64ExtendI32U      :
          UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI64);
          break;

        case bin::instruction::eI64TruncF32S       :
        case bin::instruction::eI64TruncF32U       :
          UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eI64);
          break;

        case bin::instruction::eI64TruncF64S       :
        case bin::instruction::eI64TruncF64U       :
          UnaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eI64);
          break;

        case bin::instruction::eF32ConvertI32S     :
        case bin::instruction::eF32ConvertI32U     :
          UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eF32);
          break;

        case bin::instruction::eF32ConvertI64S     :
        case bin::instruction::eF32ConvertI64U     :
          UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eF32);
          break;

        case bin::instruction::eF32DemoteF64       :
          UnaryOperator(Instruction, bin::value_type::eF64, bin::value_type::eF32);
          break;

        case bin::instruction::eF64ConvertI32S     :
        case bin::instruction::eF64ConvertI32U     :
          UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eF64);
          break;

        case bin::instruction::eF64ConvertI64S     :
        case bin::instruction::eF64ConvertI64U     :
          UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eF64);
          break;

        case bin::instruction::eF64PromoteF32      :
          UnaryOperator(Instruction, bin::value_type::eF32, bin::value_type::eF64);
          break;

        case bin::instruction::eI32ReinterpretF32  : UnaryOperatorNoAdd(bin::value_type::eF32, bin::value_type::eI32); break;
        case bin::instruction::eI64ReinterpretF64  : UnaryOperatorNoAdd(bin::value_type::eF64, bin::value_type::eI64); break;
        case bin::instruction::eF32ReinterpretI32  : UnaryOperatorNoAdd(bin::value_type::eI32, bin::value_type::eF32); break;
        case bin::instruction::eF64ReinterpretI64  : UnaryOperatorNoAdd(bin::value_type::eI64, bin::value_type::eF64); break;

        case bin::instruction::eI32Extend8S        :
        case bin::instruction::eI32Extend16S       :
          UnaryOperator(Instruction, bin::value_type::eI32, bin::value_type::eI32);
          break;

        case bin::instruction::eI64Extend8S        :
        case bin::instruction::eI64Extend16S       :
        case bin::instruction::eI64Extend32S       :
          UnaryOperator(Instruction, bin::value_type::eI64, bin::value_type::eI64);
          break;

        case bin::instruction::eRefNull   :
        {
          bin::reference_type RefType = *reinterpret_cast<const bin::reference_type *>(InstructionPointer);

          TypeStack.push(static_cast<bin::value_type>(RefType));
          PassInstruction(bin::instruction::eRefNull);
          break;
        }

        case bin::instruction::eRefIsNull :
        {
          if (TypeStack.empty())
            throw compile_status::eNoOperandsForUnary;

          if (TypeStack.top() != bin::value_type::eFuncRef && TypeStack.top() != bin::value_type::eExternRef)
            throw compile_status::eInvalidOperandType;
          TypeStack.pop();
          TypeStack.push(bin::value_type::eI32);
          PassInstruction(bin::instruction::eRefIsNull);
          break;
        }

        case bin::instruction::eRefFunc   :
        {
          auto [Value, Offset] = leb128::DecodeSigned<32>(InstructionPointer);
          InstructionPointer += Offset;

          TypeStack.push(bin::value_type::eFuncRef);
          PassInstruction(bin::instruction::eRefFunc);
          PassU32(static_cast<UINT32>(Value));
          break;
        }

        case bin::instruction::eSystem :
        case bin::instruction::eVector :
          throw compile_status::eUnsupportedFeature;
        }
      }
      catch (compile_status Err)
      {
        return Err;
      }
    }

    /* Validate stack size */
    if (TypeStack.size() > 1)
      return compile_status::eStackNotEmpty;

    /* Validate function reutrn type and/or stack size */
    if (auto Ty = Signature.ReturnType)
    {
      if (TypeStack.empty() || TypeStack.top() != Ty)
        return compile_status::eWrongReturnValueType;
    }
    else
      if (!TypeStack.empty())
        return compile_status::eStackNotEmpty;

    Functions[FunctionIndex] = Function;
    return compile_status::eOk;
  } /* End of 'CompileJIT' function */
} /* end of 'watap::impl::standard' namespace */

/* END OF 'watap_impl_standard_source.cpp' FILE */
