
/* GMRFLibP.h
 * 
 * Copyright (C) 2001-2022 Havard Rue
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
  \file GMRFLibP.h
  \brief Internal include-file for the GMRFLib source.
*/

#ifndef __GMRFLibP_H__
#define __GMRFLibP_H__

#include <assert.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#if !defined(__FreeBSD__)
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <openssl/sha.h>

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
#if __GNUC__ > 7
typedef size_t fortran_charlen_t;
#else
typedef int fortran_charlen_t;
#endif
#define F_ONE ((fortran_charlen_t)1)

// see https://stackoverflow.com/questions/3599160/how-to-suppress-unused-parameter-warnings-in-c
#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_ ## x
#endif
#ifdef __GNUC__
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
#define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif

typedef enum {
	GMRFLib_MODE_CLASSIC = 1,
	GMRFLib_MODE_TWOSTAGE,
	GMRFLib_MODE_TWOSTAGE_PART1,
	GMRFLib_MODE_TWOSTAGE_PART2,
	GMRFLib_MODE_EXPERIMENTAL
} GRMFLib_preopt_mode_tp;

#define GMRFLib_MODE_NAME() (GMRFLib_inla_mode == GMRFLib_MODE_CLASSIC ? "Classic" : \
			     (GMRFLib_inla_mode == GMRFLib_MODE_TWOSTAGE ? "TwoStage" : \
			      (GMRFLib_inla_mode == GMRFLib_MODE_TWOSTAGE_PART1 ? "TwoStage Part1" : \
			       (GMRFLib_inla_mode == GMRFLib_MODE_TWOSTAGE_PART2 ? "TwoStage Part2" : \
				(GMRFLib_inla_mode == GMRFLib_MODE_EXPERIMENTAL ? "Experimental" : "(UNKNOWN MODE)")))))

#define GMRFLib_SHA_TP         SHA256_CTX
#define GMRFLib_SHA_DIGEST_LEN SHA256_DIGEST_LENGTH
#define GMRFLib_SHA_Init       SHA256_Init
#define GMRFLib_SHA_Update     SHA256_Update
#define GMRFLib_SHA_Final      SHA256_Final
#define GMRFLib_SHA_UPDATE_LEN 64L
#define GMRFLib_SHA_UPDATE_CORE(_x, _len, _type) \
	if ((_len) > 0 && (_x)) {					\
		size_t len = (_len) * sizeof(_type);			\
		size_t n = (size_t) len / GMRFLib_SHA_UPDATE_LEN;	\
		size_t m = len - n * GMRFLib_SHA_UPDATE_LEN;		\
		unsigned char *xx = (unsigned char *) (_x);		\
		for(size_t i = 0; i < n; i++) {				\
			GMRFLib_SHA_Update(&c, (const void *) (xx + i * GMRFLib_SHA_UPDATE_LEN), (size_t) GMRFLib_SHA_UPDATE_LEN); \
		}							\
		if (m) {						\
			GMRFLib_SHA_Update(&c, (const void *) (xx + n * GMRFLib_SHA_UPDATE_LEN), m); \
		}							\
	}
#define GMRFLib_SHA_IUPDATE(_x, _len) GMRFLib_SHA_UPDATE_CORE(_x, _len, int)
#define GMRFLib_SHA_DUPDATE(_x, _len) GMRFLib_SHA_UPDATE_CORE(_x, _len, double)

// utility functions for this are mostly in smtp-pardiso.c
typedef struct {
	int n;
	int na;
	int base;					       /* 0 or 1 */
	int *ia;
	int *ja;
	double *a;
	int copy_only;
} GMRFLib_csr_tp;

typedef struct {
	double *x;
	int free;
} GMRFLib_vec_tp;

typedef enum {
	INLA_B_STRATEGY_SKIP = 0,
	INLA_B_STRATEGY_KEEP = 1
} inla_b_strategy_tp;

/* 
   here are the wrappers for calling functions which return the error-code if it fails
*/
#define GMRFLib_EWRAP__intern(func_call, leave)		\
	if (1) {					\
		int rretval;				\
		rretval = func_call;			\
		if (rretval != GMRFLib_SUCCESS){	\
			GMRFLib_ERROR(rretval);			\
			return rretval;				\
		}						\
	}
#define GMRFLib_EWRAP_MAPKIT__intern(func_call, leave)		\
	if (1){							\
		int rrretval;						\
		rrretval = func_call;					\
		if (rrretval != MAPKIT_OK){				\
			char *msg;					\
			GMRFLib_EWRAP__intern(GMRFLib_sprintf(&msg, "Mapkit-library returned error-code [%1d]", rrretval), leave); \
			GMRFLib_ERROR_MSG(GMRFLib_EMAPKIT, msg);	\
			Free(msg);					\
			return GMRFLib_EMAPKIT;				\
		}							\
	}
#define GMRFLib_EWRAP_GSL__intern(func_call, leave)			\
	if (1){								\
		int rrretval;						\
		gsl_error_handler_t *ehandler = gsl_set_error_handler_off(); \
		rrretval = func_call;					\
		gsl_set_error_handler(ehandler);			\
		if (rrretval != GSL_SUCCESS){				\
			char *msg;					\
			GMRFLib_EWRAP__intern(GMRFLib_sprintf(&msg, "GSL-library returned error-code [%1d]", rrretval), leave); \
			GMRFLib_ERROR_MSG(GMRFLib_EGSL, msg);		\
			Free(msg);					\
			return GMRFLib_EGSL;				\
		}							\
	}
#define GMRFLib_EWRAP_GSL_PTR__intern(func_call, leave)			\
	if (1){								\
		gsl_error_handler_t *ehandler = gsl_set_error_handler_off(); \
		void *retval_ptr = (void *)(func_call);			\
		gsl_set_error_handler(ehandler);			\
		if (retval_ptr == NULL){				\
			char *msg;					\
			GMRFLib_EWRAP__intern(GMRFLib_sprintf(&msg, "GSL-library call returned NULL-pointer"), leave); \
			GMRFLib_ERROR_MSG(GMRFLib_EMEMORY, msg);	\
			Free(msg);					\
			return GMRFLib_EMEMORY;				\
		}							\
	}
#define GMRFLib_EWRAP0(func_call) GMRFLib_EWRAP__intern(func_call, 0)
#define GMRFLib_EWRAP1(func_call) GMRFLib_EWRAP__intern(func_call, 1)
#define GMRFLib_EWRAP0_MAPKIT(func_call) GMRFLib_EWRAP_MAPKIT__intern(func_call, 0)
#define GMRFLib_EWRAP1_MAPKIT(func_call) GMRFLib_EWRAP_MAPKIT__intern(func_call, 1)
#define GMRFLib_EWRAP0_GSL(func_call) GMRFLib_EWRAP_GSL__intern(func_call, 0)
#define GMRFLib_EWRAP1_GSL(func_call) GMRFLib_EWRAP_GSL__intern(func_call, 1)
#define GMRFLib_EWRAP0_GSL_PTR(func_call) GMRFLib_EWRAP_GSL_PTR__intern(func_call, 0)
#define GMRFLib_EWRAP1_GSL_PTR(func_call) GMRFLib_EWRAP_GSL_PTR__intern(func_call, 1)

/* 
   this simply measure the cpu spend in `expressions' and print statistics every occation. there is also a BEGIN/END variant for
   larger blocks.
*/
#define GMRFLib_MEASURE_CPU(msg, expression)		\
	if (1) {					\
		static double _tacc = 0.0;		\
		static int _ntimes = 0;			\
		double _tref;				\
		_tref = GMRFLib_cpu();			\
		_ntimes++;				\
		expression;						\
		_tacc += GMRFLib_cpu() - _tref;				\
		printf("%s:%s:%d: cpu accumulative [%s] %.6f mean %.8f n %d\n", \
		       __FILE__, __GMRFLib_FuncName, __LINE__, msg, _tacc, _tacc/_ntimes, _ntimes); \
	}
#define GMRFLib_MEASURE_CPU_BEGIN()			\
	if (1) {					\
		static double _tacc = 0.0;		\
		static int _ntimes = 0;			\
		double _tref;				\
		_tref = GMRFLib_cpu();			\
		_ntimes++;
#define GMRFLib_MEASURE_CPU_END(msg)					\
	_tacc += GMRFLib_cpu() - _tref;					\
	printf("%s:%s:%d: cpu accumulative [%s] %.6f mean %.8f n %d\n",	\
	       __FILE__, __GMRFLib_FuncName, __LINE__, msg, _tacc, _tacc/_ntimes, _ntimes); \
	}

#define GMRFLib_DEBUG_INIT() static int debug_ = -1;			\
	static int debug_count_ = 0;					\
	_Pragma("omp threadprivate(debug_count_)")			\
	debug_count_++;							\
	if (debug_ < 0)	{						\
		debug_ = GMRFLib_debug_functions(__GMRFLib_FuncName); \
	}

#define GMRFLib_DEBUG_IF_TRUE() (debug_)
#define GMRFLib_DEBUG_IF()      (debug_ > 0 && !((debug_count_ - 1) % debug_))

#define GMRFLib_DEBUG(msg_)						\
	if (debug_ && !((debug_count_ - 1) % debug_)) {			\
		printf("\t[%1d] %s:%1d (%s): %s\n", omp_get_thread_num(), __FILE__, __LINE__, GMRFLib_debug_functions_strip(__GMRFLib_FuncName), msg_); \
	}								\

#define GMRFLib_DEBUG_i(msg_, i_)					\
	if (debug_ && !((debug_count_ - 1) % debug_)) {			\
		printf("\t[%1d] %s:%1d (%s): %s %d\n", omp_get_thread_num(), __FILE__, __LINE__, GMRFLib_debug_functions_strip(__GMRFLib_FuncName), msg_, _i); \
	}

#define GMRFLib_DEBUG_d(msg_, d_)					\
	if (debug_ && !((debug_count_ - 1) % debug_)) {			\
		printf("\t[%1d] %s:%1d (%s): %s %.4f\n", omp_get_thread_num(), __FILE__, __LINE__, GMRFLib_debug_functions_strip(__GMRFLib_FuncName), msg_, d_); \
	}

#define GMRFLib_DEBUG_id(msg_, i_, d_, a_)				\
	if (debug_ && !((debug_count_ - 1) % debug_)) {			\
		printf("\t[%1d] %s:%1d (%s): %s %d %.4f %.4f\n", omp_get_thread_num(), __FILE__, __LINE__, GMRFLib_debug_functions_strip(__GMRFLib_FuncName), msg_, i_, d_, a_); \
	}

#define Calloc_init(n_)							\
	size_t calloc_len_ = (size_t)(n_);				\
	size_t calloc_offset_ = 0;					\
	double *calloc_work_ = (calloc_len_ ? Calloc(calloc_len_, double) : NULL)
#define iCalloc_init(n_)						\
	size_t icalloc_len_ = (size_t)(n_);				\
	size_t icalloc_offset_ = 0;					\
	int *icalloc_work_ = (icalloc_len_ ? Calloc(icalloc_len_, int) : NULL)
#define Calloc_get(_n)				\
	calloc_work_ + calloc_offset_;		\
	calloc_offset_ += (size_t)(_n);		\
	Calloc_check()
#define iCalloc_get(_n)				\
	icalloc_work_ + icalloc_offset_;	\
	icalloc_offset_ += (size_t)(_n);	\
	iCalloc_check()
#define Calloc_check()  if (!(calloc_offset_ <= calloc_len_)) { P(calloc_offset_); P(calloc_len_); }; assert(calloc_offset_ <= calloc_len_)
#define iCalloc_check() if (!(icalloc_offset_ <= icalloc_len_)) { P(icalloc_offset_); P(icalloc_len_); }; assert(icalloc_offset_ <= icalloc_len_)
#define Calloc_free()   if (1) { Calloc_check(); Free(calloc_work_);}
#define iCalloc_free()  if (1) { iCalloc_check(); Free(icalloc_work_); }

/* 
   for ..SAFE_SIZE see:  https://gcc.gnu.org/bugzilla//show_bug.cgi?id=85783
*/
#define GMRFLib_ALLOC_SAFE_SIZE(n_, type_) ((size_t)(n_) * sizeof(type_) < PTRDIFF_MAX ? (size_t)(n_) : (size_t)1)
#if 0
//#define GMRFLib_TRACE_MEMORY    1000000   // trace memory larger than this ammount. undefine it to disable this feature.
#define Calloc(n, type)         (type *)GMRFLib_calloc(GMRFLib_ALLOC_SAFE_SIZE(n, type), sizeof(type), __FILE__, __GMRFLib_FuncName, __LINE__, GitID)
#define Malloc(n, type)         (type *)GMRFLib_malloc(GMRFLib_ALLOC_SAFE_SIZE((n) * sizeof(type), char), __FILE__, __GMRFLib_FuncName, __LINE__, GitID)
#define Realloc(ptr, n, type)   (type *)GMRFLib_realloc((void *)ptr, GMRFLib_ALLOC_SAFE_SIZE((n)*sizeof(type), char), __FILE__, __GMRFLib_FuncName, __LINE__, GitID)
#define Free(ptr)               {GMRFLib_free((void *)(ptr), __FILE__, __GMRFLib_FuncName, __LINE__, GitID); ptr=NULL;}
#define Memcpy(dest, src, n)    GMRFLib_memcpy(dest, src, n)
#else
#undef  GMRFLib_TRACE_MEMORY
#define Calloc(n, type)         (type *)calloc(GMRFLib_ALLOC_SAFE_SIZE(n, type), sizeof(type))
#define Malloc(n, type)         (type *)malloc(GMRFLib_ALLOC_SAFE_SIZE((n) * sizeof(type), char))
#define Realloc(ptr, n, type)   (type *)realloc((void *)ptr, GMRFLib_ALLOC_SAFE_SIZE((n) * sizeof(type), char))
#define Free(ptr)               {free((void *)(ptr)); ptr=NULL;}
#define Memcpy(dest, src, n)    memcpy((void *) (dest), (void *) (src), GMRFLib_ALLOC_SAFE_SIZE(n, char))
#endif

/* 
   ABS is for double, IABS is for int, and so on.
*/
#define ABS(x)   fabs(x)
#define DMAX(a,b) GSL_MAX_DBL(a, b)
#define DMIN(a,b) GSL_MIN_DBL(a, b)
#define TRUNCATE(x, low, high)  DMIN( DMAX(x, low), high)      /* ensure that x is in the inteval [low,high] */
#define SQR(x) gsl_pow_2(x)
#define IABS(x)   abs(x)
#define IMAX(a,b) GSL_MAX_INT(a, b)
#define IMIN(a,b) GSL_MIN_INT(a, b)
#define ITRUNCATE(x, low, high) IMIN(IMAX(x, low), high)
#define ISQR(x) ((x)*(x))
#define MOD(i,n)  (((i)+(n))%(n))
#define FIXME( msg) if (1) { printf("\n[%1d]:%s:%1d:%s: FIXME [%s]\n",  omp_get_thread_num(), __FILE__, __LINE__, __GMRFLib_FuncName,(msg?msg:""));	}
#define FIXME1(msg) if (1) { static int first=1; if (first) { first=0; FIXME(msg); }}
#define FIXMEstderr( msg) if (1) { fprintf(stderr, "\n[%1d]:%s:%1d:%s: FIXME [%s]\n",  omp_get_thread_num(), __FILE__, __LINE__, __GMRFLib_FuncName,(msg?msg:""));	}
#define FIXME1stderr(msg) if (1) { static int first=1; if (first) { first=0; FIXMEstderr(msg); }}
#define P(x)        if (1) { printf("line[%1d] " #x " = [ %.12f ]\n",__LINE__,(double)(x)); }
#define Pstderr(x)  if (1) { fprintf(stderr, "line[%1d] " #x " = [ %.12f ]\n",__LINE__,(double)(x)); }
#define P1(x)       if (1) { static int first=1;  if (first) { printf("line[%1d] " #x " = [ %.12f ]\n", __LINE__, (double)(x)); first=0; }}
#define P1stderr(x) if (1) { static int first=1;  if (first) { fprintf(stderr, "line[%1d] " #x " = [ %.12f ]\n", __LINE__, (double)(x)); first=0; }}
#define PP(msg,pt) if (1) { fprintf(stdout, "%d: %s ptr " #pt " = %p\n", __LINE__, msg, pt); }
#define PPstderr(msg,pt)  if (1) { fprintf(stderr, "%d: %s ptr " #pt " = %p\n", __LINE__, msg, pt); }
#define PPg(msg,pt) if (1) { fprintf(stdout, "%d: %s value " #pt " = %g\n", __LINE__, msg, pt); }
#define PPstderrg(msg,pt) if (1) { fprintf(stderr, "%d: %s value " #pt " = %g\n", __LINE__, msg, pt); }
#define ISINF(x) gsl_isinf(x)
#define ISNAN(x) gsl_isnan(x)
#define ISZERO(x) (gsl_fcmp(x, 0.0, DBL_EPSILON) == 0)
#define ISEQUAL(x, y) (gsl_fcmp(x, y, DBL_EPSILON) == 0)
#define LEGAL(i, n) ((i) >= 0 && (i) < (n))
#define SIGN(x) ((x) >= 0 ? 1.0 : -1.0)

#define GMRFLib_Phi(_x) gsl_cdf_ugaussian_P(_x)
#define GMRFLib_Phi_inv(_x) gsl_cdf_ugaussian_Pinv(_x)
#define GMRFLib_erf(_x) (2.0 * GMRFLib_Phi((_x)*M_SQRT2) - 1.0)
#define GMRFLib_erf_inv(_x) (M_SQRT1_2 * GMRFLib_Phi_inv(((_x) + 1.0)/2.0))
#define GMRFLib_erfc(_x) (2.0 * GMRFLib_Phi(- (_x) * M_SQRT2))
#define GMRFLib_erfc_inv(_x) (- GMRFLib_Phi_inv((_x) / 2.0) * M_SQRT1_2)

#define GMRFLib_GLOBAL_NODE(n, gptr) ((int) IMIN((n-1)*(gptr ? (gptr)->factor :  GMRFLib_global_node.factor), \
						 (gptr ? (gptr)->degree : GMRFLib_global_node.degree)))

#define GMRFLib_STOP_IF_NAN_OR_INF(value, idx, jdx)			\
	if (ISNAN(value) || ISINF(value)) {				\
		if (!nan_error)						\
			fprintf(stdout,					\
				"\n\t%s\n\tFunction: %s(), Line: %1d, Thread: %1d\n\tVariable evaluates to NAN or INF. idx=(%1d,%1d). I will try to fix it...", \
				GitID, __GMRFLib_FuncName, __LINE__, omp_get_thread_num(), idx, jdx); \
		nan_error = 1;						\
	}

#define GMRFLib_SET_PREC(arg_)						\
	(arg_->prec ? *(arg_->prec)					\
	 : (arg_->log_prec ? exp(*(arg_->log_prec))			\
	    : (arg_->log_prec_omp ? exp(*(arg_->log_prec_omp[GMRFLib_thread_id])) : 1.0)))

#define GMRFLib_SET_RANGE(arg_)						\
	(arg_->range ? *(arg_->range)					\
	 : (arg_->log_range ? exp(*(arg_->log_range))			\
	    : (arg_->log_range_omp ? exp(*(arg_->log_range_omp[GMRFLib_thread_id])) : 1.0)))

// This is for internal caching
#define GMRFLib_CACHE_LEN (ISQR(GMRFLib_MAX_THREADS))
#define GMRFLib_CACHE_SET_ID(_id) _id = (omp_get_level() == 2 ? \
					 ((omp_get_ancestor_thread_num(omp_get_level()-1) * \
					   omp_get_team_size(omp_get_level()) + \
					   omp_get_thread_num()) +	\
					  GMRFLib_MAX_THREADS * GMRFLib_thread_id) : \
					 (omp_get_thread_num() + GMRFLib_MAX_THREADS * GMRFLib_thread_id)); \
	assert((_id) < GMRFLib_CACHE_LEN); assert((_id) >= 0)


// len_work_ * n_work_ >0 will create n_work_ workspaces for all threads, each of (len_work_ * n_work_) doubles. _PTR(i_) will return the ptr to
// the thread spesific workspace index i_ and _ZERO will zero-set it, i_=0,,,n_work_-1. CODE_BLOCK_THREAD_ID must be used to set
// GMRFLib_thread_id in the parallel loop and GMRFLib_thread_id is reset automatically afterwards

#define CODE_BLOCK_WORK_PTR(i_work_) (work__ + (size_t) (i_work_) * len_work__ + (size_t) (nt__ == 1 ? 0 : omp_get_thread_num()) * len_work__ * n_work__)
#define CODE_BLOCK_WORK_ZERO(i_work_) memset(CODE_BLOCK_WORK_PTR(i_work_), 0, (size_t) len_work__ * sizeof(double))
#define CODE_BLOCK_SET_THREAD_ID GMRFLib_thread_id = id__
#define RUN_CODE_BLOCK(thread_max_, n_work_, len_work_)			\
	if (1) {							\
		int id__ = GMRFLib_thread_id;				\
		int nt__ = (GMRFLib_OPENMP_IN_PARALLEL ? GMRFLib_openmp->max_threads_inner : GMRFLib_openmp->max_threads_outer); \
		int tmax__ = thread_max_;				\
		int len_work__ = len_work_;				\
		int n_work__ = n_work_;					\
		nt__ = IMAX(1, IMIN(nt__, tmax__));			\
		double * work__ = ((len_work__ * n_work__) > 0 ? Calloc(len_work__ * n_work__ * nt__, double) : NULL); \
		if (nt__ > 1) {						\
			_Pragma("omp parallel for num_threads(nt__) schedule(static)") \
				CODE_BLOCK;				\
		} else {						\
			CODE_BLOCK;					\
		}							\
		Free(work__);						\
		CODE_BLOCK_SET_THREAD_ID;				\
        }

/* from /usr/include/assert.h. use __GMRFLib_FuncName to define name of current function.

   Version 2.4 and later of GCC define a magical variable `__PRETTY_FUNCTION__' which contains the
   name of the function currently being defined.  This is broken in G++ before version 2.6.  C9x has
   a similar variable called __func__, but prefer the GCC one since it demangles C++ function names.
*/
#ifndef __GNUC_PRERQ
#if defined __GNUC__ && defined __GNUC_MINOR__
#define __GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define __GNUC_PREREQ(maj, min) 0
#endif
#endif
#if defined __GNUC__
#if defined __cplusplus ? __GNUC_PREREQ (2, 6) : __GNUC_PREREQ (2, 4)
#define  __GMRFLib_FuncName   __PRETTY_FUNCTION__
#else
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#define __GMRFLib_FuncName  __func__
#else
#define __GMRFLib_FuncName  ((const char *) "(function-name unavailable)")
#endif
#endif
#else
#if defined(__sun)					       /* it works here */
#define __GMRFLib_FuncName __func__
#else
#define __GMRFLib_FuncName ((const char *) "(function-name unavailable)")
#endif
#endif

// from https://en.wikipedia.org/wiki/Inline_function
#ifdef _MSC_VER
#define forceinline __forceinline
#elif defined(__GNUC__)
#define forceinline inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
#define forceinline inline __attribute__((__always_inline__))
#else
#define forceinline inline
#endif
#else
#define forceinline inline
#endif

/* 
   parts taken from /usr/include/tcl.h
 */
#ifdef __STRING
#define __GMRFLib_STRINGIFY(x) __STRING(x)
#else
#ifdef _MSC_VER
#define __GMRFLib_STRINGIFY(x) #x
#else
#ifdef RESOURCE_INCLUDED
#define __GMRFLib_STRINGIFY(x) #x
#else
#ifdef __STDC__
#define __GMRFLib_STRINGIFY(x) #x
#else
#define __GMRFLib_STRINGIFY(x) "x"
#endif
#endif
#endif
#endif
__END_DECLS
#endif
