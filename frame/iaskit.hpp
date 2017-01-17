#ifndef IASKIT_HPP
#define IASKIT_HPP

#include <assert.h>
#include <typeinfo>
#include <algorithm>
#include <random>


#include <hmlp.h>
#include <hmlp_blas_lapack.h>
#include <hmlp_util.hpp>
#include <hmlp_thread.hpp>
#include <hmlp_runtime.hpp>
#include <tree.hpp>
#include <skel.hpp>

#define DEBUG_IASKIT 1

namespace hmlp
{
namespace iaskit
{





template<typename TKERNEL, typename T>
class Setup : hmlp::tree::Setup<T>
{
  public:

    // Maximum rank 
    size_t s;

    // Relative error for rank-revealed QR
    T stol;

    // kernel function
    TKERNEL kernel;
};




/**
 *  @brief This class contains all iaskit related data.
 */ 
template<typename T, typename TKERNEL>
class Data
{
  public:

    bool isskel = false;

    std::vector<size_t> skels;

    hmlp::Data<T> proj;

    TKERNEL kernel;

};


template<typename NODE>
void skeletonize( NODE *node )
{
  //int maxs = 20;
  //int nsamples = 4 * maxs;
  //auto *lchild = node->lchild;
  //auto *rchild = node->rchild;
  //auto &X = (*node->X);


  // Early return if we do not need to skeletonize
  if ( !node->parent )
  {
    printf( "no skeletonization\n" );
    return;
  }

  // random sampling or important sampling for rows.
  std::vector<size_t> amap;
  std::vector<size_t> bmap;
  std::vector<size_t> &lids = node->lids;

  // Get setup and shared data.
  auto &kernel = node->setup->kernel;
  auto maxs = node->setup->s;
  auto nsamples = 4 * maxs;

  // Get node private data.
  auto &data = node->data;
  auto &skels = data.skels;
  auto &proj = data.proj;
  auto *lchild = node->lchild;
  auto *rchild = node->rchild;
  auto &X = (*node->X);

  printf( "id %d l %d n %d isleaf %d\n", node->treelist_id, node->l, node->n, node->isleaf );
  printf( "skels.size() %lu\n", node->data.skels.size() );

  // amap needs a random sampling scheme. TODO: this seems to be slow.
 

  if ( lids.size() > maxs )
  {
    if ( nsamples < X.num() - node->n )
    {
      amap.reserve( nsamples );
      while ( amap.size() < nsamples )
      {
        size_t sample = rand() % X.num();
        if ( std::find( amap.begin(), amap.end(), sample ) == amap.end() &&
             std::find( lids.begin(), lids.end(), sample ) == lids.end() )
        {
          amap.push_back( sample );
        }
      }
    }
    else // Use all off-diagonal blocks without samples.
    {
      for ( int sample = 0; sample < X.num(); sample ++ )
      {
        if ( std::find( amap.begin(), amap.end(), sample ) == amap.end() )
        {
          amap.push_back( sample );
        }
      }
    }

    if ( node->isleaf )
    {
      bmap = node->lids;
      auto A = X( amap );
      auto B = X( bmap );
      auto Kab = kernel( A, B );
      hmlp::skel::id( amap.size(), bmap.size(), maxs, Kab, skels, proj );
    }
    else
    {
      auto &lskels = lchild->data.skels;
      auto &rskels = rchild->data.skels;
      bmap = lskels;
      bmap.insert( bmap.end(), rskels.begin(), rskels.end() );
      auto A = X( amap );
      auto B = X( bmap );
      auto Kab = kernel( A, B );
      hmlp::skel::id( amap.size(), bmap.size(), maxs, Kab, skels, proj );
    }
  }
  else
  {
    skels = lids;
    proj.resize( lids.size(), lids.size(), 0.0 );
    for ( int i = 0; i < lids.size(); i++ )
    {
      proj[ i * proj.dim() + i ] = 1.0;
    }
  }


  node->data.isskel = true;

}; // end skeletonize()


  

template<typename NODE>
class Task : public hmlp::Task
{
  public:

    NODE *arg;

    void Set( NODE *user_arg )
    {
      name = std::string( "Skeletonization" );
      arg = user_arg;
    };

    void Execute( Worker* user_worker )
    {
      //printf( "SkeletonizeTask Execute 2\n" );
      skeletonize( arg );
    };

  private:
};







}; // end namespace iaskit
}; // end namespace hmlp

#endif // define IASKIT_HPP