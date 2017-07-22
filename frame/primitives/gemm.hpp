#ifndef GEMM_HPP
#define GEMM_HPP

#include <hmlp.h>
#include <hmlp_blas_lapack.h>

/** matrix view */
#include <containers/view.hpp>


namespace hmlp
{
namespace gemm
{


template<typename T>
void xgemmTask(
    T alpha, hmlp::View<T> &A, 
             hmlp::View<T> &B, 
    T beta,  hmlp::View<T> &C )
{
  std::string transA, transB;

  printf( "\n");
  printf( "alpha %3.1E beta %3.1E\n", alpha, beta );
  printf( "A: " ); A.Print();
  printf( "B: " ); B.Print();
  printf( "C: " ); C.Print();

  if ( A.IsTransposed() ) transA = "Transpose";
  else                    transA = "No transpose";
  if ( B.IsTransposed() ) transB = "Transpose";
  else                    transB = "No transpose";

  size_t m = C.row();
  size_t n = C.col();
  size_t k = A.col();

  xgemm
  ( 
    transA.data(), transB.data(),
    m, n, k,
    alpha, A.data(), A.ld(),
           B.data(), B.ld(),
    beta,  C.data(), C.ld()
  );

}; /** end xgemmTask() */


/**
 *  @brief
 */ 
template<size_t NB = 512, typename T>
void xgemm_var1( 
    T alpha, hmlp::View<T> &A, 
             hmlp::View<T> &B, 
    T beta,  hmlp::View<T> &C )
{
  printf( "var1\n" ); fflush( stdout );

  /** all subviews */
  hmlp::View<T> AL, AR, 
                A0, A1, A2;
  hmlp::View<T> BT, BB, 
                B0, B1, B2;
  
  /** A = [ AL, AR ] */
  A.Partition1x2( AL, AR, 0, LEFT );
  /** B = [ BT; BB ] */
  B.Partition2x1( BT,
                  BB,     0, TOP  ); 

  printf( "AL.col() %lu AR.col() %lu A.col() %lu\n", AL.col(), AR.col(), A.col() );

  while ( AL.col() < A.col() )
  {
    //printf( "AL.col() %lu AR.col() %lu A.col() %lu\n", AL.col(), AR.col(), A.col() );
    size_t b = std::min( AR.col(), NB );

    /** repartition A */
    Repartition1x2To1x3( AL,      AR,
                         /** **** */
                         A0,  A1, A2, b, RIGHT );
    /** repartition B */
    Repartition2x1To3x1( BT, /**/ B0,
                             /**/ B1,
                         BB, /**/ B2, b, BOTTOM );

    /** --------------------------------------------------- */
    xgemmTask( alpha, A1, B1, beta, C );
    beta = 1.0;
    /** --------------------------------------------------- */

    /** merge A */
    ContinueWith1x3To1x2( AL,      AR,
                          /** **** */
                          A0,  A1, A2, LEFT );
    /** merge B */
    ContinueWith3x1To2x1( BT, /**/ B0,
                              /**/ B1,
                          BB, /**/ B2,  TOP );

  } /** end while */

  printf( "end var1\n" ); fflush( stdout );
}; /** end xgemm_var1() */


/**
 *  @brief [ A * BL + CL, A * BR + CR ] 
 */ 
template<size_t NB = 512, typename T>
void xgemm_var2( 
    T alpha, hmlp::View<T> &A, 
             hmlp::View<T> &B, 
    T beta,  hmlp::View<T> &C )
{
  printf( "var2\n" ); fflush( stdout );

  /** all subviews */
  hmlp::View<T> CL, CR, 
                C0, C1, C2;
  hmlp::View<T> BL, BR, 
                B0, B1, B2;
  
  C.Partition1x2( CL, CR, 0, LEFT );
  B.Partition1x2( BL, BR, 0, LEFT );

  while ( BL.col() < B.col() )
  {
    size_t b = std::min( BR.col(), NB );

    /** repartition C */
    Repartition1x2To1x3( CL,      CR,
                         /** **** */
                         C0,  C1, C2, b, RIGHT );
    /** repartition B */
    Repartition1x2To1x3( BL,      BR,
                         /** **** */
                         B0,  B1, B2, b, RIGHT );

    /** --------------------------------------------------- */
    xgemm_var1( alpha, A, B1, beta, C1 );
    /** --------------------------------------------------- */

    /** merge C */
    ContinueWith1x3To1x2( CL,      CR,
                          /** **** */
                          C0,  C1, C2, LEFT );
    /** merge B */
    ContinueWith1x3To1x2( BL,      BR,
                          /** **** */
                          B0,  B1, B2, LEFT );

  } /** end while */

  printf( "end var2\n" ); fflush( stdout );
}; /** end xgemm_var2() */


/**
 *  @brief [ AT * B + CT; AB * B + CB ] 
 */ 
template<size_t NB = 512, typename T>
void xgemm_var3( 
    T alpha, hmlp::View<T> &A, 
             hmlp::View<T> &B, 
    T beta,  hmlp::View<T> &C )
{
  printf( "var3\n" ); fflush( stdout );

  /** all subviews */
  hmlp::View<T> AT, A0, CT, C0, 
                AB, A1, CB, C1,
                    A2,     C2;

  A.Partition2x1( AT,
                  AB,     0, TOP  ); 
  C.Partition2x1( CT,
                  CB,     0, TOP  ); 

  while ( AT.row() < A.row() )
  {
    size_t b = std::min( AB.row(), NB );

    /** repartition A */
    Repartition2x1To3x1( AT, /**/ A0,
                             /**/ A1,
                         AB, /**/ A2, b, BOTTOM );
    /** repartition B */
    Repartition2x1To3x1( CT, /**/ C0,
                             /**/ C1,
                         CB, /**/ C2, b, BOTTOM );

    /** --------------------------------------------------- */
    xgemm_var2( alpha, A1, B, beta, C1 );
    /** --------------------------------------------------- */

    /** merge A */
    ContinueWith3x1To2x1( AT, /**/ A0,
                              /**/ A1,
                          AB, /**/ A2,  TOP );
    /** merge C */
    ContinueWith3x1To2x1( CT, /**/ C0,
                              /**/ C1,
                          CB, /**/ C2,  TOP );
  }; /** end while */

  printf( "end var3\n" ); fflush( stdout );
}; /** end xgemm_var3() */


template<typename T>
void xgemm( 
    T alpha, hmlp::View<T> &A, 
             hmlp::View<T> &B, 
    T beta,  hmlp::View<T> &C )
{
  xgemm_var3( alpha, A, B, beta, C );
}; /** xgemm() */


template<typename T>
void xgemm( 
    hmlpOperation_t transA, hmlpOperation_t transB, 
    T alpha, hmlp::Data<T> &A, 
             hmlp::Data<T> &B, 
    T beta,  hmlp::Data<T> &C )
{
  hmlp::View<T> Aview, Bview, Cview;

  if ( transA == HMLP_OP_T ) Aview.Set<true >( A );
  else                       Aview.Set<false>( A );
  if ( transB == HMLP_OP_T ) Bview.Set<true >( B );
  else                       Bview.Set<false>( B );

  /** C is always not transpose */
  Cview.Set( C );

  xgemm( alpha, Aview, Bview, beta, Cview );
  
}; /** xgemm() */


}; /** end namespace gemm */
}; /** end namespace hmlp */


#endif /** define GEMM_HPP */
