/*  */

#ifndef __watap_impl_standard_h_
#define __watap_impl_standard_h_

#include "../../watap_interface.h"

/* Project namespace // WASM Namespace // Implementation namesapce // Standard (multiplatform) implementation namespace */
namespace watap::impl::standard
{
  /* Interface create function.
   * ARGUMENTS: None.
   * RETURNS:
   *   (interface *) Created interface pointer;
   */
  interface * Create( VOID );

  /* Interface create function.
   * ARGUMENTS:
   *   - interface to destroy:
   *       interface *Interface;
   * RETURNS: None.
   */
  VOID Destroy( interface *Interface );
} /* end of 'watap::impl::standard' namespace */

#endif // !defined(__watap_impl_standard_h_)

/* END OF 'watap_impl_standard.h' FILE */
