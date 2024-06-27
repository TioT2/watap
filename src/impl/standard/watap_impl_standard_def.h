#ifndef __watap_impl_standard_def_h_
#define __watap_impl_standard_def_h_

#include "watap_impl_standard.h"

#ifndef WATAP_IMPL_STANDARD
#  error This file shouldn't be included in global project tree
#endif // defined(WATAP_IMPL_STANDARD)

/* Project namespace // WASM Namespace // Implementation namesapce // Standard (multiplatform) implementation namespace */
namespace watap::impl::standard
{
  /* Import table required function set */
  struct import_element
  {
    bin::import_export_type Type; // Import requirement type
    union
    {
      struct { UINT32 TypeIndex; }   Function; // Function type
      bin::table_type                Table;    // Table type
      struct { bin::limits Limits; } Memory;   // Memory type
      bin::global_type               Global;   // Global type
    };

    /* Import requirements constructor. */
    import_element( VOID ) : Type(bin::import_export_type::eFunction), Function {~0U}
    {
    } /* End of 'import_element' function */
  }; /* End of 'import_element' structure */

  /* Export element representation structure */
  struct export_element
  {
    bin::import_export_type Type; // Import/Export type
    UINT32 Index;                 // Element index
  }; /* End of 'export_element' structure */

  /* Import object name representation structure */
  struct import_name
  {
    std::string ModuleName; // Import module name
    std::string Name;       // Imoprt name

    /* Import name comparison operator.
     * ARGUMENTS:
     *   - another import name:
     *       const import_name &Rhs;
     * RETURNS:
     *   (std::strong_ordering) Operator ordering
     */
    constexpr std::strong_ordering operator<=>( const import_name &Rhs ) const noexcept
    {
      if (auto Comp = ModuleName <=> Rhs.ModuleName; Comp != std::strong_ordering::equal)
        return Comp;
      return Name <=> Rhs.Name;
    } /* End of 'operator<=> function */
  }; /* End of 'import_name' structure */

  /* Runtime instruction representation structure */
  union compiled_instruction
  {
    struct
    {
      bin::instruction Instruction;                // Instruction, actually

      union // Secondary instruction
      {
        UINT8                   InstructionData;   // Secondary instruction placeholder (or data)
        bin::vector_instruction VectorInstruction; // Vector instruction
        bin::system_instruction SystemInstruction; // System instruction
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

  /* Function represetnation class */
  struct function
  {
    std::vector<bin::value_type> Locals;       // Local variable set
    UINT32 ArgumentCount;                      // Count of locals, initially consumed from stack
    std::optional<bin::value_type> ReturnType; // Local variable set
    std::vector<UINT8> Instructions;           // Function instruction list
  }; /* End of 'function' class */

  /* Compilation status */
  enum class compile_status
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
    eInvalidFunctionTypeIndex,     // Invalid function type index
    eStackNotEmpty,                // Stack isn't empty at end of execution
    eWrongReturnValueType,         // Wrong type of value, that this function returns
    eUnsupportedFeature,           // Unsupported feature (Vector operations, system instructions, etc.)
  }; /* End of 'compile_status' enumeration */

  /* WASM Module representation structure */
  class source_impl : public source
  {
  private:

    /* Expression compilation function.
     * ARGUMENTS:
     *   - code span:
     *       std::span<const UINT8> CodeSpan;
     * RETURNS:
     *   (jit_compile_status) Expression compilation status.
     */
    compile_status CompileExpression( std::span<const UINT8> CodeSpan )
    {
      return compile_status::eUnsupportedFeature;
    } /* End of 'CompileExpression' function */


    /* Just In Time compilation function.
     * ARGUMENTS:
     *   - function to compile index:
     *       UINT32 FunctionIndex;
     * RETURNS:
     *   (jit_compile_status) Compilation status.
     */
    compile_status CompileJIT( UINT32 FunctionIndex );

  public:

    std::map<import_name, import_element> Imports;              // Required import set
    std::map<std::string, export_element, std::less<>> Exports; // Export set
    std::optional<std::string>            Start;                // Start function name (optional)

    std::vector<function_signature> FunctionSignatures; // Function signature list
    std::vector<bin::table_type> Tables;                // Table set
    std::vector<UINT32> FunctionSignatureIndices;       // Indices of function signatures

    std::vector<std::variant<raw_function_data, compiled_function_data>> Functions; // Function lists

    /* Source implementation constructor */
    source_impl( VOID )
    {

    } /* End of 'module_source' class */

    /* Function getting function.
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
      if (const_cast<source_impl *>(this)->CompileJIT(FunctionIndex) != jit_compile_status::eOk) // uugh
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
      if (auto Iter = Exports.find(Name); Iter != Exports.end() && Iter->second.Type == bin::import_export_type::eFunction)
        return GetFunction(Iter->second.Index);
      return nullptr;
    } /* End of 'GetExportFunction' function */

    /* Start function name getting function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (std::optional<std::string_view>) Start function name in case it is defined.
     */
    std::optional<std::string_view> GetStartName( VOID ) const override
    {
      return Start;
    } /* End of 'GetStartName' function */
  }; /* End of 'source' class */

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

  /* Instance implementation function */
  class instance_impl : public instance
  {
    const source_impl &Source; // Reference to module source

    /* Inner call representation structure */
    struct call
    {
      UINT32 FunctionIndex;       // Function index
      SIZE_T InstructionIndex;    // Instruction index
      SIZE_T LocalStackFrameSize; // Local stack frame size
    }; /* End of 'call' structure */

    local_stack LocalStack;        // Stack of local variables / function parameters
    local_stack EvaluationStack;   // Stack of evaluation
    std::vector<UINT8> Heap;       // Memory heap
    std::stack<call> CallStack;    // Call stack, holds pointers to functions
    BOOL Trapped = FALSE;          // Is instance trapped

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

    /* Instance implementation constructor.
     * ARGUMENTS:
     *   - module to create instance of:
     *       module_source_impl &Source;
     */
    instance_impl( const source_impl &Source ) : Source(Source)
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
      // auto *RootFnPtr = Source.GetExportFunction(FunctionName);
      // if (RootFnPtr == nullptr)
      //   return std::nullopt;
      // const compiled_function_data &RootFn = *RootFnPtr;

      return std::nullopt;
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

  /* Binary stream utility set */
  namespace bin_util
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
  } /* end of 'bin_util' namespace */
} /* end of 'watap::impl::standard' namespace */

#endif // !defined(__watap_impl_standard_def_h_)

/* END OF 'watap_impl_standard_def.h' FILE */

#if 0
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
