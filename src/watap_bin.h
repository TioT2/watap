

#ifndef __watap_bin_h_
#define __watap_bin_h_

#include "watap_def.h"

/* Project namespace // WASM Runtime implementation namespace */
namespace watap::bin
{
  /* Reference type representation enumeration */
  enum class reference_type : UINT8
  {
    eExternRef = 0x6F, // External reference
    eFuncRef   = 0x70, // Function reference
  }; /* End of 'reference_type' enumeration */

  /* Numeric type representation enumeration */
  enum class numeric_type : UINT8
  {
    eF64       = 0x7C, //  64 bit floating point type
    eF32       = 0x7D, //  32 bit floating point type
    eI64       = 0x7E, //  64 bit integral type
    eI32       = 0x7F, //  32 bit integral type
  }; /* End of 'numeric_type' enumeration */

  /* SIMD type representation enumeration */
  enum class vector_type : UINT8
  {
    eV128 = 0x7B, // 128 bit simd type
  }; /* End of 'vector_type' enumeration */

  /* Limit format representation structure */
  enum class limit_type : UINT8
  {
    eMin    = 0x00, // Min
    eMinMax = 0x01, // MinMax
  }; /* End of 'limit_type' structure */

  /* Value type */
  enum class value_type : UINT8
  {
    eExternRef = (UINT8)reference_type::eExternRef, // Extern reference
    eFuncRef   = (UINT8)reference_type::eFuncRef,   // Function reference
    eV128      = (UINT8)vector_type::eV128,         // 128 bit SIMD vector
    eF64       = (UINT8)numeric_type::eF64,         //  64 bit floating point type
    eF32       = (UINT8)numeric_type::eF32,         //  32 bit floating point type
    eI64       = (UINT8)numeric_type::eI64,         //  64 bit integral type
    eI32       = (UINT8)numeric_type::eI32,         //  32 bit integral type
  }; /* End of 'type' namespace */

  constexpr UINT8 FUNCTION_BEGIN = 0x60; // Function begin sign

  /* Value type size in bytes getting function.
   * ARGUMENTS:
   *   - value type:
   *       value_Type Type;
   * RETURNS:
   *   (SIZE_T) It's size in bytes;
   */
  inline constexpr SIZE_T GetValueTypeSize( value_type Type ) noexcept
  {
    switch (Type)
    {
    case value_type::eExternRef : return  4;
    case value_type::eFuncRef   : return  4;
    case value_type::eV128      : return 16;
    case value_type::eF64       : return  8;
    case value_type::eF32       : return  4;
    case value_type::eI64       : return  8;
    case value_type::eI32       : return  4;
    default                     : return  0;
    }
  } /* End of 'GetValueTypeSize' function */

  /* Mutability representation enumeration */
  enum class mutability : UINT8
  {
    eConstant = 0x00, // Constant type
    eMutable  = 0x01, // Mutable type
  }; /* End of 'mutability' class */

  enum class import_export_type : UINT8
  {
    eFunction = 0x00, // Function import
    eTable    = 0x01, // Table import
    eMemory   = 0x02, // Memory import
    eGlobal   = 0x03, // Global import
  }; /* End of 'import_type' enumeration */

  /* Global type representation structure */
  struct global_type
  {
    value_type Type;       // Type
    mutability Mutability; // Mutability
  }; /* End of 'global_type' structure */

  struct limits
  {
    UINT32 Min =  0U; // Minimal limits value
    UINT32 Max = ~0U; // Maximal limits value, 0xFFFFFFFF if max is not defined.
  }; /* End of 'limits' structure */

  /* Table type representation structure */
  struct table_type
  {
    reference_type ReferenceType; // Value reference type
    limits Limits;                // Limits
  }; /* End of 'table_type' structure */

  enum class instruction : UINT8
  {
    eUnreachable        = 0x00, // Fatal error
    eNop                = 0x01, // Nop
    eBlock              = 0x02, // Just block
    eLoop               = 0x03, // Loop
    eIf                 = 0x04, // Branching instruction
    eElse               = 0x05, // Else. Must appear after 'instruction::eIf' instruction only and once
    eExpressionEnd      = 0x0B, // Expression end
    eBr                 = 0x0C, // Goto
    eBrIf               = 0x0D, // Goto if
    eReturn             = 0x0F, // Return
    eBrTable            = 0x0E, // Switch/match
    eCall               = 0x10, // Call function by local index
    eCallIndirect       = 0x11, // Call from function ID

    eDrop               = 0x1A, // Pop stack
    eSelect             = 0x1B, // Select nonzero numeric operand
    eSelectTyped        = 0x1C, // Select nonzero operand of certain type

    eLocalGet           = 0x20, // Load local
    eLocalSet           = 0x21, // Store local
    eLocalTee           = 0x22, // Store and push
    eGlobalGet          = 0x23, // Load global
    eGlobalSet          = 0x24, // Store global

    eTableGet           = 0x25, // Get value from table
    eTableSet           = 0x26, // Set value into table

    eI32Load            = 0x28, // Load I32 from variable
    eI64Load            = 0x29, // Load I64 from variable
    eF32Load            = 0x2A, // Load F32 from variable
    eF64Load            = 0x2B, // Load F64 from variable

    eI32Load8S          = 0x2C, // Partially load variable from stack
    eI32Load8U          = 0x2D, // Partially load variable from stack
    eI32Load16S         = 0x2E, // Partially load variable from stack
    eI32Load16U         = 0x2F, // Partially load variable from stack

    eI64Load8S          = 0x30, // Partially load variable from stack
    eI64Load8U          = 0x31, // Partially load variable from stack
    eI64Load16S         = 0x32, // Partially load variable from stack
    eI64Load16U         = 0x33, // Partially load variable from stack
    eI64Load32S         = 0x34, // Partially load variable from stack
    eI64Load32U         = 0x35, // Partially load variable from stack

    eI32Store           = 0x36, // Store variable into stack
    eI64Store           = 0x37, // Store variable into stack
    eF32Store           = 0x38, // Store variable into stack
    eF64Store           = 0x39, // Store variable into stack

    eI32Store8          = 0x3A, // Partially store variable into stack
    eI32Store16         = 0x3B, // Partially store variable into stack

    eI64Store8          = 0x3C, // Partially store variable into stack
    eI64Store16         = 0x3D, // Partially store variable into stack
    eI64Store32         = 0x3E, // Partially store variable into stack

    eMemorySize         = 0x3F, // Returns current memory size
    eMemoryGrow         = 0x40, // Extends memory and returns old size

    eI32Const           = 0x41, // Push int32 constant
    eI64Const           = 0x42, // Push int64 constant
    eF32Const           = 0x43, // Push float32 constant
    eF64Const           = 0x44, // Push float64 constant

    eI32Eqz             = 0x45, // Math operation
    eI32Eq              = 0x46, // Math operation
    eI32Ne              = 0x47, // Math operation
    eI32LtS             = 0x48, // Math operation
    eI32LtU             = 0x49, // Math operation
    eI32GtS             = 0x4A, // Math operation
    eI32GtU             = 0x4B, // Math operation
    eI32LeS             = 0x4C, // Math operation
    eI32LeU             = 0x4D, // Math operation
    eI32GeS             = 0x4E, // Math operation
    eI32GeU             = 0x4F, // Math operation
    eI64Eqz             = 0x50, // Math operation
    eI64Eq              = 0x51, // Math operation
    eI64Ne              = 0x52, // Math operation
    eI64LtS             = 0x53, // Math operation
    eI64LtU             = 0x54, // Math operation
    eI64GtS             = 0x55, // Math operation
    eI64GtU             = 0x56, // Math operation
    eI64LeS             = 0x57, // Math operation
    eI64LeU             = 0x58, // Math operation
    eI64GeS             = 0x59, // Math operation
    eI64GeU             = 0x5A, // Math operation
    eF32Eq              = 0x5B, // Math operation
    eF32Ne              = 0x5C, // Math operation
    eF32Lt              = 0x5D, // Math operation
    eF32Gt              = 0x5E, // Math operation
    eF32Le              = 0x5F, // Math operation
    eF32Ge              = 0x60, // Math operation
    eF64Eq              = 0x61, // Math operation
    eF64Ne              = 0x62, // Math operation
    eF64Lt              = 0x63, // Math operation
    eF64Gt              = 0x64, // Math operation
    eF64Le              = 0x65, // Math operation
    eF64Ge              = 0x66, // Math operation
    eI32Clz             = 0x67, // Math operation
    eI32Ctz             = 0x68, // Math operation
    eI32Popcnt          = 0x69, // Math operation
    eI32Add             = 0x6A, // Math operation
    eI32Sub             = 0x6B, // Math operation
    eI32Mul             = 0x6C, // Math operation
    eI32DivS            = 0x6D, // Math operation
    eI32DivU            = 0x6E, // Math operation
    eI32RemS            = 0x6F, // Math operation
    eI32RemU            = 0x70, // Math operation
    eI32And             = 0x71, // Math operation
    eI32Or              = 0x72, // Math operation
    eI32Xor             = 0x73, // Math operation
    eI32Shl             = 0x74, // Math operation
    eI32ShrS            = 0x75, // Math operation
    eI32ShrU            = 0x76, // Math operation
    eI32Rotl            = 0x77, // Math operation
    eI32Rotr            = 0x78, // Math operation
    eI64Clz             = 0x79, // Math operation
    eI64Ctz             = 0x7A, // Math operation
    eI64Popcnt          = 0x7B, // Math operation
    eI64Add             = 0x7C, // Math operation
    eI64Sub             = 0x7D, // Math operation
    eI64Mul             = 0x7E, // Math operation
    eI64DivS            = 0x7F, // Math operation
    eI64DivU            = 0x80, // Math operation
    eI64RemS            = 0x81, // Math operation
    eI64RemU            = 0x82, // Math operation
    eI64And             = 0x83, // Math operation
    eI64Or              = 0x84, // Math operation
    eI64Xor             = 0x85, // Math operation
    eI64Shl             = 0x86, // Math operation
    eI64ShrS            = 0x87, // Math operation
    eI64ShrU            = 0x88, // Math operation
    eI64Rotl            = 0x89, // Math operation
    eI64Rotr            = 0x8A, // Math operation
    eF32Abs             = 0x8B, // Math operation
    eF32Neg             = 0x8C, // Math operation
    eF32Ceil            = 0x8D, // Math operation
    eF32Floor           = 0x8E, // Math operation
    eF32Trunc           = 0x8F, // Math operation
    eF32Nearest         = 0x90, // Math operation
    eF32Sqrt            = 0x91, // Math operation
    eF32Add             = 0x92, // Math operation
    eF32Sub             = 0x93, // Math operation
    eF32Mul             = 0x94, // Math operation
    eF32Div             = 0x95, // Math operation
    eF32Min             = 0x96, // Math operation
    eF32Max             = 0x97, // Math operation
    eF32CopySign        = 0x98, // Math operation
    eF64Abs             = 0x99, // Math operation
    eF64Neg             = 0x9A, // Math operation
    eF64Ceil            = 0x9B, // Math operation
    eF64Floor           = 0x9C, // Math operation
    eF64Trunc           = 0x9D, // Math operation
    eF64Nearest         = 0x9E, // Math operation
    eF64Sqrt            = 0x9F, // Math operation
    eF64Add             = 0xA0, // Math operation
    eF64Sub             = 0xA1, // Math operation
    eF64Mul             = 0xA2, // Math operation
    eF64Div             = 0xA3, // Math operation
    eF64Min             = 0xA4, // Math operation
    eF64Max             = 0xA5, // Math operation
    eF64CopySign        = 0xA6, // Math operation
    eI32WrapI64         = 0xA7, // Math operation
    eI32TruncF32S       = 0xA8, // Math operation
    eI32TruncF32U       = 0xA9, // Math operation
    eI32TruncF64S       = 0xAA, // Math operation
    eI32TruncF64U       = 0xAB, // Math operation
    eI64ExtendI32S      = 0xAC, // Math operation
    eI64ExtendI32U      = 0xAD, // Math operation
    eI64TruncF32S       = 0xAE, // Math operation
    eI64TruncF32U       = 0xAF, // Math operation
    eI64TruncF64S       = 0xB0, // Math operation
    eI64TruncF64U       = 0xB1, // Math operation
    eF32ConvertI32S     = 0xB2, // Math operation
    eF32ConvertI32U     = 0xB3, // Math operation
    eF32ConvertI64S     = 0xB4, // Math operation
    eF32ConvertI64U     = 0xB5, // Math operation
    eF32DemoteF64       = 0xB6, // Math operation
    eF64ConvertI32S     = 0xB7, // Math operation
    eF64ConvertI32U     = 0xB8, // Math operation
    eF64ConvertI64S     = 0xB9, // Math operation
    eF64ConvertI64U     = 0xBA, // Math operation
    eF64PromoteF32      = 0xBB, // Math operation
    eI32ReinterpretF32  = 0xBC, // Math operation
    eI64ReinterpretF64  = 0xBD, // Math operation
    eF32ReinterpretI32  = 0xBE, // Math operation
    eF64ReinterpretI64  = 0xBF, // Math operation
    eI32Extend8S        = 0xC0, // Math operation
    eI32Extend16S       = 0xC1, // Math operation
    eI64Extend8S        = 0xC2, // Math operation
    eI64Extend16S       = 0xC3, // Math operation
    eI64Extend32S       = 0xC4, // Math operation

    eRefNull            = 0xD0, // Push 0
    eRefIsNull          = 0xD1, // Push (Pop == 0)
    eRefFunc            = 0xD2, // Push &Fn

    eSystem             = 0xFC, // System instruction (extended by system_instruction)
    eVector             = 0xFD, // Vector instruction (extended by vector_instruction)
  }; /* End of 'instruction' namespace */

  /* Memory section identifier */
  enum class section_id : UINT8
  {
    eCustom    =  0, // Custom section
    eType      =  1, // Type declarations section
    eImport    =  2, // Import section
    eFunction  =  3, // Function storage section
    eTable     =  4, // Table storage section
    eMemory    =  5, // Memory section
    eGlobal    =  6, // Global variable section
    eExport    =  7, // Export section
    eStart     =  8, // Initial start section
    eElement   =  9, // Element section
    eCode      = 10, // Application WASM code section
    eData      = 11, // Component data segment section
    eDataCount = 12, // Number of component data segments in data section
  }; /* End of 'memory_section' structure */

  /* Vector instructions */
  enum class vector_instruction : UINT8
  {
    // TODO TC4 WASM Add V128 instruction set support
  }; /* End of 'vector_instruction' enumeration */

  /* System instructions */
  enum class system_instruction : UINT8
  {
    eI32TruncSatF32S =  0, // Trunc i32 into f32 as signed
    eI32TruncSatF32U =  1, // Trunc i32 into f32 as unsigned
    eI32TruncSatF64S =  2, // Trunc i32 into f64 as signed
    eI32TruncSatF64U =  3, // Trunc i32 into f64 as unsigned
    eI64TruncSatF32S =  4, // Trunc i64 into f32 as signed
    eI64TruncSatF32U =  5, // Trunc i64 into f32 as unsigned
    eI64TruncSatF64S =  6, // Trunc i64 into f64 as signed
    eI64TruncSatF64U =  7, // Trunc i64 into f64 as unsigned
    eMemoryInit      =  8, // Initialize memory
    eDataDrop        =  9, // Drop data segment (optimization hint)
    eMemoryCopy      = 10, // Copy from wasm data segment
    eMemoryFill      = 11, // Fill memory
    eTableInit       = 12, // Initialize new table
    eTableDrop       = 13, // Drop table
    eTableCopy       = 14, // Copy to another one table
    eTableGrow       = 15, // Grow table
    eTableSize       = 16, // Resize table
    eTableFill       = 17, // Fill table
  }; /* End of 'instruction_table_op' enumeration */

#pragma pack(push, 1)
  constexpr UINT32 WASM_MAGIC   = 0x6D736100; // Binary WebAssembly module magic number, "\0asm" character array

  /* WASM Module header */
  struct module_header
  {
    UINT32 Magic;   // Module magic number (must be equal WASM_MAGIC)
    UINT32 Version; // Module WASM version, 0x00000001 value expected, but not required

    /* Validation function.
     * ARGUMENTS: None.
     * RETURNS:
     *   (BOOL) TRUE if module header valid, FALSE otherwise.
     */
    constexpr BOOL Validate( VOID ) const noexcept
    {
      return Magic == WASM_MAGIC;
    } /* End of 'Validate' funciton */
  }; /* End of 'module_header' structure */

#pragma pack(pop)
} /* end of 'watap' namespace */

template <>
  struct std::formatter<watap::bin::value_type>
  {
    /* Format parameters parsing function.
     * ARGUENTS:
     *   - parsing context:
     *       std::format_parse_context &Context;
     * RETURNS:
     *   (auto) Format argument parse context end.
     */
    constexpr auto parse( std::format_parse_context &Context ) const
    {
      return Context.end();
    } /* End of 'parse' function */

    /* Format parameters parsing function.
     * ARGUENTS:
     *   - value to format:
     *       watap::bin::value_type Value;
     *   - formatting context:
     *       std::format_context &Context;
     * RETURNS:
     *   (auto) Format context end.
     */
    auto format( watap::bin::value_type Value, std::format_context &Context ) const
    {
      using namespace watap;

      const CHAR *TypeName;
      switch (Value)
      {
      case bin::value_type::eExternRef : TypeName = "ExternRef" ; break;
      case bin::value_type::eFuncRef   : TypeName = "FuncRef"   ; break;
      case bin::value_type::eV128      : TypeName = "V128"      ; break;
      case bin::value_type::eF64       : TypeName = "F64"       ; break;
      case bin::value_type::eF32       : TypeName = "F32"       ; break;
      case bin::value_type::eI64       : TypeName = "I64"       ; break;
      case bin::value_type::eI32       : TypeName = "I32"       ; break;
      default                          : TypeName = "<invalid>" ;
      }

      return std::format_to(Context.out(), "{}", TypeName);
    }
  }; /* End of 'std::formatter<watap::bin::value_type>' structure */

template <>
  struct std::formatter<watap::bin::limits>
  {
    /* Format parameters parsing function.
     * ARGUENTS:
     *   - parsing context:
     *       std::format_parse_context &Context;
     * RETURNS:
     *   (auto) Format argument parse context end.
     */
    constexpr auto parse( std::format_parse_context &Context ) const
    {
      return Context.end();
    } /* End of 'parse' function */

    /* Format parameters parsing function.
     * ARGUENTS:
     *   - value to format:
     *       watap::bin::limits Value;
     *   - formatting context:
     *       std::format_context &Context;
     * RETURNS:
     *   (auto) Format context end.
     */
    auto format( watap::bin::limits Value, std::format_context &Context ) const
    {
      if (Value.Max == ~0U)
        return std::format_to(Context.out(), "[{}, inf)", Value.Min);
      else
        return std::format_to(Context.out(), "[{}, {}]", Value.Min, Value.Max);
    }
  }; /* End of 'std::formatter<watap::bin::limits>' structure */

template <>
  struct std::formatter<watap::bin::mutability>
  {
    /* Format parameters parsing function.
     * ARGUENTS:
     *   - parsing context:
     *       std::format_parse_context &Context;
     * RETURNS:
     *   (auto) Format argument parse context end.
     */
    constexpr auto parse( std::format_parse_context &Context ) const
    {
      return Context.end();
    } /* End of 'parse' function */

    /* Format parameters parsing function.
     * ARGUENTS:
     *   - value to format:
     *       watap::bin::mutability Value;
     *   - formatting context:
     *       std::format_context &Context;
     * RETURNS:
     *   (auto) Format context end.
     */
    auto format( watap::bin::mutability Value, std::format_context &Context ) const
    {
      using namespace watap;
      const CHAR *Str;

      switch (Value)
      {
      case bin::mutability::eConstant : Str = "Constant" ; break;
      case bin::mutability::eMutable  : Str = "Mutable"  ; break;
      default                         : Str = "<invalid>";
      }
      return std::format_to(Context.out(), "{}", Str);
    } /* End of 'format' function */
  }; /* End of 'std::formatter<watap::bin::mutability>' structure */

  #endif // !defined(__watap_bin_h_)

/* END OF 'watap_bin.h' FILE */
