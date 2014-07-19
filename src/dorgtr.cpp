/*
    -- MAGMA (version 1.5.0-beta3) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date July 2014

       @generated from zungtr.cpp normal z -> d, Fri Jul 18 17:34:18 2014

*/
#include "common_magma.h"

/**
    Purpose
    -------
    DORGTR generates a real unitary matrix Q which is defined as the
    product of n-1 elementary reflectors of order N, as returned by
    DSYTRD:

    if UPLO = MagmaUpper, Q = H(n-1) . . . H(2) H(1),

    if UPLO = MagmaLower, Q = H(1) H(2) . . . H(n-1).

    Arguments
    ---------
    @param[in]
    uplo    magma_uplo_t
      -     = MagmaUpper: Upper triangle of A contains elementary reflectors
                          from DSYTRD;
      -     = MagmaLower: Lower triangle of A contains elementary reflectors
                          from DSYTRD.

    @param[in]
    n       INTEGER
            The order of the matrix Q. N >= 0.

    @param[in,out]
    A       DOUBLE_PRECISION array, dimension (LDA,N)
            On entry, the vectors which define the elementary reflectors,
            as returned by DSYTRD.
            On exit, the N-by-N unitary matrix Q.

    @param[in]
    lda     INTEGER
            The leading dimension of the array A. LDA >= N.

    @param[in]
    tau     DOUBLE_PRECISION array, dimension (N-1)
            TAU(i) must contain the scalar factor of the elementary
            reflector H(i), as returned by DSYTRD.

    @param[out]
    work    (workspace) DOUBLE_PRECISION array, dimension (LWORK)
            On exit, if INFO = 0, WORK(1) returns the optimal LWORK.

    @param[in]
    lwork   INTEGER
            The dimension of the array WORK. LWORK >= N-1.
            For optimum performance LWORK >= N*NB, where NB is
            the optimal blocksize.
    \n
            If LWORK = -1, then a workspace query is assumed; the routine
            only calculates the optimal size of the WORK array, returns
            this value as the first entry of the WORK array, and no error
            message related to LWORK is issued by XERBLA.

    @param[in]
    dT      DOUBLE_PRECISION array on the GPU device.
            DT contains the T matrices used in blocking the elementary
            reflectors H(i) as returned by magma_dsytrd.

    @param[in]
    nb      INTEGER
            This is the block size used in DSYTRD, and correspondingly
            the size of the T matrices, used in the factorization, and
            stored in DT.

    @param[out]
    info    INTEGER
      -     = 0:  successful exit
      -     < 0:  if INFO = -i, the i-th argument had an illegal value

    @ingroup magma_dsyev_comp
    ********************************************************************/
extern "C" magma_int_t
magma_dorgtr(magma_uplo_t uplo, magma_int_t n, double *A,
             magma_int_t lda, double *tau,
             double *work, magma_int_t lwork,
             double *dT, magma_int_t nb,
             magma_int_t *info)
{
#define A(i,j) (A + (j)*lda+ (i))

    magma_int_t i__1;
    magma_int_t i, j;
    magma_int_t iinfo;
    magma_int_t upper, lwkopt, lquery;

    *info = 0;
    lquery = (lwork == -1);
    upper = (uplo == MagmaUpper);
    if (! upper && uplo != MagmaLower) {
        *info = -1;
    } else if (n < 0) {
        *info = -2;
    } else if (lda < max(1,n)) {
        *info = -4;
    } else /* if (complicated condition) */ {
        /* Computing MAX */
        if (lwork < max(1, n-1) && ! lquery) {
            *info = -7;
        }
    }

    lwkopt = max(1, n) * nb;
    if (*info == 0) {
        work[0] = MAGMA_D_MAKE( lwkopt, 0 );
    }

    if (*info != 0) {
        magma_xerbla( __func__, -(*info));
        return *info;
    } else if (lquery) {
        return *info;
    }

    /* Quick return if possible */
    if (n == 0) {
        work[0] = MAGMA_D_ONE;
        return *info;
    }

    if (upper) {
        /*  Q was determined by a call to DSYTRD with UPLO = 'U'
            Shift the vectors which define the elementary reflectors one
            column to the left, and set the last row and column of Q to
            those of the unit matrix                                    */
        for (j = 0; j < n-1; ++j) {
            for (i = 0; i < j-1; ++i)
                *A(i, j) = *A(i, j + 1);

            *A(n-1, j) = MAGMA_D_ZERO;
        }
        for (i = 0; i < n-1; ++i) {
            *A(i, n-1) = MAGMA_D_ZERO;
        }
        *A(n-1, n-1) = MAGMA_D_ONE;
        
        /* Generate Q(1:n-1,1:n-1) */
        i__1 = n - 1;
        lapackf77_dorgql(&i__1, &i__1, &i__1, A(0,0), &lda, tau, work,
                         &lwork, &iinfo);
    } else {
        
        /*  Q was determined by a call to DSYTRD with UPLO = 'L'.
            Shift the vectors which define the elementary reflectors one
            column to the right, and set the first row and column of Q to
            those of the unit matrix                                      */
        for (j = n-1; j > 0; --j) {
            *A(0, j) = MAGMA_D_ZERO;
            for (i = j; i < n-1; ++i)
                *A(i, j) = *A(i, j - 1);
        }

        *A(0, 0) = MAGMA_D_ONE;
        for (i = 1; i < n-1; ++i)
            *A(i, 0) = MAGMA_D_ZERO;
        
        if (n > 1) {
            /* Generate Q(2:n,2:n) */
            magma_dorgqr(n-1, n-1, n-1, A(1, 1), lda, tau, dT, nb, &iinfo);
        }
    }
    
    work[0] = MAGMA_D_MAKE( lwkopt, 0 );

    return *info;
} /* magma_dorgtr */

#undef A
