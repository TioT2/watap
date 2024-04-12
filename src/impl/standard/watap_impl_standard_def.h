#ifndef __watap_impl_standard_def_h_
#define __watap_impl_standard_def_h_

#include "watap_impl_standard.h"

#ifndef WATAP_IMPL_STANDARD
#  error This file shouldn't be included in global project tree
#endif // defined(WATAP_IMPL_STANDARD)

/* Evaluation binary operator implementation generation macro */
#define WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(TYPE, OP) { TYPE *Ptr = EvaluationStack.Pop<TYPE>(sizeof(TYPE)); Ptr[-1] = (Ptr[-1] OP *Ptr); break; }

/* Evaluation binary function implementation generation macro */
#define WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(TYPE, FN) { TYPE *Ptr = EvaluationStack.Pop<TYPE>(sizeof(TYPE)); Ptr[-1] = (FN(Ptr[-1], *Ptr)); break; }

/* Evaluation unary function implementation generation macro */
#define WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(TYPE, FN) { TYPE *Ptr = EvaluationStack.Get<TYPE>() - 1; *Ptr = (FN(*Ptr)); break; }

/* WATAP_STANDARD_ROT_FUNCTION_WRAPPER macro helper */
#define __WATAP_STANDARD_ROT_FUNCTION_WRAPPER(LARG, RARG) (LARG, static_cast<INT>(RARG))

/* Wrapper, special for std::rot* functions don't emit warnings during WIN32 Compilation */
#define WATAP_STANDARD_ROT_FUNCTION_WRAPPER(FN) FN __WATAP_STANDARD_ROT_FUNCTION_WRAPPER

/* Heap loading with builtin conversion implementaion function */
#define WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(TYPE, FROM)                                             \
{                                                                                                    \
  auto [Ptr, Offset] = leb128::DecodeUnsigned(InstructionPointer);                                   \
  InstructionPointer += Offset;                                                                      \
  EvaluationStack.Push<TYPE>(sizeof(TYPE))[-1] = *reinterpret_cast<const FROM *>(Heap.data() + Ptr); \
  break;                                                                                             \
}

/* Heap loading with builtin conversion implementaion function */
#define WATAP_STANDARD_DEFINE_HEAP_STORE_TO(TYPE, TO)                                                     \
{                                                                                                         \
  auto [Ptr, Offset] = leb128::DecodeUnsigned(InstructionPointer);                                        \
  InstructionPointer += Offset;                                                                           \
  *reinterpret_cast<TO *>(Heap.data() + Ptr) = static_cast<TO>(*EvaluationStack.Pop<TYPE>(sizeof(TYPE))); \
  break;                                                                                                  \
}

/* Integer extend generation macro definition */
#define WATAP_STANDARD_DEFINE_I_EXTEND(BASE, SUB) { BASE *P = EvaluationStack.Get<BASE>() - 1; *P = static_cast<BASE>(*reinterpret_cast<SUB *>(P)); break; }

/* Heap loading with builtin conversion implementaion function */
#define WATAP_STANDARD_DEFINE_CAST(FROM, TO)                                                             \
{                                                                                                        \
  FROM P = *EvaluationStack.Pop<FROM>(sizeof(FROM));                                                     \
  EvaluationStack.Push<TO>(sizeof(TO))[-1] = static_cast<TO>(P);                                         \
  break;                                                                                                 \
}

/* Project namespace // WASM Namespace // Implementation namesapce // Standard (multiplatform) implementation namespace */
namespace watap::impl::standard
{
  /* Import requirement draft description */
  struct import_info
  {
    std::string ModuleName;      // Imported value module name
    std::string ImportName;      // Imported value name
    bin::import_export_type ImportType; // Imported value type
    union
    {
      UINT32 Function;           // Imported function type description reference
      bin::table_type Table;     // Imported table type description
      bin::limits Memory;        // Imported memory range
      bin::global_type Variable; // Imported variable
    };

    /* Import info constructor */
    import_info( VOID ) : ImportType(bin::import_export_type::eFunction), Function(0)
    {

    } /* End of 'import_info' structure */
  }; /* End of 'import_info' enumeration */

  /* Runtime instruction representation structure */
  union compiled_instruction
  {
    struct
    {
      bin::instruction Instruction;                     // Instruction, actually

      union // Secondary instruction
      {
        UINT8                   InstructionPlaceholder; // Secondary instruction placeholder
        bin::vector_instruction VectorInstruction;      // Vector instruction
        bin::system_instruction SystemInstruction;      // System instruction
      };
    };
    UINT16 InstructionID = 0; // Unique instruction identifier
  }; /* End of 'compiled_instruction' structure */

  /* Compiled function data representation structure */
  struct compiled_function_data
  {
    UINT32 ReturnSize;                              // Size of value count as returned
    UINT32 ArgumentCount;                           // Count of locals
    std::vector<UINT32> LocalSizes;                 // Sizes of arguments
    std::vector<compiled_instruction> Instructions; // Instruciton set
  }; /* End of 'compiled_function_data' structure */

  /* Raw function data representation structure */
  struct raw_function_data
  {
    UINT32 SignatureIndex;           // Function signature identifier
    std::vector<UINT8> Instructions; // Raw instruction set
  }; /* End of 'raw_function_data' structure */

  using function_data = std::variant<raw_function_data, compiled_function_data>;

  /* Function represetnation class */
  struct function
  {
    std::vector<bin::value_type> Locals;       // Local variable set
    UINT32 ArgumentCount;                      // Count of locals, initially consumed from stack
    std::optional<bin::value_type> ReturnType; // Local variable set
    std::vector<UINT8> Instructions;           // Function instruction list
  }; /* End of 'function' class */

  /* WASM Module representation structure */
  class module_source_impl : public module_source
  {
  private:
    /* JIT Compilation status */
    enum class jit_compile_status
    {
      eOk,                           // Success
      eLocalParsingError,            // Error during local variable parsing
      eNoOperandsForUnary,           // No operands for unary operation
      eNoOperandsForBinary,          // No operands for binary operation
      eNoFunctionArguments,          // No values for function in stack
      eInvalidOperandType,           // Invalid operand type
      eInvalidLocalIndex,            // Invalid index of local variable
      eInvalidFunctionArgumentsType, // Invalid function argument type
      eInvalidFunctionIndex,         // Invalid function index
      eUnsupportedFeature,           // Unsupported feature (Vector operations, system instructions, etc.)
    }; /* End of 'jit_compile_status' enumeration */

    jit_compile_status CompileJIT( UINT32 FunctionIndex )
    {
      raw_function_data *const RawData = std::get_if<raw_function_data>(&Functions[FunctionIndex]);

      if (RawData == nullptr)
        return jit_compile_status::eOk;

      std::stack<bin::value_type> TypeStack;

      auto &Signature = FunctionSignatures[RawData->SignatureIndex];

      binary_input_stream Stream {RawData->Instructions};

      compiled_function_data Function;

      std::vector<bin::value_type> LocalTypes;

      // Sizes of locals
      for (auto Type : Signature.ArgumentTypes)
        LocalTypes.push_back(Type);

      auto ParseUint = []( binary_input_stream &Stream ) -> std::optional<UINT32>
      {
        auto [Value, Offset] = leb128::DecodeUnsigned(Stream.CurrentPtr());
        if (Stream.Get<UINT8>(Offset))
          return static_cast<UINT32>(Value);
        return std::nullopt;
      };

      // Parse local batches
      {
        UINT32 LocalBatchCount = 0;
        WATAP_SET_OR_RETURN(LocalBatchCount, ParseUint(Stream), jit_compile_status::eLocalParsingError);
        while (LocalBatchCount--)
        {
          UINT32 BatchSize = 0;
          WATAP_SET_OR_RETURN(BatchSize, ParseUint(Stream), jit_compile_status::eLocalParsingError);

          bin::value_type ValueType = bin::value_type::eI32;
          WATAP_SET_OR_RETURN(ValueType, Stream.Get<bin::value_type>(), jit_compile_status::eLocalParsingError);

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
            throw jit_compile_status::eNoOperandsForUnary;

          if (TypeStack.top() != SrcType)
            throw jit_compile_status::eInvalidOperandType;
          TypeStack.pop();
          TypeStack.push(DstType);
          Function.Instructions.push_back(compiled_instruction {Instruction});
        };

      auto UnaryOperatorNoAdd = [&]( bin::value_type SrcType, bin::value_type DstType )
        {
          if (TypeStack.empty())
            throw jit_compile_status::eNoOperandsForUnary;

          if (TypeStack.top() != SrcType)
            throw jit_compile_status::eInvalidOperandType;

          TypeStack.pop();
          TypeStack.push(DstType);
        };

      auto BinaryOperator = [&]( bin::instruction Instruction, bin::value_type LhsType, bin::value_type RhsType, bin::value_type DstType ) -> BOOL
        {
          if (TypeStack.size() < 2)
            throw jit_compile_status::eNoOperandsForBinary;

          if (TypeStack.top() != LhsType)
            throw jit_compile_status::eInvalidOperandType;
          TypeStack.pop();

          if (TypeStack.top() != RhsType)
            throw jit_compile_status::eInvalidOperandType;
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

          case bin::instruction::eBlock         : // TODO TC4 WASM
          case bin::instruction::eLoop          : // TODO TC4 WASM
          case bin::instruction::eIf            : // TODO TC4 WASM
          case bin::instruction::eElse          : // TODO TC4 WASM
          case bin::instruction::eExpressionEnd : // TODO TC4 WASM
          case bin::instruction::eBr            : // TODO TC4 WASM
          case bin::instruction::eBrIf          : // TODO TC4 WASM
          case bin::instruction::eBrTable       : // TODO TC4 WASM
            throw jit_compile_status::eUnsupportedFeature;

          // Match type stack with called function signature
          case bin::instruction::eCall          :
            {
              auto [FunctionIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
              InstructionPointer += Offset;

              if (FunctionIndex >= Functions.size())
                throw jit_compile_status::eInvalidFunctionIndex;

              auto &CallSignature = FunctionSignatures[FunctionSignatureIndices[FunctionIndex]];

              if (TypeStack.size() < CallSignature.ArgumentTypes.size())
                throw jit_compile_status::eNoFunctionArguments;

              for (auto Arg : CallSignature.ArgumentTypes)
              {
                if (TypeStack.top() != Arg)
                  throw jit_compile_status::eInvalidFunctionArgumentsType;
                TypeStack.pop();
              }

              if (CallSignature.ReturnType)
                TypeStack.push(*CallSignature.ReturnType);
              PassInstruction(Instruction);
              PassU32(static_cast<UINT32>(FunctionIndex));
              break;
            }

          case bin::instruction::eCallIndirect  :
            throw jit_compile_status::eUnsupportedFeature;

          case bin::instruction::eDrop          :
            {
              if (TypeStack.empty())
                  throw jit_compile_status::eNoOperandsForUnary;

              PassInstruction(bin::instruction::eDrop, static_cast<UINT8>(bin::GetValueTypeSize(TypeStack.top())));
              TypeStack.pop();
              break;
            }

          case bin::instruction::eSelect        :
            throw jit_compile_status::eUnsupportedFeature;

          case bin::instruction::eSelectTyped   :
            throw jit_compile_status::eUnsupportedFeature;

          case bin::instruction::eLocalGet  :
            {
              auto [LocalIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
              InstructionPointer += Offset;

              if (LocalTypes.size() >= LocalIndex)
                throw jit_compile_status::eInvalidLocalIndex;

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
                throw jit_compile_status::eInvalidLocalIndex;

              if (TypeStack.top() != LocalTypes[LocalIndex])
                throw jit_compile_status::eInvalidOperandType;

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
            throw jit_compile_status::eUnsupportedFeature;

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
                throw jit_compile_status::eNoOperandsForUnary;

              if (TypeStack.top() != RequiredType)
                throw jit_compile_status::eInvalidOperandType;

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
              throw jit_compile_status::eNoOperandsForUnary;

            if (TypeStack.top() != bin::value_type::eFuncRef && TypeStack.top() != bin::value_type::eExternRef)
              throw jit_compile_status::eInvalidOperandType;
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
            throw jit_compile_status::eUnsupportedFeature;
          }
        }
        catch (jit_compile_status Err)
        {
          return Err;
        }
      }

      Functions[FunctionIndex] = Function;
      return jit_compile_status::eOk;
    } /* End of 'CompileJIT' function */

  public:
    std::vector<function_signature> FunctionSignatures;             // Function signature list
    std::vector<UINT32> FunctionSignatureIndices;                   // Indices of function signatures

    std::optional<std::string> StartName;                           // Start function name (optional)
    std::vector<function_data> Functions;                           // Function list
    std::map<std::string, UINT32, std::less<>> FunctionExportTable; // Exported function table

    /* Function getting function...
     * ARGUMENTS:
     *   - funciton index:
     *       const UINT32 FunctionIndex;
     * RETURNS:
     *   (const compiled_function_data *) Function data.
     */
    const compiled_function_data * GetFunction( const UINT32 FunctionIndex ) const
    {
      if (FunctionIndex >= Functions.size())
        return nullptr;
      if (const compiled_function_data *FData = std::get_if<compiled_function_data>(&Functions[FunctionIndex]))
        return FData;
      if (const_cast<module_source_impl *>(this)->CompileJIT(FunctionIndex) != jit_compile_status::eOk)
        return nullptr;
      return &std::get<compiled_function_data>(Functions[FunctionIndex]);
    } /* End of 'GetFunction' function */

    /* Exported function by name getting function.
     * ARGUMENTS:
     *   - funciton index:
     *       const UINT32 FunctionIndex;
     * RETURNS:
     *   (const compiled_function_data *) Function data.
     */
    const compiled_function_data * GetExportFunction( const std::string_view Name ) const
    {
      if (auto Iter = FunctionExportTable.find(Name); Iter != FunctionExportTable.end())
        return GetFunction(Iter->second);
      else
        return nullptr;
    } /* End of 'GetExportFunction' function */

    /* Module data representation structure */
    module_source_impl( VOID )
    {

    } /* End of 'module_source' class */

    /* Start function name getting function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (std::optional<std::string_view>) Start function name, if it is defined.
     */
    std::optional<std::string_view> GetStartName( VOID ) const override
    {
      return StartName;
    } /* End of 'GetStartName' function */

  }; /* End of 'module_source' class */

  /* Stack, specially designed for local variables representation class */
  class local_stack
  {
    UINT8 *Begin = nullptr;   // Stack head
    UINT8 *Current = nullptr; // Current stack pointer
    UINT8 *End = nullptr;     // Stack end (for tracking stack size)

    /* Stack resize function.
     * ARGUMENTS:
     *   - stack size:
     *       SIZE_T Size;
     * RETURNS: None.
     */
    VOID Resize( SIZE_T Size ) noexcept
    {
      SIZE_T CurrentOff = Current - Begin;

      if ((Begin = reinterpret_cast<UINT8 *>(std::realloc(Begin, Size))) == nullptr)
        std::terminate(); // TODO TC4 WASM Somehow fix this

      Current = Begin + CurrentOff;
      End = Begin + Size;
    } /* End of 'ResizeStack' function */

  public:
    /* Local stack representation structure.
     * ARGUMETNS:
     *   - initial stack size:
     *       SIZE_T InitialSize = 1024;
     */
    local_stack( SIZE_T InitialSize = 1024 ) noexcept
    {
      Resize(InitialSize);
    } /* End of 'local_stack' structure */

    /* Frame to stack pushing function.
     * ARGUMENTS:
     *   - frame size:
     *       SIZE_T FrameSize;
     * RETURNS:
     *   (VOID *) Pointer to stack top with enough space below for previous FrameSize bytes not intersecting with another frame;
     */
    template <typename return_type = VOID>
      return_type * Push( SIZE_T FrameSize ) noexcept
      {
        if (Current + FrameSize > End)
          Resize((End - Begin + 1) * 2);
        return reinterpret_cast<return_type *>(Current += FrameSize);
      } /* End of 'Push' function */

    /* Frame from stack popping function.
     * ARGUMENTS:
     *   - frame size:
     *       SIZE_T FrameSize;
     * RETURNS:
     *   (VOID *) Pointer to previous framed;
     */
    template <typename return_type = VOID>
      return_type * Pop( SIZE_T FrameSize ) noexcept
      {
        return reinterpret_cast<return_type *>(Current -= FrameSize);
      } /* End of 'Pop' function */

    template <typename return_type = VOID>
      return_type * Get( VOID ) noexcept
      {
        return reinterpret_cast<return_type *>(Current);
      } /* End of 'Get' function */

    /* Stack size getting function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (SIZE_T) Current size of stack.
     */
    constexpr SIZE_T Size( VOID ) const noexcept
    {
      return Current - Begin;
    } /* End of 'Size' function */

    /* Stack pointer resetting function.
     * ARGUMENTS: None.
     * RETURNS: None.
     */
    VOID Drop( VOID ) noexcept
    {
      Current = Begin;
    } /* End of 'Drop' function */

    /* Local stack destructor. */
    ~local_stack( VOID )
    {
      // Clear stack
      std::free(Begin);
    } /* End of '~local_stack' function */
  }; /* End of 'local_stack' structure */

  /* Runtime implementation */
  class runtime_impl : public runtime
  {
    const module_source_impl &Source; // Module source

    /* Inner call representation structure */
    struct call
    {
      UINT32 FunctionIndex;       // Function index
      SIZE_T InstructionIndex;    // Instruction index
      SIZE_T LocalStackFrameSize; // Local stack frame size
    }; /* End of 'call' structure */

    local_stack LocalStack;        // Stack of local variables / function parameters
    local_stack EvaluationStack;   // Stack of evaluation
    std::vector<UINT8> Heap;       // Heap
    std::stack<call> CallStack;    // Call stack, holds pointers to functions
    BOOL Trapped = FALSE;          // Trapped

    /* Evaluation terminate function.
     * ARGUMENTS: None.
     * RETURNS: None.
     */
    VOID Trap( VOID )
    {
      Trapped = TRUE;
      LocalStack.Drop();
      EvaluationStack.Drop();
      CallStack = std::stack<call>();
    } /* End of 'Trap' function */

  public:

    runtime_impl( const module_source_impl &Source ) : Source(Source)
    {
      Heap.resize(65536);
    } /* End of 'runtime_impl' class */

    /* Module function calling function.
     * ARGUMENTS:
     *   - function name:
     *       std::string_view FunctionName;
     *   - function parameter list:
     *       std::span<const value> Parameters;
     * RETURNS:
     *   (std::optional<value>) Return value if called, std::nullopt otherwise;
     */
    std::optional<value> Call( std::string_view FunctionName, std::span<const value> Parameters = {} )
    {
      auto *RootFnPtr = Source.GetExportFunction(FunctionName);
      if (RootFnPtr == nullptr)
        return std::nullopt;
      const compiled_function_data &RootFn = *RootFnPtr;

      return std::nullopt;
    } /* End of 'Call' function */

    #if FALSE
    /* Module function calling function.
     * ARGUMENTS:
     *   - function name:
     *       std::string_view FunctionName;
     *   - function parameter list:
     *       std::span<const value> Parameters;
     * RETURNS:
     *   (std::optional<value>) Return value if called, std::nullopt otherwise;
     */
    std::optional<value> Call( std::string_view FunctionName, std::span<const value> Parameters = {} )
    {
      UINT32 RootFunctionIndex;
      if (auto FIter = Source.FunctionExportTable.find(FunctionName); FIter != Source.FunctionExportTable.end())
        RootFunctionIndex = FIter->second;
      else
        return std::nullopt;
      auto &RootFunction = Source.Functions[RootFunctionIndex];

      // Initialize callstack
      CallStack.push(call {
        .FunctionIndex = RootFunctionIndex,
        .InstructionIndex = 0,
        .LocalStackFrameSize = RootFunction.Locals.size() * sizeof(UINT64),
      });

      // Push initial arguments into evaluation stack (for them being popped from during first function start)
      for (UINT32 i = 0; i < RootFunction.ArgumentCount; i++)
      {
        SIZE_T Size = bin::GetValueTypeSize(RootFunction.Locals[i]);
        UINT8 *Ptr = EvaluationStack.Push<UINT8>(Size);
        std::memcpy(Ptr - Size, &Parameters[i], Size);
      }

      // Start function execution
      while (!CallStack.empty())
      {
        call &Call = CallStack.top();

        if (Call.InstructionIndex >= Source.Functions[Call.FunctionIndex].Instructions.size())
        {
          CallStack.pop();
          continue;
        }

        const auto &Function = Source.Functions[Call.FunctionIndex];
        
        // Decode instructions
        auto &Instructions = Function.Instructions;
        const UINT8 *InstructionPointer = Instructions.data() + Call.InstructionIndex;
        const UINT8 *const InstructionsEnd = Instructions.data() + Instructions.size();

        UINT64 *StackFrame = reinterpret_cast<UINT64 *>(LocalStack.Push(Call.LocalStackFrameSize));
        // Load bytes from stack

        // Write function argumetns if function just started
        if (Call.InstructionIndex == 0)
          for (INT i = 0; i < (INT)Function.ArgumentCount; i++)
            StackFrame[-1 - i] = *reinterpret_cast<UINT64 *>(EvaluationStack.Pop(bin::GetValueTypeSize(Function.Locals[i])));

        BOOL Continue = TRUE;
        while (Continue && InstructionPointer < InstructionsEnd)
        {
          bin::instruction Instruction = static_cast<bin::instruction>(*InstructionPointer++);

          switch (Instruction)
          {
          case bin::instruction::eUnreachable       :
            Trap();
            Continue = FALSE;
            break;

          case bin::instruction::eNop               :
            break;

          case bin::instruction::eBlock             :
          {
            break;
          }

          case bin::instruction::eLoop              : break;
          case bin::instruction::eIf                :
          {

            break;
          }
          case bin::instruction::eElse              : break;
          case bin::instruction::eExpressionEnd     : break;
          case bin::instruction::eBr                : break;
          case bin::instruction::eBrIf              : break;
          case bin::instruction::eReturn            :
          {
            // TODO TC4 WASM Fix return
            Continue = FALSE;
            break;
          }
          case bin::instruction::eBrTable           : break;

          case bin::instruction::eCall              :
          {
            auto [FunctionIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            CallStack.push(call {
              .FunctionIndex = (UINT32)FunctionIndex,
              .InstructionIndex = 0,
              .LocalStackFrameSize = Source.Functions[FunctionIndex].Locals.size() * sizeof(UINT64),
            });
            Continue = FALSE;
            break;
          }

          case bin::instruction::eCallIndirect      : break;

          case bin::instruction::eDrop              : break; // TODO TC4 WASM Implement type context.
          case bin::instruction::eSelect            : break;
          case bin::instruction::eSelectTyped       : break;

          case bin::instruction::eLocalGet          :
          {
            auto [LocalIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            SIZE_T LocalSize = bin::GetValueTypeSize(Function.Locals[LocalIndex]);
            UINT8 *P = EvaluationStack.Push<UINT8>(LocalSize);
            std::memcpy(P - LocalSize, StackFrame - LocalIndex - 1, LocalSize);
            break;
          }

          case bin::instruction::eLocalSet          :
          {
            auto [LocalIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            SIZE_T LocalSize = bin::GetValueTypeSize(Function.Locals[LocalIndex]);
            UINT8 *P = EvaluationStack.Pop<UINT8>(LocalSize);
            std::memcpy(StackFrame - LocalIndex - 1, P, LocalSize);
            break;
          }

          case bin::instruction::eLocalTee          :
          {
            auto [LocalIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;

            SIZE_T LocalSize = bin::GetValueTypeSize(Function.Locals[LocalIndex]);
            UINT8 *P = EvaluationStack.Get<UINT8>() - LocalSize;
            std::memcpy(StackFrame - LocalIndex - 1, P, LocalSize);
            break;
          }

          case bin::instruction::eGlobalGet         : break;
          case bin::instruction::eGlobalSet         : break;
          case bin::instruction::eTableGet          : break;
          case bin::instruction::eTableSet          : break;

          case bin::instruction::eI32Load           : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM( UINT32,  UINT32)
          case bin::instruction::eI64Load           : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM( UINT64,  UINT64)
          case bin::instruction::eF32Load           : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(FLOAT32, FLOAT32)
          case bin::instruction::eF64Load           : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(FLOAT64, FLOAT64)

          case bin::instruction::eI32Load8S         : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT32,  INT8 )
          case bin::instruction::eI32Load8U         : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT32, UINT8 )
          case bin::instruction::eI32Load16S        : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT32,  INT16)
          case bin::instruction::eI32Load16U        : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT32, UINT16)

          case bin::instruction::eI64Load8S         : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT64,  INT8 )
          case bin::instruction::eI64Load8U         : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT64, UINT8 )
          case bin::instruction::eI64Load16S        : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT64,  INT16)
          case bin::instruction::eI64Load16U        : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT64, UINT16)
          case bin::instruction::eI64Load32S        : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT64,  INT32)
          case bin::instruction::eI64Load32U        : WATAP_STANDARD_DEFINE_HEAP_LOAD_FROM(UINT64, UINT32)

          case bin::instruction::eI32Store          :
          case bin::instruction::eF32Store          :
             WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT32, UINT32)

          case bin::instruction::eI64Store          :
          case bin::instruction::eF64Store          :
             WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT64, UINT64)

          case bin::instruction::eI32Store8         : WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT32, UINT8 )
          case bin::instruction::eI32Store16        : WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT32, UINT16)
          case bin::instruction::eI64Store8         : WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT64, UINT8 )
          case bin::instruction::eI64Store16        : WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT64, UINT16)
          case bin::instruction::eI64Store32        : WATAP_STANDARD_DEFINE_HEAP_STORE_TO(UINT64, UINT32)

          case bin::instruction::eMemorySize        :
          {
            EvaluationStack.Push<UINT32>(4)[-1] = static_cast<UINT32>(Heap.size());
            break;
          }

          case bin::instruction::eMemoryGrow        :
          {
            Heap.resize(Heap.size() * 2, 0x00);
            break;
          }

          case bin::instruction::eI32Const          :
          {
            auto [Value, Offset] = leb128::DecodeSigned<32>(InstructionPointer);
            InstructionPointer += Offset;
            EvaluationStack.Push<UINT32>(4)[-1] = (UINT32)Value;
            break;
          }

          case bin::instruction::eF32Const          :
          {
            EvaluationStack.Push<FLOAT>(4)[-1] = *reinterpret_cast<const FLOAT *>(InstructionPointer);
            InstructionPointer += 4;
            break;
          }

          case bin::instruction::eI64Const          :
          {
            auto [Value, Offset] = leb128::DecodeSigned<64>(InstructionPointer);
            InstructionPointer += Offset;
            EvaluationStack.Push<UINT64>(4)[-1] = Value;
            break;
          }

          case bin::instruction::eF64Const          :
          {
            EvaluationStack.Push<FLOAT>(8)[-1] = *reinterpret_cast<const FLOAT *>(InstructionPointer);
            InstructionPointer += 8;
            break;
          }

          case bin::instruction::eI32Eqz            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(  INT32, 0 ==)
          case bin::instruction::eI32Eq             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, ==)
          case bin::instruction::eI32Ne             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, !=)
          case bin::instruction::eI32LtS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, <)
          case bin::instruction::eI32LtU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, <)
          case bin::instruction::eI32GtS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, >)
          case bin::instruction::eI32GtU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, >)
          case bin::instruction::eI32LeS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, <=)
          case bin::instruction::eI32LeU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, <=)
          case bin::instruction::eI32GeS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, >=)
          case bin::instruction::eI32GeU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, >=)

          case bin::instruction::eI64Eqz            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(  INT64, 0 ==)
          case bin::instruction::eI64Eq             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, ==)
          case bin::instruction::eI64Ne             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, !=)
          case bin::instruction::eI64LtS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, <)
          case bin::instruction::eI64LtU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, <)
          case bin::instruction::eI64GtS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, >)
          case bin::instruction::eI64GtU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, >)
          case bin::instruction::eI64LeS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, <=)
          case bin::instruction::eI64LeU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, <=)
          case bin::instruction::eI64GeS            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, >=)
          case bin::instruction::eI64GeU            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, >=)

          case bin::instruction::eF32Eq             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, ==)
          case bin::instruction::eF32Ne             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, !=)
          case bin::instruction::eF32Lt             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, <)
          case bin::instruction::eF32Gt             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, >)
          case bin::instruction::eF32Le             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, <=)
          case bin::instruction::eF32Ge             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, >=)
          case bin::instruction::eF64Eq             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, ==)
          case bin::instruction::eF64Ne             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, !=)
          case bin::instruction::eF64Lt             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, <)
          case bin::instruction::eF64Gt             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, >)
          case bin::instruction::eF64Le             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, <=)
          case bin::instruction::eF64Ge             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, >=)

          case bin::instruction::eI32Clz            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(UINT32, std::countl_zero<UINT32>)
          case bin::instruction::eI32Ctz            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(UINT32, std::countr_zero<UINT32>)
          case bin::instruction::eI32Popcnt         : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(UINT32, std::popcount<UINT32>)

          case bin::instruction::eI32Add            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, +)
          case bin::instruction::eI32Sub            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, -)
          case bin::instruction::eI32Mul            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, *)
          case bin::instruction::eI32DivS           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, /)
          case bin::instruction::eI32DivU           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, /)
          case bin::instruction::eI32RemS           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, %)
          case bin::instruction::eI32RemU           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, %)
          case bin::instruction::eI32And            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, &)
          case bin::instruction::eI32Or             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, |)
          case bin::instruction::eI32Xor            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, ^)
          case bin::instruction::eI32Shl            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, <<)
          case bin::instruction::eI32ShrS           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT32, >>)
          case bin::instruction::eI32ShrU           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT32, >>)
          case bin::instruction::eI32Rotl           : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(UINT32, WATAP_STANDARD_ROT_FUNCTION_WRAPPER(std::rotl))
          case bin::instruction::eI32Rotr           : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(UINT32, WATAP_STANDARD_ROT_FUNCTION_WRAPPER(std::rotr))

          case bin::instruction::eI64Clz            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(UINT64, std::countl_zero<UINT64>)
          case bin::instruction::eI64Ctz            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(UINT64, std::countr_zero<UINT64>)
          case bin::instruction::eI64Popcnt         : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(UINT64, std::popcount<UINT64>)

          case bin::instruction::eI64Add            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, +)
          case bin::instruction::eI64Sub            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, -)
          case bin::instruction::eI64Mul            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, *)
          case bin::instruction::eI64DivS           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, /)
          case bin::instruction::eI64DivU           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, /)
          case bin::instruction::eI64RemS           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, %)
          case bin::instruction::eI64RemU           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, %)
          case bin::instruction::eI64And            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, &)
          case bin::instruction::eI64Or             : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, |)
          case bin::instruction::eI64Xor            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, ^)
          case bin::instruction::eI64Shl            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, <<)
          case bin::instruction::eI64ShrS           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY( INT64, >>)
          case bin::instruction::eI64ShrU           : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(UINT64, >>)
          case bin::instruction::eI64Rotl           : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(UINT64, WATAP_STANDARD_ROT_FUNCTION_WRAPPER(std::rotl))
          case bin::instruction::eI64Rotr           : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(UINT64, WATAP_STANDARD_ROT_FUNCTION_WRAPPER(std::rotr))

          case bin::instruction::eF32Abs            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, std::abs)
          case bin::instruction::eF32Neg            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, -)
          case bin::instruction::eF32Ceil           : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, std::ceil)
          case bin::instruction::eF32Floor          : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, std::floor)
          case bin::instruction::eF32Trunc          : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, std::trunc)
          case bin::instruction::eF32Nearest        : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, std::round)
          case bin::instruction::eF32Sqrt           : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT32, std::sqrt)
          case bin::instruction::eF32Add            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, +)
          case bin::instruction::eF32Sub            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, -)
          case bin::instruction::eF32Mul            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, *)
          case bin::instruction::eF32Div            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT32, /)
          case bin::instruction::eF32CopySign       : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(FLOAT32, std::copysign)
          case bin::instruction::eF32Min            : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(FLOAT32, std::min)
          case bin::instruction::eF32Max            : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(FLOAT32, std::max)

          case bin::instruction::eF64Abs            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, std::abs)
          case bin::instruction::eF64Neg            : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, -)
          case bin::instruction::eF64Ceil           : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, std::ceil)
          case bin::instruction::eF64Floor          : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, std::floor)
          case bin::instruction::eF64Trunc          : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, std::trunc)
          case bin::instruction::eF64Nearest        : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, std::round)
          case bin::instruction::eF64Sqrt           : WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(FLOAT64, std::sqrt)
          case bin::instruction::eF64Add            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, +)
          case bin::instruction::eF64Sub            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, -)
          case bin::instruction::eF64Mul            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, *)
          case bin::instruction::eF64Div            : WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(FLOAT64, /)
          case bin::instruction::eF64Min            : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(FLOAT64, std::copysign)
          case bin::instruction::eF64Max            : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(FLOAT64, std::min)
          case bin::instruction::eF64CopySign       : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(FLOAT64, std::max)

          case bin::instruction::eI32WrapI64        : WATAP_STANDARD_DEFINE_CAST( UINT32,  UINT64)

          case bin::instruction::eI32TruncF32S      : WATAP_STANDARD_DEFINE_CAST(FLOAT32,   INT32)
          case bin::instruction::eI32TruncF32U      : WATAP_STANDARD_DEFINE_CAST(FLOAT32,  UINT32)
          case bin::instruction::eI32TruncF64S      : WATAP_STANDARD_DEFINE_CAST(FLOAT64,   INT32)
          case bin::instruction::eI32TruncF64U      : WATAP_STANDARD_DEFINE_CAST(FLOAT64,  UINT32)

          case bin::instruction::eI64ExtendI32S     : WATAP_STANDARD_DEFINE_CAST(  INT32,   INT64)
          case bin::instruction::eI64ExtendI32U     : WATAP_STANDARD_DEFINE_CAST( UINT32,  UINT64)
          case bin::instruction::eI64TruncF32S      : WATAP_STANDARD_DEFINE_CAST(FLOAT32,   INT64)
          case bin::instruction::eI64TruncF32U      : WATAP_STANDARD_DEFINE_CAST(FLOAT32,  UINT64)
          case bin::instruction::eI64TruncF64S      : WATAP_STANDARD_DEFINE_CAST(FLOAT64,   INT64)
          case bin::instruction::eI64TruncF64U      : WATAP_STANDARD_DEFINE_CAST(FLOAT64,  UINT64)

          case bin::instruction::eF32ConvertI32S    : WATAP_STANDARD_DEFINE_CAST(  INT32, FLOAT32)
          case bin::instruction::eF32ConvertI32U    : WATAP_STANDARD_DEFINE_CAST( UINT32, FLOAT32)
          case bin::instruction::eF32ConvertI64S    : WATAP_STANDARD_DEFINE_CAST(  INT64, FLOAT32)
          case bin::instruction::eF32ConvertI64U    : WATAP_STANDARD_DEFINE_CAST( UINT64, FLOAT32)
          case bin::instruction::eF32DemoteF64      : WATAP_STANDARD_DEFINE_CAST(FLOAT64, FLOAT32)

          case bin::instruction::eF64ConvertI32S    : WATAP_STANDARD_DEFINE_CAST(  INT32, FLOAT64)
          case bin::instruction::eF64ConvertI32U    : WATAP_STANDARD_DEFINE_CAST( UINT32, FLOAT64)
          case bin::instruction::eF64ConvertI64S    : WATAP_STANDARD_DEFINE_CAST(  INT64, FLOAT64)
          case bin::instruction::eF64ConvertI64U    : WATAP_STANDARD_DEFINE_CAST( UINT64, FLOAT64)
          case bin::instruction::eF64PromoteF32     : WATAP_STANDARD_DEFINE_CAST(FLOAT32, FLOAT64)

          case bin::instruction::eI32ReinterpretF32 : break; // That's all
          case bin::instruction::eI64ReinterpretF64 : break; // That's all
          case bin::instruction::eF32ReinterpretI32 : break; // That's all
          case bin::instruction::eF64ReinterpretI64 : break; // That's all

          case bin::instruction::eI32Extend8S       : WATAP_STANDARD_DEFINE_I_EXTEND(INT32, INT8 )
          case bin::instruction::eI32Extend16S      : WATAP_STANDARD_DEFINE_I_EXTEND(INT32, INT16)
          case bin::instruction::eI64Extend8S       : WATAP_STANDARD_DEFINE_I_EXTEND(INT64, INT8 )
          case bin::instruction::eI64Extend16S      : WATAP_STANDARD_DEFINE_I_EXTEND(INT64, INT16)
          case bin::instruction::eI64Extend32S      : WATAP_STANDARD_DEFINE_I_EXTEND(INT64, INT32)

          case bin::instruction::eRefNull           :
            *EvaluationStack.Push<UINT32>(4) = 0;
            break;

          case bin::instruction::eRefIsNull         : WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(UINT32, 0 ==)

          case bin::instruction::eRefFunc           :
          {
            auto [FunctionIndex, Offset] = leb128::DecodeUnsigned(InstructionPointer);
            InstructionPointer += Offset;
            *EvaluationStack.Push<UINT32>(4) = FunctionIndex;
            break;
          }

          case bin::instruction::eSystem            :
            Trap(); // TODO TC4 WASM
            break;

          case bin::instruction::eVector            :
            Trap(); // TODO TC4 WASM SIMD Add support
            break;

          default                                   : break;
          }
        }

        Call.InstructionIndex = InstructionPointer - Instructions.data();
      }

      if (Trapped || !RootFunction.ReturnType)
        return std::nullopt;

      SIZE_T Size = bin::GetValueTypeSize(*RootFunction.ReturnType);
      value Result { .F64x2 {0, 0} };
      std::memcpy(&Result, EvaluationStack.Pop(Size), Size);
      return Result;
    } /* End of 'Call' function */
    #endif

    /* Global value getting function.
     * ARGUMENTS:
     *   - global value name:
     *       std::string_view GlobalName;
     * RETURNS:
     *   (VOID *) Pointer to runtime memory that corresponds to WasmPtr;
     */
    BOOL SetGlobal( std::string_view GlobalName, value Value )
    {
      return FALSE;
    } /* End of 'SetGlobal' function */

    /* Global value getting function.
     * ARGUMENTS:
     *   - global value name:
     *       std::string_view GlobalName;
     * RETURNS:
     *   (VOID *) Pointer to runtime memory that corresponds to WasmPtr;
     */
    std::optional<value> GetGlobal( std::string_view GlobalName ) const
    {
      return std::nullopt;
    } /* End of 'GetGlobal' function */

    /* Module pointer dereferencing function.
     * ARGUMENTS:
     *   - module ptr:
     *       UINT32 WasmPtr;
     * RETURNS:
     *   (VOID *) Pointer to runtime memory that corresponds to WasmPtr;
     */
    VOID * GetPtr( UINT32 WasmPtr )
    {
      return Heap.data() + WasmPtr;
    } /* End of 'GetPtr' function */

    /* Is module trapped, trap requires module full restart.
     * ARGUMENTS: None.
     * RETURNS:
     *   (BOOL) TRUE if runtime is trapped, FALSE otherwise;
     */
    BOOL IsTrapped( VOID ) const
    {
      return Trapped;
    } /* End of 'IsTrapped' function */

    /* Module restart function.
     * ARGUMENTS: None.
     * RETURNS: None.
     */
    VOID Restart( VOID )
    {
      if (!Trapped)
        Trap();
      Trapped = FALSE;
    } /* End of 'Restart' function */
  }; /* End of 'runtime_impl' class */
} /* end of 'watap::impl::standard' namespace */

#endif // !defined(__watap_impl_standard_def_h_)

/* END OF 'watap_impl_standard_def.h' FILE */
