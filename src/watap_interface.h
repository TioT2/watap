

#ifndef __watap_interface_h_
#define __watap_interface_h_

#include "watap_bin.h"
#include "watap_utils.h"

/* Project namespace // WASM Runtime implementation namespace */
namespace watap
{
  /* WASM Function signature representation structure */
  struct function_signature
  {
    std::vector<bin::value_type> ArgumentTypes; // List of argument types
    std::optional<bin::value_type> ReturnType;  // List of return types
  }; /* End of 'function_type' structure */

  /* Function import requirement description */
  struct function_import_description
  {
    std::string ModuleName;       // Function module name
    std::string Name;             // Function name
    function_signature Signature; // Function signature
  }; /* End of 'function_import_description' structure */

  /* Variable import requirement description */
  struct variable_import_description
  {
    std::string ModuleName;       // Function module name
    std::string Name;             // Function name
    bin::value_type VariableType; // Type of imported variable
    bin::mutability Mutability;   // It's mutability
  }; /* End of 'variable_import_description' structure */

  /* WASM Value representation structure, analog of __m128 practically */
  union value
  {
    INT64   I64x2[2];  // 2 times INT64
    UINT64  U64x2[2];  // 2 times UINT64
    FLOAT64 F64x2[2];  // 2 times DOUBLE

    INT32   I32x4[4];  // 4 times INT
    UINT32  U32x4[4];  // 4 times UINT
    FLOAT32 F32x4[4];  // 4 times FLOAT

    INT16   I16x8[8];  // 8 times SHORT
    UINT16  U16x8[8];  // 8 times USHORT

    INT8    I8x16[16]; // 16 times CHAR
    UINT8   U8x16[16]; // 16 times UINT8
  }; /* End of 'value' structure */

  /* Module source descrpitor */
  using module_source_info = std::variant<
    std::span<const UINT8>, // WASM (Binary representation)
    std::string_view       // WAT  (Text representation)
  >;

  /* Import table descriptor */
  struct import_table_info
  {
  }; /* End of 'import_table_info' structure */

  /* WASM Module representation structure */
  class module_source abstract
  {
  public:
    /* Start function name getting function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (std::optional<std::string_view>) Start function name, if it is defined.
     */
    virtual std::optional<std::string_view> GetStartName( VOID ) const = 0;
  }; /* End of 'module_source' class */

  /* WASM Import table representation class */
  class import_table abstract
  {
  public:
  }; /* End of 'import_table' structure */

  /* Module instance descriptor */
  struct runtime_info
  {
    module_source *ModuleSource; // Actual module
    import_table *ImportTable;   // Table of module imports
  }; /* End of 'module_instance_info' structure */

  /* Started module representation class */
  class runtime abstract
  {
  public:
    /* Module function calling function.
     * ARGUMENTS:
     *   - function name:
     *       std::string_view FunctionName;
     *   - function parameter list:
     *       std::span<const value> Parameters;
     * RETURNS:
     *   (std::optional<value>) Return value if called, std::nullopt otherwise;
     */
    virtual std::optional<value> Call( std::string_view FunctionName, std::span<const value> Parameters = {} ) = 0;

    /* Global value getting function.
     * ARGUMENTS:
     *   - global value name:
     *       std::string_view GlobalName;
     * RETURNS:
     *   (VOID *) Pointer to runtime memory that corresponds to WasmPtr;
     */
    virtual BOOL SetGlobal( std::string_view GlobalName, value Value ) = 0;

    /* Global value getting function.
     * ARGUMENTS:
     *   - global value name:
     *       std::string_view GlobalName;
     * RETURNS:
     *   (VOID *) Pointer to runtime memory that corresponds to WasmPtr;
     */
    virtual std::optional<value> GetGlobal( std::string_view GlobalName ) const = 0;

    /* Module pointer dereferencing function.
     * ARGUMENTS:
     *   - module ptr:
     *       UINT32 WasmPtr;
     * RETURNS:
     *   (VOID *) Pointer to runtime memory that corresponds to WasmPtr;
     */
    virtual VOID * GetPtr( UINT32 WasmPtr ) = 0;

    /* Is module trapped, trap requires module full restart.
     * ARGUMENTS: None.
     * RETURNS:
     *   (BOOL) TRUE if runtime is trapped, FALSE otherwise;
     */
    virtual BOOL IsTrapped( VOID ) const = 0;

    /* Module restart function.
     * ARGUMENTS: None.
     * RETURNS: None.
     */
    virtual VOID Restart( VOID ) = 0;
  }; /* End of 'runtime' class */

  /* WASM Runtime interface representation structure */
  class interface abstract
  {
  public:
    /* Module source create function.
     * ARGUMENTS:
     *   - module source descriptor:
     *       const module_source_info &Info;
     * RETURNS:
     *   (module_source *) Created module source pointer;
     */ 
    virtual module_source * CreateModuleSource( const module_source_info &Info ) = 0;

    /* Module source destroy function.
     * ARGUMENTS:
     *   - module source pointer:
     *       module_source *ModuleSource;
     * RETURNS: None.
     */ 
    virtual VOID DestroyModuleSource( module_source *ModuleSource ) = 0;

    /* Import table create function.
     * ARGUMENTS:
     *   - import table descriptor:
     *       const import_table_info &Info;
     * RETURNS:
     *   (import_table *) Created import table pointer;
     */ 
    virtual import_table * CreateImportTable( const import_table_info &Info ) = 0;

    /* Import table destroy function.
     * ARGUMENTS:
     *   - import table pointer:
     *       import_table *ImportTable;
     * RETURNS: None.
     */ 
    virtual VOID DestroyImportTable( import_table *ImportTable ) = 0;

    /* Runtime create function.
     * ARGUMENTS:
     *   - runtime descriptor:
     *       const runtime_info &Info;
     * RETURNS:
     *   (runtime *) Created import table pointer;
     */ 
    virtual runtime * CreateRuntime( const runtime_info &Info ) = 0;

    /* Runtime destroy function.
     * ARGUMENTS:
     *   - runtime pointer:
     *       runtime *Runtime;
     * RETURNS: None.
     */ 
    virtual VOID DestroyRuntime( runtime *Runtime ) = 0;
  }; /* End of 'interface' class */
} /* end of 'watap' namespace */

#endif // !defined(__watap_interface_h_)

/* END OF 'watap_interface.h' FILE */
