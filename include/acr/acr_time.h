/*
 * Copyright (C) 2016 Maxime Schmitt
 *
 * ACR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 *
 * \file acr_time.h
 * \brief Time functions for performance analysis purpose
 *
 */

#ifndef __ACR_TIME_H
#define __ACR_TIME_H

#if _OPENMP
#include <omp.h>

typedef double acr_time;

static inline void acr_get_current_time(acr_time *time) {
  *time = omp_get_wtime();
}

static inline double acr_difftime(acr_time t0, acr_time t1) {
  return t1 - t0;
}

#else // _OPENMP
#include <time.h>

#ifdef CLOCK_MONOTONIC_RAW
#define ACR_CLOCK CLOCK_MONOTONIC_RAW
#else
#define ACR_CLOCK CLOCK_MONOTONIC
#endif

typedef struct timespec acr_time;

static inline void acr_get_current_time(acr_time *time) {
  clock_gettime(ACR_CLOCK, time);
}

static inline double acr_difftime(acr_time t0, acr_time t1) {
  double secdiff = difftime(t1.tv_sec, t0.tv_sec);
  if (t1.tv_nsec < t0.tv_nsec) {
    long val = 1000000000l - t0.tv_nsec + t1.tv_nsec;
    secdiff += (double)val / 1e9 - 1.;
  } else {
    long val = t1.tv_nsec - t0.tv_nsec;
    secdiff += (double)val / 1e9;
  }
  return secdiff;
}

#endif // _OPENMP

#endif // __ACR_TIME_H
