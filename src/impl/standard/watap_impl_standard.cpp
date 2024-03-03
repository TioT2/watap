#define WATAP_IMPL_STANDARD

#include "watap_impl_standard_interface.h"

namespace watap::impl::standard
{
  /* Interface create function.
   * ARGUMENTS: None.
   * RETURNS:
   *   (interface *) Created interface pointer;
   */
  interface * Create( VOID )
  {
    return new interface_impl();
  } /* End of 'Create' function */

  /* Interface create function.
   * ARGUMENTS:
   *   - interface to destroy:
   *       interface *Interface;
   * RETURNS: None.
   */
  VOID Destroy( interface *Interface )
  {
    if (auto Impl = dynamic_cast<interface_impl *>(Interface))
      delete Impl;
  } /* End of 'Destroy' function */
} /* end of 'watap::impl::standard' namespace */