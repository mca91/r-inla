
/* interpol.h
 * 
 * Copyright (C) 2011-2022 Havard Rue
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
#ifndef __GMRFLib_INTERPOL_H__
#define __GMRFLib_INTERPOL_H__
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
 *
 */
#define GMRFLib_SN_SKEWMAX (0.988)
    typedef struct {
	double xmin;
	double xmax;
	gsl_interp_accel *accel;
	gsl_spline *spline;
} GMRFLib_spline_tp;

GMRFLib_spline_tp *GMRFLib_spline_create(double *x, double *y, int n);
GMRFLib_spline_tp *GMRFLib_spline_create_from_matrix(GMRFLib_matrix_tp * M);
double GMRFLib_spline_eval(double x, GMRFLib_spline_tp * s);
double GMRFLib_spline_eval_deriv(double x, GMRFLib_spline_tp * s);
double GMRFLib_spline_eval_deriv2(double x, GMRFLib_spline_tp * s);
int GMRFLib_spline_free(GMRFLib_spline_tp * s);

__END_DECLS
#endif
