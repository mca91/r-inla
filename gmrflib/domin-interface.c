
/* domin-interface.c
 * 
 * Copyright (C) 2006-2022 Havard Rue
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
 */

#ifndef GITCOMMIT
#define GITCOMMIT
#endif
static const char GitID[] = "file: " __FILE__ "  " GITCOMMIT;

#include <float.h>
#include <math.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#if !defined(__FreeBSD__)
#include <malloc.h>
#endif
#include <stdlib.h>

#include "GMRFLib/GMRFLib.h"
#include "GMRFLib/GMRFLibP.h"

static GMRFLib_opt_arg_tp G;				       /* hold arguments */
static int opt_setup = 0;

typedef struct {
	double f_best;
	double *f_best_x;
	double *f_best_latent;
} Best_tp;

static Best_tp B = {
	0.0,
	NULL,
	NULL
};

static opt_dir_params_tp Opt_dir_params = {
	NULL,
	NULL,
	0
};

typedef struct {
	double time_used;
	int num_fncall;
} fncall_timing_tp;

static fncall_timing_tp fncall_timing = {
	0.0, 0
};

int GMRFLib_opt_setup(double ***hyperparam, int nhyper,
		      GMRFLib_ai_log_extra_tp * log_extra, void *log_extra_arg,
		      char *compute,
		      double *x, double *b, double *c, double *mean,
		      GMRFLib_bfunc_tp ** bfunc, double *d,
		      GMRFLib_logl_tp * loglFunc, void *loglFunc_arg,
		      GMRFLib_graph_tp * graph, GMRFLib_Qfunc_tp * Qfunc, void *Qfunc_arg,
		      GMRFLib_constr_tp * constr, GMRFLib_ai_param_tp * ai_par, GMRFLib_ai_store_tp * ai_store, GMRFLib_preopt_tp * preopt)
{
	double *theta;
	int i;

	opt_setup = 1;
	fncall_timing.time_used = 0.0;
	fncall_timing.num_fncall = 0;
	G.use_directions = ai_par->optimise_use_directions;
	G.hyperparam = hyperparam;
	G.nhyper = nhyper;
	G.log_extra = log_extra;
	G.log_extra_arg = log_extra_arg;
	G.f_count = Calloc(GMRFLib_MAX_THREADS, int);
	G.compute = compute;
	G.solution = Calloc(nhyper, double);
	G.fvalue = 0.0;
	G.x = x;
	G.b = b;
	G.c = c;
	G.mean = mean;
	G.bfunc = bfunc;
	G.d = d;
	G.loglFunc = loglFunc;
	G.loglFunc_arg = loglFunc_arg;
	G.graph = graph;
	G.directions = ai_par->optimise_use_directions_m;
	G.preopt = preopt;
	G.parallel_linesearch = ai_par->parallel_linesearch;
	G.Qfunc = Calloc(GMRFLib_MAX_THREADS, GMRFLib_Qfunc_tp *);
	G.Qfunc_arg = Calloc(GMRFLib_MAX_THREADS, void *);
	for (i = 0; i < GMRFLib_MAX_THREADS; i++) {
		G.Qfunc[i] = Qfunc;
		G.Qfunc_arg[i] = Qfunc_arg;
	}

	G.constr = constr;
	G.ai_par = ai_par;
	G.ai_store = ai_store;

	theta = Calloc(nhyper, double);
	for (i = 0; i < nhyper; i++) {
		theta[i] = hyperparam[i][GMRFLib_thread_id][0];
	}
	Free(theta);

	return GMRFLib_SUCCESS;

}

int GMRFLib_opt_turn_off_parallel_linesearch()
{
	G.parallel_linesearch = 0;
	return GMRFLib_SUCCESS;
}
int GMRFLib_opt_reset_directions(void)
{
	// restart with diagonal direction matrix
	Opt_dir_params.reset_directions = 1;
	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_get_latent(double *latent)
{
	Memcpy(latent, B.f_best_latent, G.graph->n * sizeof(double));
	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_exit(void)
{
	opt_setup = 0;
	memset(&G, 0, sizeof(GMRFLib_opt_arg_tp));
	memset(&B, 0, sizeof(Best_tp));
	// we want to keep the directions. if the dimension changes then we reset... see below
	if (0) {
		memset(&Opt_dir_params, 0, sizeof(opt_dir_params_tp));
	}
	memset(&fncall_timing, 0, sizeof(fncall_timing_tp));

	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_f(double *x, double *fx, int *ierr, GMRFLib_tabulate_Qfunc_tp ** tabQfunc, double **bnew)
{
	/*
	 * this function is called only for thread=0!!! 
	 */
	GMRFLib_ENTER_ROUTINE;
	GMRFLib_ASSERT(GMRFLib_OPENMP_IN_PARALLEL == 0, GMRFLib_ESNH);
	GMRFLib_ASSERT(GMRFLib_thread_id == 0, GMRFLib_ESNH);
	GMRFLib_ASSERT(omp_get_thread_num() == 0, GMRFLib_ESNH);

	GMRFLib_opt_f_intern(x, fx, ierr, G.ai_store, tabQfunc, bnew);

	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_f_omp(double **x, int nx, double *f, int *ierr)
{
	/*
	 * Evaluate nx function evaluations of f for configuration x[i][0]...x[i][..], in parallel.
	 */

	int i, tmax, *err = NULL;
	GMRFLib_ai_store_tp **ai_store = NULL, *ai_store_reference = NULL;

	GMRFLib_ASSERT(GMRFLib_OPENMP_IN_PARALLEL == 0, GMRFLib_ESNH);
	tmax = GMRFLib_MAX_THREADS;
	ai_store = Calloc(tmax, GMRFLib_ai_store_tp *);
	err = Calloc(nx, int);

	/*
	 * This is the copy that is to be copied 
	 */
	ai_store_reference = GMRFLib_duplicate_ai_store(G.ai_store, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
#pragma omp parallel for private(i) num_threads(GMRFLib_openmp->max_threads_outer)
	for (i = 0; i < nx; i++) {
		int local_err;
		GMRFLib_ai_store_tp *ais = NULL;

		GMRFLib_thread_id = omp_get_thread_num();
		if (GMRFLib_thread_id == 0) {
			ais = G.ai_store;
		} else {
			if (!ai_store[GMRFLib_thread_id]) {
				ai_store[GMRFLib_thread_id] =
				    GMRFLib_duplicate_ai_store(ai_store_reference, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
			}
			ais = ai_store[GMRFLib_thread_id];
		}
		GMRFLib_opt_f_intern(x[i], &f[i], &local_err, ais, NULL, NULL);
		err[i] = err[i] || local_err;
	}
	for (i = 0; i < nx; i++) {
		*ierr = *ierr || err[i];
	}

	GMRFLib_free_ai_store(ai_store_reference);
	for (i = 0; i < tmax; i++) {
		if (ai_store[i]) {
			GMRFLib_free_ai_store(ai_store[i]);
		}
	}
	Free(ai_store);
	Free(err);

	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_f_intern(double *x, double *fx, int *ierr, GMRFLib_ai_store_tp * ais, GMRFLib_tabulate_Qfunc_tp ** tabQfunc, double **bnew)
{
	/*
	 * this version controls AI_STORE 
	 */

	GMRFLib_ENTER_ROUTINE;

	int i, debug = 0;
	double ffx, fx_local;
	double tref = GMRFLib_cpu();

	/*
	 * tabulate Qfunc here. store it in argument 'tagQfunc' if present, otherwise, use local storage. 
	 */
	GMRFLib_tabulate_Qfunc_tp **tabQfunc_local = NULL;
	double **bnew_local = NULL;

	int id = GMRFLib_thread_id;

	if (!tabQfunc) {
		tabQfunc_local = Calloc(GMRFLib_MAX_THREADS, GMRFLib_tabulate_Qfunc_tp *);
	}
	if (!bnew) {
		bnew_local = Calloc(GMRFLib_MAX_THREADS, double *);
	}

	GMRFLib_thread_id = id;
	for (i = 0; i < G.nhyper; i++) {
		G.hyperparam[i][GMRFLib_thread_id][0] = x[i];
	}

	// yes, this also works with 'preopt' as tabulate_Qfunc do not tabulate but just return a ptr-copy
	GMRFLib_tabulate_Qfunc((tabQfunc ? tabQfunc : &(tabQfunc_local[GMRFLib_thread_id])), G.graph,
			       G.Qfunc[GMRFLib_thread_id], G.Qfunc_arg[GMRFLib_thread_id], NULL, NULL, NULL);

	double con, *bnew_ptr = NULL;

	GMRFLib_bnew(&bnew_ptr, &con, G.graph->n, G.b, G.bfunc);
	if (bnew) {
		*bnew = bnew_ptr;
	} else {
		bnew_local[GMRFLib_thread_id] = bnew_ptr;
	}

	GMRFLib_ai_marginal_hyperparam(fx, G.x, bnew_ptr, G.c, G.mean, G.d, G.loglFunc, G.loglFunc_arg,
				       G.graph,
				       (tabQfunc ? (*tabQfunc)->Qfunc : tabQfunc_local[GMRFLib_thread_id]->Qfunc),
				       (tabQfunc ? (*tabQfunc)->Qfunc_arg : tabQfunc_local[GMRFLib_thread_id]->Qfunc_arg), G.constr, G.ai_par, ais,
				       G.preopt);
	*fx += con;					       /* add missing constant due to b = b(theta) */
	ffx = G.log_extra(x, G.nhyper, G.log_extra_arg);

	if (tabQfunc_local) {
		for (i = 0; i < GMRFLib_MAX_THREADS; i++) {
			GMRFLib_free_tabulate_Qfunc(tabQfunc_local[i]);
		}
		Free(tabQfunc_local);
	}
	if (bnew_local) {
		for (i = 0; i < GMRFLib_MAX_THREADS; i++) {
			Free(bnew_local[i]);
		}
		Free(bnew_local);
	}
	*fx += ffx;					       /* add contributions */
	*fx *= -1.0;					       /* opt() do minimisation */
	fx_local = *fx;

	if (debug) {
		printf("\t%d: fx_local %.12g where f_best %.12g (%s)\n", omp_get_thread_num(), fx_local, B.f_best,
		       (fx_local < B.f_best ? "BETTER!" : ""));
	}

	tref = GMRFLib_cpu() - tref;
#pragma omp atomic
	fncall_timing.time_used += tref;
#pragma omp atomic
	fncall_timing.num_fncall++;

	*ierr = 0;
	G.f_count[omp_get_thread_num()]++;

	if (B.f_best == 0.0 || (!(ISNAN(fx_local) || ISINF(fx_local)) && (fx_local < B.f_best))) {
#pragma omp critical
		{
			if (B.f_best == 0.0 || fx_local < B.f_best) {

				if (debug) {
					printf("f_local %g f_best %g\n", fx_local, B.f_best);
				}

				B.f_best = fx_local;

				if (!B.f_best_x) {
					B.f_best_x = Calloc(G.nhyper, double);
				}
				Memcpy(B.f_best_x, x, G.nhyper * sizeof(double));

				if (!B.f_best_latent) {
					B.f_best_latent = Calloc(G.graph->n, double);
				}
				Memcpy(B.f_best_latent, ais->mode, G.graph->n * sizeof(double));

				if (debug) {
					printf("\t%d: set: B.f_best %.12g fx %.12g\n", omp_get_thread_num(), B.f_best, fx_local);
				}
				if (G.ai_par->fp_log) {
					fprintf(G.ai_par->fp_log, "maxld= %.3f fn=%3d theta=", -fx_local, GMRFLib_opt_get_f_count());
					for (i = 0; i < G.nhyper; i++) {
						fprintf(G.ai_par->fp_log, " %.3f", x[i]);
					}

					double m_min = GMRFLib_min_value(ais->mode, G.graph->n, NULL);
					double m_max = GMRFLib_max_value(ais->mode, G.graph->n, NULL);

					fprintf(G.ai_par->fp_log, " [%.1f, %.2f]\n", DMAX(ABS(m_min), ABS(m_max)),
						fncall_timing.time_used / fncall_timing.num_fncall);
					fflush(G.ai_par->fp_log);
					fflush(stdout);	       /* helps for remote inla */
					fflush(stderr);	       /* helps for remote inla */
				}
			}
		}
	}

	GMRFLib_LEAVE_ROUTINE;
	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_gradf(double *x, double *gradx, int *ierr)
{
	int val;

	val = GMRFLib_opt_gradf_intern(x, gradx, NULL, ierr);

	return val;
}

int GMRFLib_opt_gradf_intern(double *x, double *gradx, double *f0, int *ierr)
{
	/*
	 * new implementation more suited for OpenMP. return also, optionally, also a better estimate for f0.
	 */

	int i, tmax, debug = 0, id_save;
	double h = G.ai_par->gradient_finite_difference_step_len, f_zero;
	GMRFLib_ai_store_tp **ai_store = NULL;
	GMRFLib_ai_store_tp *ai_store_reference = NULL;

	GMRFLib_ASSERT(GMRFLib_OPENMP_IN_PARALLEL == 0, GMRFLib_ESNH);
	tmax = GMRFLib_MAX_THREADS;
	ai_store = Calloc(tmax, GMRFLib_ai_store_tp *);
	id_save = GMRFLib_thread_id;

	/*
	 * this is the one to be copied 
	 */

	if (GMRFLib_openmp->max_threads_outer > 1) {
		ai_store_reference = GMRFLib_duplicate_ai_store(G.ai_store, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
	}

	if (G.ai_par->gradient_forward_finite_difference) {
		/*
		 * forward differences 
		 */
		double *f = Calloc(G.nhyper + 1, double);

#pragma omp parallel for private(i) num_threads(GMRFLib_openmp->max_threads_outer)
		for (i = 0; i < G.nhyper + 1; i++) {
			double *xx = NULL;
			int j, err;
			GMRFLib_ai_store_tp *ais = NULL;

			GMRFLib_thread_id = omp_get_thread_num();
			xx = Calloc(G.nhyper, double);
			Memcpy(xx, x, G.nhyper * sizeof(double));

			if (GMRFLib_OPENMP_IN_PARALLEL) {
				if (GMRFLib_thread_id == 0) {
					ais = G.ai_store;
				} else {
					if (!ai_store[GMRFLib_thread_id]) {
						ai_store[GMRFLib_thread_id] =
						    GMRFLib_duplicate_ai_store(ai_store_reference, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
					}
					ais = ai_store[GMRFLib_thread_id];
				}
			} else {
				ais = G.ai_store;
			}

			/*
			 * j is the point 
			 */
			j = i;
			if (j < G.nhyper) {
				GMRFLib_opt_dir_step(xx, j, h);
				GMRFLib_opt_f_intern(xx, &f[j], &err, ais, NULL, NULL);
			} else {
				GMRFLib_opt_f_intern(xx, &f[G.nhyper], &err, ais, NULL, NULL);
			}
			Free(xx);
		}
		GMRFLib_thread_id = id_save;

		/*
		 * then compute the gradient where f0 = f[G.nhyper] 
		 */
		for (i = 0; i < G.nhyper; i++) {
			gradx[i] = (f[i] - f[G.nhyper]) / h;
		}
		GMRFLib_opt_dir_transform_gradient(gradx);

		f_zero = f[G.nhyper];
		if (f0) {
			*f0 = f_zero;
		}

		Free(f);
	} else {
		/*
		 * use central differences. 'Estimate' the f0 using the mean of all difference
		 */
		double *f = NULL, *fm = NULL, *ff = NULL, *ffm = NULL;
		/*
		 * use a five-point stencil instead? 
		 */
		int use_five_point = GMRFLib_FALSE;

		f = Calloc(G.nhyper, double);
		fm = Calloc(G.nhyper, double);
		if (use_five_point) {
			ff = Calloc(G.nhyper, double);
			ffm = Calloc(G.nhyper, double);
		}
#pragma omp parallel for private(i) num_threads(GMRFLib_openmp->max_threads_outer)
		for (i = 0; i < 2 * G.nhyper; i++) {
			int j, err;
			double *xx = NULL;
			GMRFLib_ai_store_tp *ais = NULL;

			j = (i < G.nhyper ? i : i - G.nhyper);

			GMRFLib_thread_id = omp_get_thread_num();
			xx = Calloc(G.nhyper, double);
			Memcpy(xx, x, G.nhyper * sizeof(double));

			if (GMRFLib_OPENMP_IN_PARALLEL) {
				if (GMRFLib_thread_id == 0) {
					ais = G.ai_store;
				} else {
					if (!ai_store[GMRFLib_thread_id]) {
						ai_store[GMRFLib_thread_id] =
						    GMRFLib_duplicate_ai_store(ai_store_reference, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
					}
					ais = ai_store[GMRFLib_thread_id];
				}
			} else {
				ais = G.ai_store;
			}

			if (i < G.nhyper) {
				GMRFLib_opt_dir_step(xx, j, h);
				GMRFLib_opt_f_intern(xx, &f[j], &err, ais, NULL, NULL);
				if (use_five_point) {
					GMRFLib_opt_dir_step(xx, j, h);
					GMRFLib_opt_f_intern(xx, &ff[j], &err, ais, NULL, NULL);
				}
			} else {
				GMRFLib_opt_dir_step(xx, j, -h);
				GMRFLib_opt_f_intern(xx, &fm[j], &err, ais, NULL, NULL);
				if (use_five_point) {
					GMRFLib_opt_dir_step(xx, j, -h);
					GMRFLib_opt_f_intern(xx, &ffm[j], &err, ais, NULL, NULL);
				}
			}
			Free(xx);
			GMRFLib_thread_id = 0;
		}

		if (use_five_point) {
			for (i = 0; i < G.nhyper; i++) {
				gradx[i] = (-ff[i] + 8.0 * f[i] - 8.0 * fm[i] + ffm[i]) / (12.0 * h);
			}
		} else {
			for (i = 0; i < G.nhyper; i++) {
				gradx[i] = (f[i] - fm[i]) / (2.0 * h);
			}
		}
		GMRFLib_opt_dir_transform_gradient(gradx);

		/*
		 * this should be the mean of the means 
		 */
		double sum = 0.0;
		if (use_five_point) {
			for (i = 0; i < G.nhyper; i++) {
				sum += (ff[i] + f[i] + fm[i] + ffm[i]) / 4.0;
			}
		} else {
			for (i = 0; i < G.nhyper; i++) {
				sum += (f[i] + fm[i]) / 2.0;
			}
		}
		f_zero = sum / G.nhyper;
		if (f0) {
			*f0 = f_zero;
		}
		Free(f);
		Free(fm);
		Free(ff);
		Free(ffm);
	}

	GMRFLib_thread_id = id_save;

	GMRFLib_free_ai_store(ai_store_reference);
	for (i = 0; i < tmax; i++) {
		if (ai_store[i]) {
			GMRFLib_free_ai_store(ai_store[i]);
		}
	}
	Free(ai_store);
	*ierr = 0;

	if (debug) {
		printf("\n\tf0       = %20.12f \n", f_zero);
		for (i = 0; i < G.nhyper; i++) {
			printf("\tgradx[%1d] = %20.12f \t x[%1d] = %20.12f\n", i, gradx[i], i, x[i]);
		}
		printf("\n");
	}

	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_estimate_hessian(double *hessian, double *x, double *log_dens_mode, int count)
{
	/*
	 * Estimate the Hessian using finite differences with fixed step_size. chose either central or forward differences. The central-option is somewhat more
	 * expensive than the forward-option and require 3n(n-1) additional function evaluations.
	 *
	 * This version is better suited for OpenMP
	 *
	 * If !mode_known, then revise 'x' and 'log_dens_mode' according to the best mode-configuration found. If a better is found, then return !GMRFLib_SUCCESS
	 * and this routine will be called once more.
	 */

#define F1(result, idx, step, x_store)					\
	if (1) {							\
		if (debug) printf("F1 idx %d step %g\n",  idx, step);	\
		double *xx;					        \
		int err;						\
		xx = Calloc(G.nhyper, double);			        \
		Memcpy(xx, x, G.nhyper*sizeof(double));			\
		GMRFLib_opt_dir_step(xx, idx, step);			\
		GMRFLib_opt_f_intern(xx, &(result), &err, ais, NULL, NULL); \
		if (debug){						\
			int iii;					\
			printf("Estimate Hessian x=[");			\
			for(iii=0; iii<G.nhyper; iii++)			\
				printf(" %.8g", xx[iii]);		\
			printf("] idx=%d step=%g F1 = %.12g\n", idx, step, result); \
		}							\
		if (x_store) {						\
			Memcpy(x_store, xx, G.nhyper*sizeof(double));	\
		}							\
		Free(xx);						\
	}

#define F2(result, idx, step, iidx, sstep)				\
	if (1) {							\
		double *xx;						\
		int err;						\
		if (debug) printf("F2 idx %d %d step %g %g\n",  idx, iidx, step, sstep); \
		xx = Calloc(G.nhyper, double);				\
		Memcpy(xx, x, G.nhyper*sizeof(double));			\
		GMRFLib_opt_dir_step(xx, idx, step);			\
		GMRFLib_opt_dir_step(xx, iidx, sstep);			\
		GMRFLib_opt_f_intern(xx, &(result), &err, ais, NULL, NULL); \
		if (debug){						\
			int iii;					\
			printf("Estimate Hessian x=[");			\
			for(iii=0; iii<G.nhyper; iii++)			\
				printf(" %.8g", xx[iii]);		\
			printf("] idx=%d %d step=%g %g F2 = %.12g\n", idx, iidx, step, sstep, result); \
		}							\
		Free(xx);						\
	}

#define ALLOC_XX_HOLD(len)					\
	{							\
		int _i;						\
		xx_hold = Calloc(len, double *);		\
		for(_i=0; _i<len; _i++) 			\
			xx_hold[_i] = Calloc(G.nhyper, double); \
	}

#define FREE_XX_HOLD(len)			\
	if (xx_hold) {				\
		int _i;				\
		for(_i=0; _i<len; _i++)		\
			Free(xx_hold[_i]);	\
		Free(xx_hold);			\
	}

	GMRFLib_ai_store_tp **ai_store = NULL;
	double h = G.ai_par->hessian_finite_difference_step_len, f0, f0min, *f1 = NULL, *fm1 = NULL, f_best_save, **xx_hold, *xx_min;
	int i, n = G.nhyper, tmax, id, ok = 0, debug = 0, len_xx_hold;

	tmax = GMRFLib_MAX_THREADS;
	id = omp_get_thread_num();
	f1 = Calloc(n, double);
	fm1 = Calloc(n, double);
	memset(hessian, 0, ISQR(n) * sizeof(double));

	ai_store = Calloc(tmax, GMRFLib_ai_store_tp *);

	h *= pow(G.ai_par->stupid_search_factor, count);       /* increase h with the number of trials */
	len_xx_hold = 2 * n + 1;
	ALLOC_XX_HOLD(len_xx_hold);

	int *i2thread = Calloc(len_xx_hold, int);

	if (0) {
		if (G.ai_par->fp_log) {
			fprintf(G.ai_par->fp_log, "Estimate Hessian Part I (%1d) ", 2 * n + 1);
		}
	}
#pragma omp parallel for private(i) num_threads(GMRFLib_openmp->max_threads_outer)
	for (i = 0; i < 2 * n + 1; i++) {
		int j;
		GMRFLib_ai_store_tp *ais = NULL;

		if (0) {
			if (G.ai_par->fp_log) {
				fprintf(G.ai_par->fp_log, "[%1d:%s]", i, (i < n ? "p" : (i < 2 * n ? "m" : "0")));
			}
		}

		GMRFLib_thread_id = omp_get_thread_num();
		if (GMRFLib_OPENMP_IN_PARALLEL) {
			if (!ai_store[GMRFLib_thread_id]) {
				ai_store[GMRFLib_thread_id] = GMRFLib_duplicate_ai_store(G.ai_store, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
			}
			ais = ai_store[GMRFLib_thread_id];
			i2thread[i] = GMRFLib_thread_id;
		} else {
			ais = G.ai_store;
			i2thread[i] = -1;
		}

		if (i < n) {
			j = i;
			F1(f1[j], j, h, xx_hold[i]);
		} else if (i < 2 * n) {
			j = i - n;
			F1(fm1[j], j, -h, xx_hold[i]);
		} else {
			F1(f0, 0, 0.0, xx_hold[i]);
		}
	}
	GMRFLib_thread_id = id;

	/*
	 * If the mode is ok, then all neigbouring points are larger; just check. otherwise, set f0 as the minimum value. 
	 */
	f0min = f0;
	if (debug) {
		P(f0);
	}

	int thread_min;

	xx_min = xx_hold[len_xx_hold - 1];		       /* Yes, this is stored as the last element */
	thread_min = i2thread[len_xx_hold - 1];

	for (i = 0; i < len_xx_hold - 1; i++) {
		int j;

		if (i < n) {
			j = i;
			if (f1[j] < f0min) {
				f0min = f1[j];
				xx_min = xx_hold[i];
				thread_min = i2thread[i];
			}
		} else if (i < 2 * n) {
			j = i - n;
			if (fm1[j] < f0min) {
				f0min = fm1[j];
				xx_min = xx_hold[i];
				thread_min = i2thread[i];
			}
		}
	}

	Free(i2thread);

	if (debug) {
		P(f0min);
		P(thread_min);
	}

	/*
	 * this is a problem, thread_min might not point to G.ai_store, which is used later in the main code. we would like to do a similar thing as below, but
	 * cannot since G.ai_store is pointing to a storage in INLA() which cannot be changed. therefore the optimiser has to be restarted to set things straight!
	 */

	if (G.ai_par->stupid_search_mode && !G.ai_par->mode_known && !ISEQUAL(f0, f0min)) {
		if (debug)
			fprintf(stderr, "%s: (I) Mode not found sufficiently accurate %.8g %.8g\n\n", __GMRFLib_FuncName, f0, f0min);
		f0 = f0min;
		Memcpy(x, xx_min, G.nhyper * sizeof(double));
		*log_dens_mode = -f0;
		ok = 0;
		for (i = 0; i < n; i++) {
			hessian[i + i * n] = (f1[i] - 2 * f0 + fm1[i]) / SQR(h);
		}
	} else {
		/*
		 * just set the mode to the best in any case 
		 */
		f0 = f0min;

		/*
		 * make sure the global reference thinks the same 
		 */
		if (debug)
			printf("set B.f_best from %f to %f\n", B.f_best, f0min);

		B.f_best = f0min;
		Memcpy(B.f_best_x, xx_min, G.nhyper * sizeof(double));

		/*
		 * keep for reference 
		 */
		f_best_save = B.f_best;

		/*
		 * estimate the diagonal terms 
		 */
		for (i = 0; i < n; i++) {
			hessian[i + i * n] = (f1[i] - 2 * f0 + fm1[i]) / SQR(h);
		}

		if (!G.ai_par->hessian_force_diagonal) {
			/*
			 * then the off-diagonal terms 
			 */

			typedef struct {
				int i, j;
			} Idx_tp;
			int nn = (n * (n - 1)) / 2, j, k;
			Idx_tp *idx = NULL;

			idx = Calloc(nn, Idx_tp);
			for (i = k = 0; i < n; i++) {
				for (j = i + 1; j < n; j++) {
					idx[k].i = i;
					idx[k].j = j;
					k++;
				}
			}

#pragma omp parallel for private(k) num_threads(GMRFLib_openmp->max_threads_outer)
			for (k = 0; k < nn; k++) {
				int ii, jj;
				double f11, fm11, f1m1, fm1m1;
				GMRFLib_ai_store_tp *ais = NULL;

				GMRFLib_thread_id = omp_get_thread_num();
				if (GMRFLib_OPENMP_IN_PARALLEL) {
					if (!ai_store[GMRFLib_thread_id]) {
						ai_store[GMRFLib_thread_id] =
						    GMRFLib_duplicate_ai_store(G.ai_store, GMRFLib_TRUE, GMRFLib_TRUE, GMRFLib_FALSE);
					}
					ais = ai_store[GMRFLib_thread_id];
				} else {
					ais = G.ai_store;
				}

				ii = idx[k].i;
				jj = idx[k].j;
				if (G.ai_par->hessian_forward_finite_difference) {
					F2(f11, ii, h, jj, h);
					hessian[ii + jj * n] = hessian[jj + ii * n] = (f11 - f1[ii] - f1[jj] + f0) / SQR(h);
				} else {
					F2(f11, ii, h, jj, h);
					F2(fm11, ii, -h, jj, h);
					F2(f1m1, ii, h, jj, -h);
					F2(fm1m1, ii, -h, jj, -h);
					hessian[ii + jj * n] = hessian[jj + ii * n] = (f11 - fm11 - f1m1 + fm1m1) / 4.0 / SQR(h);
				}
			}
			GMRFLib_thread_id = id;
			Free(idx);
		}
		if (G.ai_par->fp_log) {
			fprintf(G.ai_par->fp_log, "\n");
		}
		if (G.ai_par->stupid_search_mode && !G.ai_par->mode_known && !ISEQUAL(f_best_save, B.f_best)) {
			/*
			 * There is a change 
			 */
			if (debug) {
				fprintf(stderr, "\n%s: (II) Mode not found sufficiently accurate %.8g %.8g\n", __GMRFLib_FuncName, f0, f0min);
			}
			Memcpy(x, B.f_best_x, G.nhyper * sizeof(double));
			*log_dens_mode = -B.f_best;
			ok = 0;
		} else {
			ok = 1;
		}
	}

	GMRFLib_opt_dir_transform_hessian(hessian);

	FREE_XX_HOLD(len_xx_hold);
	Free(f1);
	Free(fm1);
	for (i = 0; i < tmax; i++) {
		if (ai_store[i]) {
			GMRFLib_free_ai_store(ai_store[i]);
		}
	}
	Free(ai_store);
	GMRFLib_thread_id = id;

	return (ok ? GMRFLib_SUCCESS : !GMRFLib_SUCCESS);
#undef F1
#undef F2
#undef ALLOC_XX_HOLD
#undef FREE_XX_HOLD
}

GMRFLib_matrix_tp *GMRFLib_opt_get_directions(void)
{
	if (Opt_dir_params.A) {
		size_t i, j, n;
		GMRFLib_matrix_tp *D;
		n = Opt_dir_params.A->size1;
		D = Calloc(1, GMRFLib_matrix_tp);
		D->nrow = D->ncol = n;
		D->elems = ISQR(n);
		D->A = Calloc(ISQR(n), double);
		for (i = 0; i < n; i++) {
			for (j = 0; j < n; j++) {
				D->A[i + j * n] = gsl_matrix_get(Opt_dir_params.A, i, j);
			}
		}
		return D;
	} else {
		return NULL;
	}
}

int GMRFLib_opt_get_f_count(void)
{
	if (opt_setup) {
		int i, sum;
		for (sum = 0, i = 0; i < GMRFLib_MAX_THREADS; i++) {
			sum += G.f_count[i];
		}
		return sum;
	} else {
		return 0;
	}
}

double GMRFLib_gsl_f(const gsl_vector * v, void *UNUSED(params))
{
	/*
	 * this function is called only for thread=0!!! 
	 */
	GMRFLib_ASSERT(GMRFLib_thread_id == 0, GMRFLib_ESNH);

	double fx, *x;
	int ierr, i;

	x = Calloc(G.nhyper, double);
	for (i = 0; i < G.nhyper; i++) {
		x[i] = gsl_vector_get(v, i);
	}
	GMRFLib_opt_f(x, &fx, &ierr, NULL, NULL);
	Free(x);

	return fx;
}

void GMRFLib_gsl_df(const gsl_vector * v, void *UNUSED(params), gsl_vector * df)
{
	/*
	 * this function is called only for thread=0!!! 
	 */
	GMRFLib_ASSERT_RETVAL(GMRFLib_thread_id == 0, GMRFLib_ESNH, (void) NULL);

	double *x, *gradx;
	int ierr, i;

	x = Calloc(G.nhyper, double);
	gradx = Calloc(G.nhyper, double);
	for (i = 0; i < G.nhyper; i++) {
		x[i] = gsl_vector_get(v, i);
	}
	GMRFLib_opt_gradf(x, gradx, &ierr);
	for (i = 0; i < G.nhyper; i++) {
		gsl_vector_set(df, i, gradx[i]);
	}

	Free(x);
	Free(gradx);
}

void GMRFLib_gsl_fdf(const gsl_vector * v, void *UNUSED(params), double *f, gsl_vector * df)
{
	/*
	 * This function is a merge of the _f and _df function, but we can compute the 'f' value through the gradient
	 * routine and save one function evaluation.
	 *
	 * This function is called only for thread=0!!!
	 */
	GMRFLib_ASSERT_RETVAL(GMRFLib_thread_id == 0, GMRFLib_ESNH, (void) NULL);

	double *x, *gradx;
	int ierr, i;

	x = Calloc(G.nhyper, double);
	gradx = Calloc(G.nhyper, double);
	for (i = 0; i < G.nhyper; i++) {
		x[i] = gsl_vector_get(v, i);
	}
	GMRFLib_opt_gradf_intern(x, gradx, f, &ierr);
	for (i = 0; i < G.nhyper; i++) {
		gsl_vector_set(df, i, gradx[i]);
	}

	Free(x);
	Free(gradx);
}

int GMRFLib_gsl_get_results(double *theta_mode, double *log_dens_mode)
{
	Memcpy(theta_mode, G.solution, G.nhyper * sizeof(double));
	*log_dens_mode = G.fvalue;

	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_dir_step(double *x, int idx, double h)
{
	if (Opt_dir_params.A) {
		size_t n = Opt_dir_params.A->size1, i;
		for (i = 0; i < n; i++) {
			x[i] += h * gsl_matrix_get(Opt_dir_params.A, i, idx);
		}
	} else {
		x[idx] += h;
	}

	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_dir_transform_gradient(double *grad)
{
	if (Opt_dir_params.A) {
		size_t n = Opt_dir_params.A->size1, i, j;
		double *g = Calloc(n, double);
		for (i = 0; i < n; i++) {
			for (j = 0; j < n; j++) {
				g[i] += gsl_matrix_get(Opt_dir_params.tAinv, i, j) * grad[j];
			}
		}
		Memcpy(grad, g, n * sizeof(double));
		Free(g);
	} else {
		// the gradient is now ok as is.
	}

	return GMRFLib_SUCCESS;
}

int GMRFLib_opt_dir_transform_hessian(double *hessian)
{
	if (Opt_dir_params.A) {
		size_t n = Opt_dir_params.A->size1, i, j, ii, jj;
		double *h = Calloc(ISQR(n), double), tmp;

		// H = t(solve(A)) %*% hessian %*% solve(A)
		for (i = 0; i < n; i++) {
			for (j = i; j < n; j++) {
				tmp = 0.0;
				for (ii = 0; ii < n; ii++) {
					for (jj = 0; jj < n; jj++) {
						tmp += gsl_matrix_get(Opt_dir_params.tAinv, i, ii) * hessian[ii + jj * n] * gsl_matrix_get(Opt_dir_params.tAinv, j, jj);	/* yes, 
																						 * swap 
																						 * arguments 
																						 */
					}
				}
				h[i + j * n] = h[j + i * n] = tmp;
			}
		}
		Memcpy(hessian, h, ISQR(n) * sizeof(double));
		Free(h);
	} else {
		// the hessian is ok as is.
	}

	return GMRFLib_SUCCESS;
}

int GMRFLib_gsl_optimize(GMRFLib_ai_param_tp * ai_par)
{
	double step_size = ai_par->gsl_step_size, tol = ai_par->gsl_tol, dx = 0.0;
	size_t i, j;
	int status, iter = 0, iter_max = 1000;

	const gsl_multimin_fdfminimizer_type *T;
	gsl_multimin_fdfminimizer *s;
	gsl_multimin_function_fdf my_func;
	gsl_vector *x = NULL, *xx;

	static gsl_matrix *A = NULL;
	static gsl_matrix *Adir = NULL;
	static gsl_matrix *tAinv = NULL;

	if (G.use_directions) {
		if (Opt_dir_params.reset_directions || !Opt_dir_params.A || (Opt_dir_params.A && Opt_dir_params.A->size1 != (size_t) G.nhyper)) {

			if (!A && !Adir && !tAinv) {
				A = gsl_matrix_alloc(G.nhyper, G.nhyper);
				Adir = gsl_matrix_alloc(G.nhyper, G.nhyper);
				tAinv = gsl_matrix_alloc(G.nhyper, G.nhyper);
			}
			gsl_matrix_set_zero(A);
			gsl_matrix_set_zero(tAinv);
			gsl_matrix_set_identity(Adir);

			if (G.directions && !Opt_dir_params.reset_directions) {
				// start fom here instead
				if (Adir->size1 == G.directions->size1 && Adir->size2 == G.directions->size2) {
					gsl_matrix_memcpy(Adir, G.directions);
				} else {
					fprintf(stderr, "Direction matrix has wrong dimension, ignore. [(%1d,%1d) != (%1d,%1d)]\n",
						(int) G.directions->size1, (int) G.directions->size2, (int) Adir->size1, (int) Adir->size2);
				}
			}
			gsl_matrix_memcpy(A, Adir);
			gsl_matrix_memcpy(tAinv, Adir);
			Opt_dir_params.reset_directions = 0;

		} else {
			// ok
		}
	}

	Opt_dir_params.A = A;
	Opt_dir_params.tAinv = tAinv;

	my_func.n = G.nhyper;
	my_func.f = &GMRFLib_gsl_f;
	my_func.df = &GMRFLib_gsl_df;
	my_func.fdf = &GMRFLib_gsl_fdf;
	my_func.params = (void *) &Opt_dir_params;

	x = gsl_vector_alloc(G.nhyper);
	for (i = 0; i < (size_t) G.nhyper; i++) {
		gsl_vector_set(x, i, G.hyperparam[i][GMRFLib_thread_id][0]);
	}

	// T = gsl_multimin_fdfminimizer_vector_bfgs2; /* GSL version */
	if (G.parallel_linesearch) {
		T = gsl_multimin_fdfminimizer_vector_bfgs4;
	} else {
		T = gsl_multimin_fdfminimizer_vector_bfgs3;
	}
	s = gsl_multimin_fdfminimizer_alloc(T, G.nhyper);
	gsl_multimin_fdfminimizer_set(s, &my_func, x, step_size, tol);

	gsl_vector *x_prev = NULL;
	double f_prev = gsl_multimin_fdfminimizer_minimum(s);

	xx = gsl_multimin_fdfminimizer_x(s);
	x_prev = gsl_vector_alloc(xx->size);
	gsl_vector_memcpy(x_prev, xx);

	int status_g = GSL_CONTINUE;
	int status_f = GSL_CONTINUE;
	int status_x = GSL_CONTINUE;

	do {
		iter++;
		status = gsl_multimin_fdfminimizer_iterate(s);

		status_g = gsl_multimin_test_gradient(s->gradient, ai_par->gsl_epsg);
		double gnrm2 = gsl_blas_dnrm2(s->gradient);

		xx = gsl_multimin_fdfminimizer_x(s);
		if (x_prev) {
			size_t i_s;

			for (i_s = 0, dx = 0.0; i_s < xx->size; i_s++) {
				dx += SQR(gsl_vector_get(xx, i_s) - gsl_vector_get(x_prev, i_s));
			}

			if (G.use_directions) {
				if (G.nhyper == 1) {
					gsl_matrix_set(Adir, 0, 0, 1.0);
				} else {
					if (ISZERO(dx)) {
						// this can happen if we cannot improve. then do nothing.
					} else {
						// shift
						for (j = Adir->size1 - 1; j > 0; j--) {
							for (i = 0; i < Adir->size1; i++) {
								gsl_matrix_set(Adir, i, j, gsl_matrix_get(Adir, i, j - 1));
							}
						}
						for (i_s = 0; i_s < xx->size; i_s++) {
							gsl_matrix_set(Adir, i_s, 0, gsl_vector_get(xx, i_s) - gsl_vector_get(x_prev, i_s));
						}
					}
				}
				gsl_matrix_memcpy(A, Adir);
				GMRFLib_gsl_mgs(A);
				if (G.ai_par->fp_log) {
					fprintf(G.ai_par->fp_log, "New directions for numerical gradient\n");
					for (size_t j = 0; j < A->size2; j++) {
						printf("\t  dir%.2zu", j + 1);
					}
					printf("\n");
					GMRFLib_printf_gsl_matrix(G.ai_par->fp_log, A, "\t %6.3f");
				}
				gsl_matrix_transpose_memcpy(tAinv, A);
				GMRFLib_gsl_ginv(tAinv, GMRFLib_eps(0.5), -1);
			}

			dx = sqrt(dx / xx->size);
			status_x = gsl_multimin_test_size(dx, ai_par->gsl_epsx);
		} else {
			status_x = GSL_CONTINUE;
		}
		if (!x_prev) {
			x_prev = gsl_vector_alloc(xx->size);
		}
		gsl_vector_memcpy(x_prev, xx);

		double df = ABS(f_prev - gsl_multimin_fdfminimizer_minimum(s));
		status_f = gsl_multimin_test_size(df, ai_par->gsl_epsf);
		f_prev = gsl_multimin_fdfminimizer_minimum(s);

		if (ai_par->fp_log) {
			fprintf(ai_par->fp_log, "Iter=%1d ", iter);
			if (status_g != GSL_CONTINUE) {
				fprintf(ai_par->fp_log, "|grad| = %.3g(pass) ", gnrm2);
			} else {
				fprintf(ai_par->fp_log, "|grad|=%.3g ", gnrm2);
			}
			if (status_x != GSL_CONTINUE) {
				fprintf(ai_par->fp_log, "|x-x.old|=%.3g(pass) ", dx);
			} else {
				fprintf(ai_par->fp_log, "|x-x.old|=%.3g ", dx);
			}
			if (status_f != GSL_CONTINUE) {
				fprintf(ai_par->fp_log, "|f-f.old|=%.3g(pass) ", df);
			} else {
				fprintf(ai_par->fp_log, "|f-f.old|=%.3g ", df);
			}
			if (status != GSL_SUCCESS) {
				fprintf(ai_par->fp_log, "Reached numerical limit!");
			}
			fprintf(ai_par->fp_log, "\n");
		}

		if (GMRFLib_request_optimiser_to_stop) {       /* the signal is sent... */
			fprintf(stderr, "\n\n\t%s: got the message to stop the optimiser...\n\n", __GMRFLib_FuncName);
			status_g = status_f = status_x = GSL_SUCCESS;
			break;
		}
	} while ((status_g == GSL_CONTINUE) && (status_f == GSL_CONTINUE) && (status_x == GSL_CONTINUE) && (status == GSL_SUCCESS)
		 && (iter < iter_max));

	xx = gsl_multimin_fdfminimizer_x(s);
	G.fvalue = -gsl_multimin_fdfminimizer_minimum(s);

	for (i = 0; i < (size_t) G.nhyper; i++) {
		G.solution[i] = gsl_vector_get(xx, i);
	}

	gsl_multimin_fdfminimizer_free(s);
	gsl_vector_free(x);
	if (x_prev) {
		gsl_vector_free(x_prev);
	}

	return GMRFLib_SUCCESS;
}
