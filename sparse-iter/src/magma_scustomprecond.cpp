/*
    -- MAGMA (version 1.6.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date January 2015

       @generated from magma_zcustomprecond.cpp normal z -> s, Fri Jan 30 19:00:30 2015
       @author Hartwig Anzt

*/
#include "magma_lapack.h"
#include "common_magma.h"
#include "magmasparse.h"

#include <assert.h>

/*
    -- MAGMA (version 1.6.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date January 2015

       @author Hartwig Anzt 

       @generated from magma_zcustomprecond.cpp normal z -> s, Fri Jan 30 19:00:30 2015
*/
// includes CUDA
#include <cuda_runtime_api.h>
#include <cublas.h>
#include <cusparse_v2.h>
#include <cuda_profiler_api.h>

// project includes
#include "common_magma.h"
#include "magmasparse.h"

#include <assert.h>


#define PRECISION_s



/**
    Purpose
    -------

    This is an interface to the left solve for any custom preconditioner. 
    It should compute x = FUNCTION(b)
    The vectors are located on the device.

    Arguments
    ---------

    @param[in]
    b           magma_s_vector
                RHS

    @param[in,out]
    x           magma_s_vector*
                vector to precondition

    @param[in,out]
    precond     magma_s_preconditioner*
                preconditioner parameters
    @param[in]
    queue       magma_queue_t
                Queue to execute in.

    @ingroup magmasparse_saux
    ********************************************************************/

extern "C" magma_int_t
magma_sapplycustomprecond_l(
    magma_s_vector b, 
    magma_s_vector *x, 
    magma_s_preconditioner *precond,
    magma_queue_t queue )
{
    // vector access via x.dval, y->dval
    
    return MAGMA_SUCCESS;
}


/**
    Purpose
    -------

    This is an interface to the right solve for any custom preconditioner. 
    It should compute x = FUNCTION(b)
    The vectors are located on the device.

    Arguments
    ---------

    @param[in]
    b           magma_s_vector
                RHS

    @param[in,out]
    x           magma_s_vector*
                vector to precondition

    @param[in,out]
    precond     magma_s_preconditioner*
                preconditioner parameters
    @param[in]
    queue       magma_queue_t
                Queue to execute in.

    @ingroup magmasparse_saux
    ********************************************************************/

extern "C" magma_int_t
magma_sapplycustomprecond_r(
    magma_s_vector b, 
    magma_s_vector *x, 
    magma_s_preconditioner *precond,
    magma_queue_t queue )
{
    // vector access via x.dval, y->dval
    // sizes are x.num_rows, x.num_cols
    
    
    return MAGMA_SUCCESS;
}





