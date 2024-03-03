#ifndef __watap_impl_standard_interface_h_
#define __watap_impl_standard_interface_h_

#include "watap_impl_standard_def.h"

namespace watap::impl::standard
{
  /* Interface implementation */
  class interface_impl : public interface
  {
  public:
    /* Module source create function.
     * ARGUMENTS:
     *   - module source descriptor:
     *       const module_source_info &Info;
     * RETURNS:
     *   (module_source *) Created module source pointer;
     */ 
    module_source * CreateModuleSource( const module_source_info &Info ) override;

    /* Module source destroy function.
     * ARGUMENTS:
     *   - module source pointer:
     *       module_source *ModuleSource;
     * RETURNS: None.
     */ 
    VOID DestroyModuleSource( module_source *ModuleSource ) override;

    /* Import table create function.
     * ARGUMENTS:
     *   - import table descriptor:
     *       const import_table_info &Info;
     * RETURNS:
     *   (import_table *) Created import table pointer;
     */ 
    import_table * CreateImportTable( const import_table_info &Info )
    {
      return nullptr;
    } /* End of 'CreateImportTable' function */

    /* Import table destroy function.
     * ARGUMENTS:
     *   - import table pointer:
     *       import_table *ImportTable;
     * RETURNS: None.
     */ 
    VOID DestroyImportTable( import_table *ImportTable )
    {
    } /* End of 'DestroyImportTable' function */

    /* Runtime create function.
     * ARGUMENTS:
     *   - runtime descriptor:
     *       const runtime_info &Info;
     * RETURNS:
     *   (runtime *) Created import table pointer;
     */ 
    runtime * CreateRuntime( const runtime_info &Info )
    {
      return nullptr;
    } /* End of 'CreateRuntime' function */

    /* Runtime destroy function.
     * ARGUMENTS:
     *   - runtime pointer:
     *       runtime *Runtime;
     * RETURNS: None.
     */ 
    VOID DestroyRuntime( runtime *Runtime )
    {

    } /* End of 'DestroyRuntime' function */
  }; /* End of 'interface_impl' class */
} /* end of 'watap_impl_standard_interface' namespace */

#endif // !defined(__watap_impl_standard_interface_h_)

/* END OF 'watap_impl_standard_interface.h' FILE */