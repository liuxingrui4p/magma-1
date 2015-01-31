/*
    -- MAGMA (version 1.6.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date January 2015

       @author Hartwig Anzt 

       @generated from zbpcg.cpp normal z -> s, Fri Jan 30 19:00:30 2015
*/

#include "common_magma.h"
#include "../include/magmasparse.h"

#include <assert.h>

#define RTOLERANCE     lapackf77_slamch( "E" )
#define ATOLERANCE     lapackf77_slamch( "E" )

#define  r(i)  r.dval+i*dofs
#define  b(i)  b.dval+i*dofs
#define  h(i)  h.dval+i*dofs
#define  p(i)  p.dval+i*dofs
#define  q(i)  q.dval+i*dofs



/**
    Purpose
    -------

    Solves a system of linear equations
       A * X = B
    where A is a real symmetric N-by-N positive definite matrix A.
    This is a GPU implementation of the block preconditioned Conjugate 
    Gradient method.

    Arguments
    ---------

    @param[in]
    A           magma_s_sparse_matrix
                input matrix A

    @param[in]
    b           magma_s_vector
                RHS b - can be a block

    @param[in,out]
    x           magma_s_vector*
                solution approximation

    @param[in,out]
    solver_par  magma_s_solver_par*
                solver parameters

    @param[in]
    precond_par magma_s_preconditioner*
                preconditioner
    @param[in]
    queue       magma_queue_t
                Queue to execute in.

    @ingroup magmasparse_ssysv
    ********************************************************************/

extern "C" magma_int_t
magma_sbpcg(
    magma_s_sparse_matrix A, magma_s_vector b, magma_s_vector *x,  
    magma_s_solver_par *solver_par, 
    magma_s_preconditioner *precond_par,
    magma_queue_t queue )
{
    // set queue for old dense routines
    magma_queue_t orig_queue;
    magmablasGetKernelStream( &orig_queue );
    magma_int_t stat_dev = 0, stat_cpu = 0;
    
    magma_int_t i, num_vecs = b.num_rows/A.num_rows;

    // prepare solver feedback
    solver_par->solver = Magma_PCG;
    solver_par->numiter = 0;
    solver_par->info = MAGMA_SUCCESS;

    // local variables
    float c_zero = MAGMA_S_ZERO, c_one = MAGMA_S_ONE;
    
    magma_int_t dofs = A.num_rows;

    // GPU workspace
    magma_s_vector r, rt, p, q, h;
    magma_s_vinit( &r, Magma_DEV, dofs*num_vecs, c_zero, queue );
    magma_s_vinit( &rt, Magma_DEV, dofs*num_vecs, c_zero, queue );
    magma_s_vinit( &p, Magma_DEV, dofs*num_vecs, c_zero, queue );
    magma_s_vinit( &q, Magma_DEV, dofs*num_vecs, c_zero, queue );
    magma_s_vinit( &h, Magma_DEV, dofs*num_vecs, c_zero, queue );
    
    // solver variables
    float *alpha, *beta;
    alpha = NULL;
    beta = NULL;
    stat_cpu += magma_smalloc_cpu(&alpha, num_vecs);
    stat_cpu += magma_smalloc_cpu(&beta, num_vecs);

    float *nom, *nom0, *r0, *gammaold, *gammanew, *den, *res, *residual;
    nom        = NULL;
    nom0       = NULL;
    r0         = NULL;
    gammaold   = NULL;
    gammanew   = NULL;
    den        = NULL;
    res        = NULL;
    residual   = NULL;
    stat_cpu += magma_smalloc_cpu(&residual, num_vecs);
    stat_cpu += magma_smalloc_cpu(&nom, num_vecs);
    stat_cpu += magma_smalloc_cpu(&nom0, num_vecs);
    stat_cpu += magma_smalloc_cpu(&r0, num_vecs);
    stat_cpu += magma_smalloc_cpu(&gammaold, num_vecs);
    stat_cpu += magma_smalloc_cpu(&gammanew, num_vecs);
    stat_cpu += magma_smalloc_cpu(&den, num_vecs);
    stat_cpu += magma_smalloc_cpu(&res, num_vecs);
    stat_cpu += magma_smalloc_cpu(&residual, num_vecs);
    if( stat_cpu != 0 ){
        magma_free_cpu( nom      );
        magma_free_cpu( nom0     );
        magma_free_cpu( r0       );
        magma_free_cpu( gammaold );
        magma_free_cpu( gammanew );
        magma_free_cpu( den      );
        magma_free_cpu( res      );
        magma_free_cpu( alpha    );
        magma_free_cpu( beta     );
        magma_free_cpu( residual );
        magmablasSetKernelStream( orig_queue );
        printf("error: memory allocation.\n");
        return MAGMA_ERR_HOST_ALLOC;
    }
    // solver setup
    magma_sscal( dofs*num_vecs, c_zero, x->dval, 1) ;                     // x = 0
    magma_scopy( dofs*num_vecs, b.dval, 1, r.dval, 1 );                    // r = b

    // preconditioner
    magma_s_applyprecond_left( A, r, &rt, precond_par, queue );
    magma_s_applyprecond_right( A, rt, &h, precond_par, queue );

    magma_scopy( dofs*num_vecs, h.dval, 1, p.dval, 1 );                 // p = h

    for( i=0; i<num_vecs; i++) {
        nom[i] = MAGMA_S_REAL( magma_sdot(dofs, r(i), 1, h(i), 1) );     
        nom0[i] = magma_snrm2( dofs, r(i), 1 );       
    }
                                          
    magma_s_spmv( c_one, A, p, c_zero, q, queue );                     // q = A p

    for( i=0; i<num_vecs; i++)
        den[i] = MAGMA_S_REAL( magma_sdot(dofs, p(i), 1, q(i), 1) );  // den = p dot q

    solver_par->init_res = nom0[0];
    
    if ( (r0[0] = nom[0] * solver_par->epsilon) < ATOLERANCE ) 
        r0[0] = ATOLERANCE;
    // check positive definite
    if (den[0] <= 0.0) {
        printf("Operator A is not postive definite. (Ar,r) = %f\n", den[0]);
        magmablasSetKernelStream( orig_queue );
        return MAGMA_NONSPD;
        solver_par->info = MAGMA_NONSPD;;
    }
    if ( nom[0] < r0[0] ) {
        magmablasSetKernelStream( orig_queue );
        return MAGMA_SUCCESS;
    }

    //Chronometry
    real_Double_t tempo1, tempo2;
    tempo1 = magma_sync_wtime( queue );
    if ( solver_par->verbose > 0 ) {
        solver_par->res_vec[0] = (real_Double_t)nom0[0];
        solver_par->timing[0] = 0.0;
    }
    
    // start iteration
    for( solver_par->numiter= 1; solver_par->numiter<solver_par->maxiter; 
                                                    solver_par->numiter++ ) {
        // preconditioner
        magma_s_applyprecond_left( A, r, &rt, precond_par, queue );
        magma_s_applyprecond_right( A, rt, &h, precond_par, queue );


        for( i=0; i<num_vecs; i++)
            gammanew[i] = MAGMA_S_REAL( magma_sdot(dofs, r(i), 1, h(i), 1) );  // gn = < r,h>


        if ( solver_par->numiter==1 ) {
            magma_scopy( dofs*num_vecs, h.dval, 1, p.dval, 1 );                    // p = h            
        } else {
            for( i=0; i<num_vecs; i++) {
                beta[i] = MAGMA_S_MAKE(gammanew[i]/gammaold[i], 0.);       // beta = gn/go
                magma_sscal(dofs, beta[i], p(i), 1);            // p = beta*p
                magma_saxpy(dofs, c_one, h(i), 1, p(i), 1); // p = p + h 
            }
        }

        magma_s_spmv( c_one, A, p, c_zero, q, queue );           // q = A p

     //   magma_s_bspmv_tuned( dofs, num_vecs, c_one, A, p.dval, c_zero, q.dval, queue );


        for( i=0; i<num_vecs; i++) {
            den[i] = MAGMA_S_REAL(magma_sdot(dofs, p(i), 1, q(i), 1));    
                // den = p dot q 

            alpha[i] = MAGMA_S_MAKE(gammanew[i]/den[i], 0.);
            magma_saxpy(dofs,  alpha[i], p(i), 1, x->dval+dofs*i, 1); // x = x + alpha p
            magma_saxpy(dofs, -alpha[i], q(i), 1, r(i), 1);      // r = r - alpha q
            gammaold[i] = gammanew[i];

            res[i] = magma_snrm2( dofs, r(i), 1 );
        }

        if ( solver_par->verbose > 0 ) {
            tempo2 = magma_sync_wtime( queue );
            if ( (solver_par->numiter)%solver_par->verbose==0 ) {
                solver_par->res_vec[(solver_par->numiter)/solver_par->verbose] 
                        = (real_Double_t) res[0];
                solver_par->timing[(solver_par->numiter)/solver_par->verbose] 
                        = (real_Double_t) tempo2-tempo1;
            }
        }


        if (  res[0]/nom0[0]  < solver_par->epsilon ) {
            break;
        }
    } 
    tempo2 = magma_sync_wtime( queue );
    solver_par->runtime = (real_Double_t) tempo2-tempo1;
    magma_sresidual( A, b, *x, residual, queue );
    solver_par->iter_res = res[0];
    solver_par->final_res = residual[0];

    if ( solver_par->numiter < solver_par->maxiter) {
        solver_par->info = MAGMA_SUCCESS;
    } else if ( solver_par->init_res > solver_par->final_res ) {
        if ( solver_par->verbose > 0 ) {
            if ( (solver_par->numiter)%solver_par->verbose==0 ) {
                solver_par->res_vec[(solver_par->numiter)/solver_par->verbose] 
                        = (real_Double_t) res[0];
                solver_par->timing[(solver_par->numiter)/solver_par->verbose] 
                        = (real_Double_t) tempo2-tempo1;
            }
        }
        solver_par->info = MAGMA_SLOW_CONVERGENCE;
    }
    else {
        if ( solver_par->verbose > 0 ) {
            if ( (solver_par->numiter)%solver_par->verbose==0 ) {
                solver_par->res_vec[(solver_par->numiter)/solver_par->verbose] 
                        = (real_Double_t) res[0];
                solver_par->timing[(solver_par->numiter)/solver_par->verbose] 
                        = (real_Double_t) tempo2-tempo1;
            }
        }
        solver_par->info = MAGMA_DIVERGENCE;
    }
    for( i=0; i<num_vecs; i++) {
        printf("%.4e  ",res[i]);
    }
    printf("\n");
    for( i=0; i<num_vecs; i++) {
        printf("%.4e  ",residual[i]);
    }
    printf("\n");

    magma_s_vfree(&r, queue );
    magma_s_vfree(&rt, queue );
    magma_s_vfree(&p, queue );
    magma_s_vfree(&q, queue );
    magma_s_vfree(&h, queue );

    magma_free_cpu(alpha);
    magma_free_cpu(beta);
    magma_free_cpu(nom);
    magma_free_cpu(nom0);
    magma_free_cpu(r0);
    magma_free_cpu(gammaold);
    magma_free_cpu(gammanew);
    magma_free_cpu(den);
    magma_free_cpu(res);

    magmablasSetKernelStream( orig_queue );
    return MAGMA_SUCCESS;
}   /* magma_sbpcg */


