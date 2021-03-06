/*  File produced by Kranc */

#define KRANC_C

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "GenericFD.h"
#include "Differencing.h"
#include "cctk_Loop.h"
#include "loopcontrol.h"

/* Define macros used in calculations */
#define INITVALUE (42)
#define QAD(x) (SQR(SQR(x)))
#define INV(x) ((1.0) / (x))
#define SQR(x) ((x) * (x))
#define CUB(x) ((x) * (x) * (x))

extern "C" void eulersr_cons_calc_rhs_3_SelectBCs(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  CCTK_INT ierr = 0;
  ierr = Boundary_SelectGroupForBC(cctkGH, CCTK_ALL_FACES, GenericFD_GetBoundaryWidth(cctkGH), -1 /* no table */, "EulerSR::Den_grouprhs","flat");
  if (ierr < 0)
    CCTK_WARN(1, "Failed to register flat BC for EulerSR::Den_grouprhs.");
  ierr = Boundary_SelectGroupForBC(cctkGH, CCTK_ALL_FACES, GenericFD_GetBoundaryWidth(cctkGH), -1 /* no table */, "EulerSR::S_grouprhs","flat");
  if (ierr < 0)
    CCTK_WARN(1, "Failed to register flat BC for EulerSR::S_grouprhs.");
  ierr = Boundary_SelectGroupForBC(cctkGH, CCTK_ALL_FACES, GenericFD_GetBoundaryWidth(cctkGH), -1 /* no table */, "EulerSR::tau_grouprhs","flat");
  if (ierr < 0)
    CCTK_WARN(1, "Failed to register flat BC for EulerSR::tau_grouprhs.");
  return;
}

static void eulersr_cons_calc_rhs_3_Body(cGH const * restrict const cctkGH, int const dir, int const face, CCTK_REAL const normal[3], CCTK_REAL const tangentA[3], CCTK_REAL const tangentB[3], int const imin[3], int const imax[3], int const n_subblock_gfs, CCTK_REAL * restrict const subblock_gfs[])
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  
  /* Declare finite differencing variables */
  
  /* Include user-supplied include files */
  
  /* Initialise finite differencing variables */
  ptrdiff_t const di = 1;
  ptrdiff_t const dj = CCTK_GFINDEX3D(cctkGH,0,1,0) - CCTK_GFINDEX3D(cctkGH,0,0,0);
  ptrdiff_t const dk = CCTK_GFINDEX3D(cctkGH,0,0,1) - CCTK_GFINDEX3D(cctkGH,0,0,0);
  ptrdiff_t const cdi = sizeof(CCTK_REAL) * di;
  ptrdiff_t const cdj = sizeof(CCTK_REAL) * dj;
  ptrdiff_t const cdk = sizeof(CCTK_REAL) * dk;
  CCTK_REAL const dx = ToReal(CCTK_DELTA_SPACE(0));
  CCTK_REAL const dy = ToReal(CCTK_DELTA_SPACE(1));
  CCTK_REAL const dz = ToReal(CCTK_DELTA_SPACE(2));
  CCTK_REAL const dt = ToReal(CCTK_DELTA_TIME);
  CCTK_REAL const t = ToReal(cctk_time);
  CCTK_REAL const dxi = INV(dx);
  CCTK_REAL const dyi = INV(dy);
  CCTK_REAL const dzi = INV(dz);
  CCTK_REAL const khalf = 0.5;
  CCTK_REAL const kthird = 1/3.0;
  CCTK_REAL const ktwothird = 2.0/3.0;
  CCTK_REAL const kfourthird = 4.0/3.0;
  CCTK_REAL const keightthird = 8.0/3.0;
  CCTK_REAL const hdxi = 0.5 * dxi;
  CCTK_REAL const hdyi = 0.5 * dyi;
  CCTK_REAL const hdzi = 0.5 * dzi;
  
  /* Initialize predefined quantities */
  CCTK_REAL const p1o1 = 1;
  CCTK_REAL const p1odx = INV(dx);
  CCTK_REAL const p1ody = INV(dy);
  CCTK_REAL const p1odz = INV(dz);
  
  /* Assign local copies of arrays functions */
  
  
  
  /* Calculate temporaries and arrays functions */
  
  /* Copy local copies back to grid functions */
  
  /* Loop over the grid points */
  #pragma omp parallel
  CCTK_LOOP3 (eulersr_cons_calc_rhs_3,
    i,j,k, imin[0],imin[1],imin[2], imax[0],imax[1],imax[2],
    cctk_lsh[0],cctk_lsh[1],cctk_lsh[2])
  {
    ptrdiff_t const index = di*i + dj*j + dk*k;
    
    /* Assign local copies of grid functions */
    
    CCTK_REAL DenFluxL = DenFlux[index];
    CCTK_REAL DenrhsL = Denrhs[index];
    CCTK_REAL S1FluxL = S1Flux[index];
    CCTK_REAL S1rhsL = S1rhs[index];
    CCTK_REAL S2FluxL = S2Flux[index];
    CCTK_REAL S2rhsL = S2rhs[index];
    CCTK_REAL S3FluxL = S3Flux[index];
    CCTK_REAL S3rhsL = S3rhs[index];
    CCTK_REAL tauFluxL = tauFlux[index];
    CCTK_REAL taurhsL = taurhs[index];
    
    
    /* Include user supplied include files */
    
    /* Precompute derivatives */
    CCTK_REAL const PDplus3DenFlux = PDplus3(&DenFlux[index]);
    CCTK_REAL const PDplus3S1Flux = PDplus3(&S1Flux[index]);
    CCTK_REAL const PDplus3S2Flux = PDplus3(&S2Flux[index]);
    CCTK_REAL const PDplus3S3Flux = PDplus3(&S3Flux[index]);
    CCTK_REAL const PDplus3tauFlux = PDplus3(&tauFlux[index]);
    
    /* Calculate temporaries and grid functions */
    DenrhsL = DenrhsL - PDplus3DenFlux;
    
    S1rhsL = S1rhsL - PDplus3S1Flux;
    
    S2rhsL = S2rhsL - PDplus3S2Flux;
    
    S3rhsL = S3rhsL - PDplus3S3Flux;
    
    taurhsL = taurhsL - PDplus3tauFlux;
    
    /* Copy local copies back to grid functions */
    Denrhs[index] = DenrhsL;
    S1rhs[index] = S1rhsL;
    S2rhs[index] = S2rhsL;
    S3rhs[index] = S3rhsL;
    taurhs[index] = taurhsL;
  }
  CCTK_ENDLOOP3 (eulersr_cons_calc_rhs_3);
}

extern "C" void eulersr_cons_calc_rhs_3(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  
  if (verbose > 1)
  {
    CCTK_VInfo(CCTK_THORNSTRING,"Entering eulersr_cons_calc_rhs_3_Body");
  }
  
  if (cctk_iteration % eulersr_cons_calc_rhs_3_calc_every != eulersr_cons_calc_rhs_3_calc_offset)
  {
    return;
  }
  
  const char *groups[] = {"EulerSR::Den_flux_group","EulerSR::Den_grouprhs","EulerSR::S1_flux_group","EulerSR::S2_flux_group","EulerSR::S3_flux_group","EulerSR::S_grouprhs","EulerSR::tau_flux_group","EulerSR::tau_grouprhs"};
  GenericFD_AssertGroupStorage(cctkGH, "eulersr_cons_calc_rhs_3", 8, groups);
  
  GenericFD_EnsureStencilFits(cctkGH, "eulersr_cons_calc_rhs_3", 1, 1, 1);
  
  GenericFD_LoopOverInterior(cctkGH, &eulersr_cons_calc_rhs_3_Body);
  
  if (verbose > 1)
  {
    CCTK_VInfo(CCTK_THORNSTRING,"Leaving eulersr_cons_calc_rhs_3_Body");
  }
}
