/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */

/*  Name: Cheng Zhang
 *  Andrew ID: chengzh1
 */

#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
/* Function to deal with 32 * 32, 64 * 64 and 61 * 67 martix
 * transpose cases respectively
 */
void transpose32(int M, int N, int A[N][M], int B[M][N]);
void transpose64(int M, int N, int A[N][M], int B[M][N]);
void transpose61(int M, int N, int A[N][M], int B[M][N]);
/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);
    /* According to different value of M and N, use different function */
    if (M == 32 && N == 32){
        transpose32(M, N, A, B);
    }else if (M == 64 && N == 64){
       transpose64(M, N, A, B);
    }else{
        transpose61(M, N, A, B);
    }
    
    ENSURES(is_transpose(M, N, A, B));
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

/* 
 * transpose32 - Function to deal with 32 * 32 matrix transpose case
 * In this case, using 8 * 8 block and deal with diagonal sepeartely
 */
void transpose32(int M, int N, int A[N][M], int B[M][N]){
    int kk, jj, k, j, tempDiagonal;
    int f = 0;
    
    for (kk = 0; kk < N; kk += 8) {
        for (jj = 0; jj < M; jj += 8) {
            for (k = kk; k < kk + 8; k++) {
                f = 0;
                for (j = jj; j < jj + 8; j++) {
                /* If the element is a diagonal one, deal with it later */
                    if (k == j){
                        f = 1;
                        tempDiagonal = A[k][j];
                        continue;
                    }
                    B[j][k] = A[k][j];
                }
                if (f == 1)
                    B[k][k] = tempDiagonal;
            }
        }
    }
}

/*
 * transpose64 - Function to deal with 64 * 64 matrix transpose case
 * In this case, first using 8 * 8 block, and then transpose 4 * 4 blocks
 * in the 8 * 8 block one by one. In order to do it, a proper buffer should
 * be found in B.
 */
void transpose64(int M, int N, int A[N][M], int B[M][N]){
    int kk, jj, k, j;
    int a0, a1, a2, a3, a4, a5, a6, a7;
   
    /* Divide into 8 * 8 block
     * Pick up a block in the bottom of B as a buffer
     * Then transpose A to B via buffer
     */
    for (jj = 0; jj < 64; jj += 8){
        for (kk = 0; kk < 64; kk += 8){
            /* Case that can not find buffer, deal with it seperately */
             if (jj == 56 && kk == 48)
                 continue;
            /* The right bottom block, use B[56 to 64][48 to 56] as buffer */
             else if (jj == 56 && kk == 56)
                 a0 = 6;
            /* When the block to transpose is not in the bottom
             * Choose a block that has different column num with 
             * the transpose block in A
             */
             else if (jj < 56){
                 a0 = kk / 8;
                 a0 = (a0 + 1) % 8;
                 if (a0 * 8 == jj)
                     a0 ++;
             /* When the block to transpose is at bottom
              * Choose the next block as buffer
              */
             }else{
                a0 = kk / 8 + 1;
             }
            /* a0 is the column number for the buffer block */
             a0 *= 8;
            
            /* Begin to transpose block in A to block in B via Buffer
             * Tranpose 4 * 4 blocks one by one. That is
             * Transpose: A1, A2    To: B1, B2
             *            A3, A4        B3, B4
             * B1 = T(A1), B2 = T(A3), B3 = T(A2), B4 = T(A4)
             */
            
            /* A1,A2 store to buffer */
            for (k = kk; k < kk + 4; k ++)
                for (j = jj; j < jj + 8; j ++)
                    B[56 + k - kk][a0 + j - jj] = A[k][j];
            /* Tranpose A1 to B1 from buffer */
            for (k = kk; k < kk + 4; k ++)
                for (j = jj; j < jj + 4; j ++)
                    B[j][k] = B[56 + k - kk][a0 + j - jj];
            /* Store A3 to buffer */
            for (k = kk + 4; k < kk + 8; k ++)
                for (j = jj; j < jj + 4; j ++)
                    B[56 + k - kk - 4][a0 + j - jj] = A[k][j];
            /* Tranpose A3 to B2 from buffer */
            for (k = kk + 4; k < kk + 8; k ++)
                for (j = jj; j < jj + 4; j ++)
                    B[j][k] = B[56 + k - kk - 4][a0 + j - jj];
             /* Store A4 to buffer */
            for (k = kk + 4; k < kk + 8; k ++)
                 for (j = jj + 4; j < jj + 8; j ++)
                     B[56 + k - kk - 4][a0 + j - jj - 4] = A[k][j];
           /* Tranpose A2 to B3 from buffer */
           for (k = kk; k < kk + 4; k ++)
                for (j = jj + 4; j < jj + 8; j ++)
                    B[j][k] = B[56 + k - kk][a0 + j - jj];
            /* Tranpose A4 to B4 from buffer */
            for (k = kk + 4; k < kk + 8; k ++)
                for (j = jj + 4; j < jj + 8; j ++)
                    B[j][k] = B[56 + k - kk - 4][a0 + j - jj - 4];
        }
    }
    /* The block of kk = 48 - 56, jj = 56-64 can not find buffer
     * So deal with it using 4 * 8 block sepeartely
     */
    for (kk = 48; kk < 56; kk += 8){
        for (jj = 56; jj < 64; jj += 4){
            for (k = kk; k < kk + 4; k++) {
            /* A[k + 4][j] and A[k][j] use the same set
             * So store A to local variable first and then assign to B
             */
                a0 = A[k][0 + jj];
                a1 = A[k][1 + jj];
                a2 = A[k][2 + jj];
                a3 = A[k][3 + jj];
                a4 = A[k + 4][0 + jj];
                a5 = A[k + 4][1 + jj];
                a6 = A[k + 4][2 + jj];
                a7 = A[k + 4][3 + jj];
                B[0 + jj][k] = a0;
                B[0 + jj][k + 4] = a4;
                B[1 + jj][k] = a1;
                B[1 + jj][k + 4] = a5;
                B[2 + jj][k] = a2;
                B[2 + jj][k + 4] = a6;
                B[3 + jj][k] = a3;
                B[3 + jj][k + 4] = a7;
            }
        }
    }
}

/*
 * transpose61 - Function to deal with 61 * 67 matrix transpose case
 * In this case, I find that using 14 * 3 block can recude miss greatly
 */
void transpose61(int M, int N, int A[N][M], int B[M][N]){
    
    int rowSize = 14, colSize = 3;
    int en = (N / rowSize + 1) * rowSize;
    int em = (M / colSize + 1) * colSize;
    int kk, jj, k, j;
    
    for (kk = 0; kk < en; kk += rowSize) {
        for (jj = 0; jj < em; jj += colSize) {
            for (k = kk; k < kk + rowSize && k < N; k++) {
                for (j = jj; j < jj + colSize && j < M; j++) {
                    B[j][k] = A[k][j];
                }
            }
        }
    }
    
}



