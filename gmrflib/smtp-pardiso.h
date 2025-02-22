
/* smtp-pardiso.h
 * 
 * Copyright (C) 2018-2022 Havard Rue
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The author's contact information:
 *
 *        Haavard Rue
 *        CEMSE Division
 *        King Abdullah University of Science and Technology
 *        Thuwal 23955-6900, Saudi Arabia
 *        Email: haavard.rue@kaust.edu.sa
 *        Office: +966 (0)12 808 0640
 *
 *
 */

/*!
  \file smtp-band.h
  \brief GMRFLib interface to the band-solver in the LAPACK library
*/

#ifndef __GMRFLib_SMTP_PARDISO_H__
#define __GMRFLib_SMTP_PARDISO_H__

#if !defined(__FreeBSD__)
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS					       /* empty */
#define __END_DECLS					       /* empty */
#endif

__BEGIN_DECLS

/* 
   
 */
#define GMRFLib_PSTORE_TNUM_REF (0)			       /* the reference thread number to store store-spesific things */
#define GMRFLib_PARDISO_PLEN (64)
    typedef enum {
	GMRFLib_PARDISO_FLAG_REORDER = 1,
	GMRFLib_PARDISO_FLAG_SYMFACT,
	GMRFLib_PARDISO_FLAG_CHOL,
	GMRFLib_PARDISO_FLAG_QINV,
	GMRFLib_PARDISO_FLAG_SOLVE_L,
	GMRFLib_PARDISO_FLAG_SOLVE_LT,
	GMRFLib_PARDISO_FLAG_SOLVE_LLT
} GMRFLib_pardiso_flag_tp;

typedef struct {
	int iparm[GMRFLib_PARDISO_PLEN];
	double dparm[GMRFLib_PARDISO_PLEN];
	int done_with_build;
	int done_with_chol;

	double dummy;
	int err_code;
	int idummy;
	int nrhs;
	int phase;
	int L_nnz;
	int *perm;
	int *iperm;
	double log_det_Q;
	GMRFLib_csr_tp *Q;
	GMRFLib_csr_tp *Qinv;
} GMRFLib_pardiso_store_pr_thread_tp;

typedef struct {
	int copy_pardiso_ptr;				       /* set this if the contents is just a copy_ptr */

	void *pt[GMRFLib_PARDISO_PLEN];
	int *iparm_default;
	double *dparm_default;
	int maxfct;
	int done_with_init;
	int done_with_reorder;

	int msglvl;
	int mtype;
	int solver;
	GMRFLib_graph_tp *graph;

	GMRFLib_pardiso_store_pr_thread_tp **pstore;
} GMRFLib_pardiso_store_tp;

#ifdef _WIN32
#include <io.h>
#define NULL_FNM "NUL"
#define CROSS_DUP(fd) _dup(fd)
#define CROSS_DUP2(fd, newfd) _dup2(fd, newfd)
#else
#include <unistd.h>
#define NULL_FNM "/dev/null"
#define CROSS_DUP(fd) dup(fd)
#define CROSS_DUP2(fd, newfd) dup2(fd, newfd)
#endif

#define STDOUT_TO_DEV_NULL_START(_silent)				\
	int XX_silent = _silent;					\
	int XX_stdoutBackupFd;						\
	int XX_stderrBackupFd;						\
        FILE *XX_nullOut;						\
        FILE *XX_nullErr;						\
	if (XX_silent) {						\
		XX_stdoutBackupFd = CROSS_DUP(STDOUT_FILENO);		\
		XX_stderrBackupFd = CROSS_DUP(STDERR_FILENO);		\
		fflush(stdout);						\
		fflush(stderr);						\
		XX_nullOut = fopen(NULL_FNM, "w");			\
		XX_nullErr = fopen(NULL_FNM, "w");			\
		CROSS_DUP2(fileno(XX_nullOut), STDOUT_FILENO);		\
		CROSS_DUP2(fileno(XX_nullErr), STDERR_FILENO);		\
	}

#define STDOUT_TO_DEV_NULL_END						\
	if (XX_silent) {						\
		fflush(stdout);						\
		fflush(stderr);						\
		fclose(XX_nullOut);					\
		fclose(XX_nullErr);					\
		CROSS_DUP2(XX_stdoutBackupFd, STDOUT_FILENO);		\
		CROSS_DUP2(XX_stderrBackupFd, STDERR_FILENO);		\
		close(XX_stdoutBackupFd);				\
		close(XX_stderrBackupFd);				\
	}

double GMRFLib_pardiso_Qfunc_default(int i, int j, double *values, void *arg);
double GMRFLib_pardiso_logdet(GMRFLib_pardiso_store_tp * store);
int GMRFLib_Q2csr(GMRFLib_csr_tp ** csr, GMRFLib_graph_tp * graph, GMRFLib_Qfunc_tp * Qfunc, void *Qfunc_arg);
int GMRFLib_csr2Q(GMRFLib_tabulate_Qfunc_tp ** Qtab, GMRFLib_graph_tp ** graph, GMRFLib_csr_tp * csr);
int GMRFLib_csr_base(int base, GMRFLib_csr_tp * M);
int GMRFLib_csr_check(GMRFLib_csr_tp * M);
int GMRFLib_csr_convert(GMRFLib_csr_tp * M);
int GMRFLib_csr_duplicate(GMRFLib_csr_tp ** csr_to, GMRFLib_csr_tp * csr_from);
int GMRFLib_csr_free(GMRFLib_csr_tp ** csr);
int GMRFLib_csr_print(FILE * fp, GMRFLib_csr_tp * csr);
int GMRFLib_csr_read(char *filename, GMRFLib_csr_tp ** csr);
int GMRFLib_csr_write(char *filename, GMRFLib_csr_tp * csr);
int GMRFLib_duplicate_pardiso_store(GMRFLib_pardiso_store_tp ** new, GMRFLib_pardiso_store_tp * old, int copy_ptr, int copy_pardiso_ptr);
int GMRFLib_pardiso_Qinv(GMRFLib_pardiso_store_tp * store);
int GMRFLib_pardiso_Qinv_INLA();
int GMRFLib_pardiso_bitmap(void);
int GMRFLib_pardiso_build(GMRFLib_pardiso_store_tp * store, GMRFLib_graph_tp * graph, GMRFLib_Qfunc_tp * Qfunc, void *Qfunc_arg);
int GMRFLib_pardiso_check_install(int quiet, int no_err);
int GMRFLib_pardiso_chol(GMRFLib_pardiso_store_tp * store);
int GMRFLib_pardiso_free(GMRFLib_pardiso_store_tp ** store);
int GMRFLib_pardiso_get_nrhs(void);
int GMRFLib_pardiso_init(GMRFLib_pardiso_store_tp ** store);
int GMRFLib_pardiso_iperm(double *x, int m, GMRFLib_pardiso_store_tp * store);
int GMRFLib_pardiso_num_proc();
int GMRFLib_pardiso_perm(double *x, int m, GMRFLib_pardiso_store_tp * store);
int GMRFLib_pardiso_perm_core(double *x, int m, GMRFLib_pardiso_store_tp * store, int direction);
int GMRFLib_pardiso_reorder(GMRFLib_pardiso_store_tp * store, GMRFLib_graph_tp * graph);
int GMRFLib_pardiso_set_debug(int debug);
int GMRFLib_pardiso_set_nrhs(int nrhs);
int GMRFLib_pardiso_set_parallel_reordering(int value);
int GMRFLib_pardiso_set_verbose(int verbose);
int GMRFLib_pardiso_setparam(GMRFLib_pardiso_flag_tp flag, GMRFLib_pardiso_store_tp * store, int *thread_num);
int GMRFLib_pardiso_solve_L(GMRFLib_pardiso_store_tp * store, double *x, double *b, int nrhs);
int GMRFLib_pardiso_solve_LLT(GMRFLib_pardiso_store_tp * store, double *x, double *b, int nrhs);
int GMRFLib_pardiso_solve_LT(GMRFLib_pardiso_store_tp * store, double *x, double *b, int nrhs);
int GMRFLib_pardiso_solve_core(GMRFLib_pardiso_store_tp * store, GMRFLib_pardiso_flag_tp flag, double *x, double *b, int nrhs);
int GMRFLib_pardiso_symfact(GMRFLib_pardiso_store_tp * store);
int GMRFLib_pardiso_exit(void);

int my_pardiso_test1(void);
int my_pardiso_test2(void);
int my_pardiso_test3(void);
int my_pardiso_test4(void);
int my_pardiso_test5(void);
int my_pardiso_test6();
int my_pardiso_test7(void);
double my_pardiso_test_Q(int i, int j, double *values, void *arg);

void pardiso(void *, int *, int *, int *, int *, int *, double *, int *, int *, int *, int *, int *, int *, double *, double *, int *, double *);
void pardiso_chkmatrix(int *, int *, double *, int *, int *, int *);
void pardiso_chkvec(int *, int *, double *, int *);
void pardiso_copy_symbolic_factor_single(void *, void *, int *, int *, double *, double *, int *, int *, int *);
void pardiso_delete_symbolic_factor_single(void *, int *, int *);
void pardiso_get_factor_csc(void **, double *, int *, int *, double *, int *, int *, int *, int);
void pardiso_get_inverse_factor_csc(void **, double *, int *, int *, int *, int);
void pardiso_printstats(int *, int *, double *, int *, int *, int *, double *, int *);
void pardiso_residual(int *mtype, int *n, double *a, int *ia, int *ja, double *b, double *x, double *y, double *norm_b, double *norm_res);
void pardisoinit(void *, int *, int *, int *, double *, int *);

__END_DECLS
#endif
