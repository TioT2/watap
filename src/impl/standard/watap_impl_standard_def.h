#ifndef __watap_impl_standard_def_h_
#define __watap_impl_standard_def_h_

#include "watap_impl_standard.h"

#ifndef WATAP_IMPL_STANDARD
#  error This file shouldn't be included in global project tree
#endif // defined(WATAP_IMPL_STANDARD)

/* Evaluation binary operator implementation generation macro */
#define WATAP_STANDARD_DEFINE_EXEC_OP_BINARY(TYPE, OP) { TYPE *Ptr = EvaluationStack.Pop<TYPE>(sizeof(TYPE));  Ptr[-1] = (Ptr[-1] OP *Ptr);     break; }

/* Evaluation binary function implementation generation macro */
#define WATAP_STANDARD_DEFINE_EXEC_FN_BINARY(TYPE, FN) { TYPE *Ptr = EvaluationStack.Pop<TYPE>(sizeof(TYPE));  Ptr[-1] = (FN(Ptr[-1], *Ptr));   break; }

/* Evaluation unary function implementation generation macro */
#define WATAP_STANDARD_DEFINE_EXEC_FN_UNARY(TYPE, FN)  { TYPE *Ptr = EvaluationStack.Get<TYPE>() - 1;         *Ptr = (FN(*Ptr));                break; }

/* WATAP_STANDARD_ROT_FUNCTION_WRAPPER macro wrapper */
#define __WATAP_STANDARD_ROT_FUNCTION_WRAPPER(LARG, RARG) (LARG, static_cast<INT>(RARG))

/* Wrapper, special for std::rot* functions don't emit warnings during WIN32 Compilation */
#define WATAP_STANDARD_ROT_FUNCTION_WRAPPER(FN) FN __WATAP_STANDARD_ROT_FUNCTION_WRAPPER

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

    import_info( VOID ) : ImportType(bin::import_export_type::eFunction), Function(0)
    {

    } /* End of 'import_info' structure */
  }; /* End of 'import_info' enumeration */

  /* Runtime instruction representation structure */
  struct instruction
  {
    union
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
    };
  }; /* End of 'instruction' structure */

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
  public:

    std::optional<std::string> StartName;                           // Start function name
    std::vector<function> Functions;                                // Function list
    std::map<std::string, UINT32, std::less<>> FunctionExportTable; // Exported function table

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
        std::terminate(); // TODO TC4 Somehow fix this

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

      // Push arguments into evaluation stack
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

          case bin::instruction::eBlock             : break;
          case bin::instruction::eLoop              : break;
          case bin::instruction::eIf                : break;
          case bin::instruction::eElse              : break;
          case bin::instruction::eExpressionEnd     : break;
          case bin::instruction::eBr                : break;
          case bin::instruction::eBrIf              : break;
          case bin::instruction::eReturn            :
          {
            // TODO TC4 Fix return
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
          case bin::instruction::eDrop              : break;
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
          case bin::instruction::eI32Load           : break;
          case bin::instruction::eI64Load           : break;
          case bin::instruction::eF32Load           : break;
          case bin::instruction::eF64Load           : break;
          case bin::instruction::eI32Load8S         : break;
          case bin::instruction::eI32Load8U         : break;
          case bin::instruction::eI32Load16S        : break;
          case bin::instruction::eI32Load16U        : break;
          case bin::instruction::eI64Load8S         : break;
          case bin::instruction::eI64Load8U         : break;
          case bin::instruction::eI64Load16S        : break;
          case bin::instruction::eI64Load16U        : break;
          case bin::instruction::eI64Load32S        : break;
          case bin::instruction::eI64Load32U        : break;
          case bin::instruction::eI32Store          : break;
          case bin::instruction::eI64Store          : break;
          case bin::instruction::eF32Store          : break;
          case bin::instruction::eF64Store          : break;
          case bin::instruction::eI32Store8         : break;
          case bin::instruction::eI32Store16        : break;
          case bin::instruction::eI64Store8         : break;
          case bin::instruction::eI64Store16        : break;
          case bin::instruction::eI64Store32        : break;
          case bin::instruction::eMemorySize        : break;
          case bin::instruction::eMemoryGrow        : break;

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

          case bin::instruction::eI32Clz            : break;
          case bin::instruction::eI32Ctz            : break;
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

          case bin::instruction::eI64Clz            : break;
          case bin::instruction::eI64Ctz            : break;
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

          case bin::instruction::eI32WrapI64        : break;
          case bin::instruction::eI32TruncF32S      : break;
          case bin::instruction::eI32TruncF32U      : break;
          case bin::instruction::eI32TruncF64S      : break;
          case bin::instruction::eI32TruncF64U      : break;
          case bin::instruction::eI64ExtendI32S     : break;
          case bin::instruction::eI64ExtendI32U     : break;
          case bin::instruction::eI64TruncF32S      : break;
          case bin::instruction::eI64TruncF32U      : break;
          case bin::instruction::eI64TruncF64S      : break;
          case bin::instruction::eI64TruncF64U      : break;
          case bin::instruction::eF32ConvertI32S    : break;
          case bin::instruction::eF32ConvertI32U    : break;
          case bin::instruction::eF32ConvertI64S    : break;
          case bin::instruction::eF32ConvertI64U    : break;
          case bin::instruction::eF32DemoteF64      : break;
          case bin::instruction::eF64ConvertI32S    : break;
          case bin::instruction::eF64ConvertI32U    : break;
          case bin::instruction::eF64ConvertI64S    : break;
          case bin::instruction::eF64ConvertI64U    : break;
          case bin::instruction::eF64PromoteF32     : break;

          case bin::instruction::eI32ReinterpretF32 : break; // That's all
          case bin::instruction::eI64ReinterpretF64 : break; // That's all
          case bin::instruction::eF32ReinterpretI32 : break; // That's all
          case bin::instruction::eF64ReinterpretI64 : break; // That's all

          case bin::instruction::eI32Extend8S       : break;
          case bin::instruction::eI32Extend16S      : break;
          case bin::instruction::eI64Extend8S       : break;
          case bin::instruction::eI64Extend16S      : break;
          case bin::instruction::eI64Extend32S      : break;

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
            Trap();
            break;
          case bin::instruction::eVector            :
            Trap(); // No SIMD Extension support
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