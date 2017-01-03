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
 * \defgroup acr_time
 *
 * @{
 * \brief Dealing with system time measurements
 *
 */

#ifndef __ACR_TIME_H
#define __ACR_TIME_H

#include <time.h>
#include <stdbool.h>

#ifdef CLOCK_MONOTONIC_RAW
/** \brief Linux specific clock that does not change with ntp and adjtime */
#define ACR_CLOCK CLOCK_MONOTONIC_RAW
#else
/** \brief Type of clock used by the system */
#define ACR_CLOCK CLOCK_MONOTONIC
#endif

/** \brief The time type used by acr */
typedef struct timespec acr_time;

/**
 * \brief Get the current time
 * \param[out] time The object to initialize with the current time
 */
static inline void acr_get_current_time(acr_time *time) {
  clock_gettime(ACR_CLOCK, time);
}

static inline double acr_time_to_double(acr_time time) {
  return (double) time.tv_sec + (double) time.tv_nsec / 1e9;
}

/**
 * \brief Get the difference in seconds between two times
 * \param[in] t0 A time measurement
 * \param[in] t1 A time measurement
 * \return The difference between the two times in seconds
 * \pre t0 must have been measured **before** t1
 */
static inline double acr_difftime(acr_time t0, acr_time t1) {
  double secdiff = (double) (t1.tv_sec - t0.tv_sec);
  long val = t1.tv_nsec - t0.tv_nsec;
  if (t1.tv_nsec < t0.tv_nsec) {
    val += 1000000000l;
    secdiff -= 1.;
  }
  secdiff += (double)val / 1e9;
  return secdiff;
}

/**
 * \brief Get the difference in struct timespec between two times
 * \param[in] t0 A time measurement
 * \param[in] t1 A time measurement
 * \return The difference between the two times in timespec style
 * \pre t0 must have been measured **before** t1
 */
static inline struct timespec acr_difftime_tspec(acr_time t0, acr_time t1) {
  struct timespec t2;
  if (t1.tv_nsec < t0.tv_nsec) {
    t2.tv_sec = -1;
    t2.tv_nsec = 1000000000;
  } else {
    t2.tv_sec = 0;
    t2.tv_nsec = 0;
  }
  t2.tv_nsec += t1.tv_nsec - t0.tv_nsec;
  t2.tv_sec += t1.tv_sec - t0.tv_sec;
  return t2;
}

/**
 * \brief Test if a time happens befor an other
 * \param[in] t0 A time measurement
 * \param[in] t1 A time measurement
 * \retval true if t0 happens befor t1
 * \retval false otherwise
 */
static inline bool acr_time_is_lower(struct timespec t0, struct timespec t1) {
  return t0.tv_sec < t1.tv_sec || (t0.tv_sec == t1.tv_sec && t0.tv_nsec < t1.tv_nsec);
}

#endif // __ACR_TIME_H

/**
 *
 * @}
 *
 */
