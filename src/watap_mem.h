#ifndef __watap_mem_h_
#define __watap_mem_h_

#include "watap_def.h"

namespace watap
{
  /* Memory type */
  enum class type
  {
    eLimMin    = 0x00, // Minimal limit
    eLimMinMax = 0x01, // Minimal and maximal limits
    eFunction  = 0x60, // Function
    eExternRef = 0x6F, // Extern reference
    eFuncRef   = 0x70, // Function reference
    eV128      = 0x7B, // 128 bit SIMD vector
    eF64       = 0x7C, //  64 bit floating point type
    eF32       = 0x7D, //  32 bit floating point type
    eI64       = 0x7E, //  64 bit integral type
    eI32       = 0x7F, //  32 bit integral type
  }; /* End of 'type' namespace */

  struct limit_data
  {
    UINT32 Min;
    UINT32 Max;
  };

  enum class instruction : UINT8
  {
    eUnreachable  = 0x00, // FATAL ERROR!!!
    eNop          = 0x01, // Nop
    eBlock        = 0x02, // Just block
    eLoop         = 0x03, // Loop
    eIfElse       = 0x04, // IfElse where Else is optional
    eBr           = 0x0C, // Just goto
    eBrIf         = 0x0D, // ???
    eBrTable      = 0x0E, // switch/match
    eReturn       = 0x0F, // return
    eCall         = 0x10, // call function
    eCallIndirect = 0x11, // call from ptr

    eDrop         = 0x1A, // Pop stack
    eSelect       = 0x1B, // Select nonzero numeric operand
    eSelectTyped  = 0x1C, // Select nonzero operand of certain type

    eLocalGet     = 0x20, // Load local
    eLocalSet     = 0x21, // Store local
    eLocalTee     = 0x22, // Store and push
    eGlobalGet    = 0x23, // Load global
    eGlobalSet    = 0x24, // Store global

    eTableGet     = 0x25, // Get value from table
    eTableSet     = 0x26, // Set value into table

    eI32Load      = 0x28,
    eI64Load      = 0x29,
    eF32Load      = 0x2A,
    eF64Load      = 0x2B,

    eI32Load8S    = 0x2C,
    eI32Load8U    = 0x2D,
    eI32Load16S   = 0x2E,
    eI32Load16U   = 0x2F,

    eI64Load8S    = 0x30,
    eI64Load8U    = 0x31,
    eI64Load16S   = 0x32,
    eI64Load16U   = 0x33,
    eI64Load32S   = 0x34,
    eI64Load32U   = 0x35,

    eI32Store     = 0x36,
    eI64Store     = 0x37,
    eF32Store     = 0x38,
    eF64Store     = 0x39,
    
    eI32Store8    = 0x3A,
    eI32Store16   = 0x3B,

    eI64Store8    = 0x3C,
    eI64Store16   = 0x3D,
    eI64Store32   = 0x3E,

    eMemorySize   = 0x3F,   // Returns current memory size
    eMemoryGrow   = 0x40,   // Extends memory and returns old size

    eI32Const     = 0x41, // Push int32 constant
    eI64Const     = 0x42, // Push int64 constant
    eF32Const     = 0x43, // Push Float32 constant
    eF64Const     = 0x44, // Push Float64 constant

    // MATH OPERATIONS

    eNullPush     = 0xD0, // Push 0
    eNullCheck    = 0xD1, // Push (Pop == 0)
    eRefFunc      = 0xD2, // Push &Fn

    eSystem       = 0xFC, // System instruction (extended by system_instruction)
  }; /* End of 'instruction' namespace */

  
  /* eTableOp instruction extension bytes */
  enum class system_instruction : UINT32
  {
    eMemoryInit =  8, // Initialize memory
    eDataDrop   =  9, // Drop data segment (optimization hint)
    eMemoryCopy = 10, // Copy from wasm data segment
    eMemoryFill = 11, // Fill memory
    eTableInit  = 12, // Init table
    eTableDrop  = 13, // Drop table
    eTableCopy  = 14, // Copy table
    eTableGrow  = 15, // Grow table
    eTableSize  = 16, // Resize table
    eTableFill  = 17, // Fill table
  }; /* End of 'instruction_table_op' enumeration */

} /* end of 'watap' namespace */

#endif // !defined(__watap_mem_h_)
