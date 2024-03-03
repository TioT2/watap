#ifndef __watap_impl_standard_def_h_
#define __watap_impl_standard_def_h_

#include "watap_impl_standard.h"

#ifndef WATAP_IMPL_STANDARD
#  error This file shouldn't be included in global project tree
#endif // defined(WATAP_IMPL_STANDARD)

/* Project namespace // WASM Namespace // Implementation namesapce // Standard (multiplatform) implementation namespace */
namespace watap::impl::standard
{
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
    std::vector<BYTE> Instructions;            // Function instruction list
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

  class stack
  {
    SIZE_T Size = 0;
    VOID *Begin = nullptr;
    VOID *Current = nullptr;
  public:
    VOID Push( VOID )
    {

    } /* End of 'Push' function */

    VOID Pop( VOID )
    {

    } /* End of 'Pop' function */
  }; /* End of 'stack' structure */

  /* Runtime implementation */
  class runtime_impl : public runtime
  {
    const module_source_impl &Source; // Module source

    struct call
    {
      UINT32 FunctionIndex;       // Function index
      SIZE_T InstructionIndex;    // Instruction index
      SIZE_T LocalStackFrameSize; // Local stack frame size
    }; /* End of 'call' structure */

    std::vector<UINT8> Heap;       // Heap
    std::vector<UINT8> Stack;      // Local variable stack
    std::stack<UINT64> LocalStack; // Local variable stack
    std::stack<call> CallStack;    // Call stack, holds pointers to functions

  public:

    runtime_impl( const module_source_impl &Source ) : Source(Source)
    {
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
      // UINT32 FunctionIndex;
      // if (auto FIter = Source.FunctionExportTable.find(FunctionName); FIter != Source.FunctionExportTable.end())
      //   FunctionIndex = FIter->second;
      // else
      //   return std::nullopt;
      // const function &Function = Source.Functions[FunctionIndex];
      // 
      // call Call;
      // Call.FunctionIndex = FunctionIndex;
      // Call.InstructionIndex = 0;
      // Call.LocalStackFrameSize = Function.Locals.size(); // TODO TC4 Valid until SIMD instruction set not added.

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
      return FALSE;
    } /* End of 'IsTrapped' function */

    /* Module restart function.
     * ARGUMENTS: None.
     * RETURNS: None.
     */
    VOID Restart( VOID )
    {

    } /* End of 'Restart' function */
  }; /* End of 'runtime_impl' class */
} /* end of 'watap::impl::standard' namespace */

#endif // !defined(__watap_impl_standard_def_h_)

/* END OF 'watap_impl_standard_def.h' FILE */