/****************************************************************************
 * Header File                                                              *
 * Programmer: Huangrui Mo                                                  *
 * - Follow the Google's C/C++ style Guide                                  *
 ****************************************************************************/
/****************************************************************************
 * Header File Guards to Avoid Interdependence
 ****************************************************************************/
#ifndef ARTRACFD_SOLVE_H_ /* if this is the first definition */
#define ARTRACFD_SOLVE_H_ /* a unique marker for this header file */
/****************************************************************************
 * Required Header Files
 ****************************************************************************/
#include "commons.h"
/****************************************************************************
 * Data Structure Declarations
 ****************************************************************************/
/****************************************************************************
 * Public Functions Declaration
 ****************************************************************************/
/*
 * Numerical solving
 *
 * Function
 *      Call a series of function to perform numerical computation of the flow
 *      case.
 */
extern int Solve(Field *, Flux *, Space *, Particle *, Time *, 
        const Partition *, const Fluid *, const Flow *);
#endif
/* a good practice: end file with a newline */

