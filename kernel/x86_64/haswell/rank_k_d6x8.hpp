#include <stdio.h>
#include <hmlp_internal.hpp>


/** BLIS kernel prototype declaration */ 
BLIS_GEMM_KERNEL(bli_sgemm_asm_6x16,float);
BLIS_GEMM_KERNEL(bli_dgemm_asm_6x8,double);


struct rank_k_asm_s6x16
{
  const static size_t mr         =  6;
  const static size_t nr         = 16;
  const static size_t pack_mr    =  6;
  const static size_t pack_nr    = 16;
  const static size_t align_size = 32;
  const static bool   row_major  = false;

  inline STRA_OPERATOR(float) const
  {
    printf( "no STRA_OPERATOR implementation\n" );
    exit( 1 );
  };

  inline GEMM_OPERATOR(float) const
  {
    float alpha = 1.0;
    /** if this is the first kc iteration then beta = 1.0 */
    float beta = aux->pc ? 1.0 : 0.0;
    /** invoke blis kernel */
    bli_sgemm_asm_6x16
    (
      k,
      &alpha,
      a,
      b,
      &beta,
      c, rs_c, cs_c,
      aux
    );
  };

}; /** end struct rank_k_asm_s6x16 */


struct rank_k_asm_d6x8 
{
  const static size_t mr         =  6;
  const static size_t nr         =  8;
  const static size_t pack_mr    =  6;
  const static size_t pack_nr    =  8;
  const static size_t align_size = 32;
  const static bool   row_major  = false;

  inline STRA_OPERATOR(double) const
  {
    printf( "no STRA_OPERATOR implementation\n" );
    exit( 1 );
  };

  inline GEMM_OPERATOR(double) const
  {
    double alpha = 1.0;
    /** if this is the first kc iteration then beta = 1.0 */
    double beta = aux->pc ? 1.0 : 0.0;
    /** invoke blis kernel */
    bli_dgemm_asm_6x8
    (
      k,
      &alpha,
      a,
      b,
      &beta,
      c, rs_c, cs_c,
      aux
    );
  };

}; /** end struct rank_k_asm_d6x8 */
