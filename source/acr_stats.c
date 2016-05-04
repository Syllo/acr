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

#include "acr/acr_stats.h"

#ifdef ACR_STATS_ENABLED

void acr_print_stats(
    FILE *out,
    struct acr_simulation_time_stats *sim_stats,
    struct acr_threads_time_stats *thread_stats) {
  double simulation_step_time =
    sim_stats->total_time / (double) sim_stats->num_simmulation_step;
  double monitor_mean_time =
    thread_stats->total_time[acr_thread_time_monitor]
    / (double) thread_stats->num_mesurements[acr_thread_time_monitor];
  double cloog_mean_time =
    thread_stats->total_time[acr_thread_time_cloog]
    / (double) thread_stats->num_mesurements[acr_thread_time_cloog];
  double cc_mean_time =
    thread_stats->total_time[acr_thread_time_cc]
    / (double) thread_stats->num_mesurements[acr_thread_time_cc];
  double tcc_mean_time =
    thread_stats->total_time[acr_thread_time_tcc]
    / (double) thread_stats->num_mesurements[acr_thread_time_tcc];
  double monitor_proportion_of_sim_step = monitor_mean_time / simulation_step_time;
  double cloog_proportion_of_sim_step = cloog_mean_time / simulation_step_time;
  double cc_proportion_of_sim_step = cc_mean_time / simulation_step_time;
  double tcc_proportion_of_sim_step = tcc_mean_time / simulation_step_time;
  double total_time_spent = 0.;
  for (int i = 0; i < acr_thread_time_total; ++i) {
    total_time_spent += thread_stats->total_time[i];
  }
  total_time_spent += sim_stats->total_time;
  fprintf(out,
      "\n############ ACR STATISTICS ############\n\n"
      "%29s: %fs\n\n"
      "%29s: %fs\n"
      "%29s: %zu\n"
      "%29s: %fs\n\n"
      "%29s: %fs\n"
      "%29s: %zu\n"
      "%29s: %fs\n\n"
      "%29s: %fs\n"
      "%29s: %zu\n"
      "%29s: %fs\n\n"
      "%29s: %fs\n"
      "%29s: %zu\n"
      "%29s: %fs\n\n"
      "%29s: %fs\n"
      "%29s: %zu\n"
      "%29s: %fs\n\n"
      "%29s: %f\n"
      "%29s: %f\n"
      "%29s: %f\n"
      "%29s: %f\n"
      "\n########################################\n\n",
      "Total time spent",
      total_time_spent,
      "Total simulation time",
      sim_stats->total_time,
      "Total simulation steps",
      sim_stats->num_simmulation_step,
      "Time of one simulation step",
      simulation_step_time,
      "Total monitoring time",
      thread_stats->total_time[acr_thread_time_monitor],
      "Total monitoring loops",
      thread_stats->num_mesurements[acr_thread_time_monitor],
      "Mean time spent in monitoring",
      monitor_mean_time,
      "Total cloog time",
      thread_stats->total_time[acr_thread_time_cloog],
      "Total cloog invocations",
      thread_stats->num_mesurements[acr_thread_time_cloog],
      "Mean time spent in cloog",
      cloog_mean_time,
      "Total cc time",
      thread_stats->total_time[acr_thread_time_cc],
      "Total cc invocations",
      thread_stats->num_mesurements[acr_thread_time_cc],
      "Mean time spent in cc",
      cc_mean_time,
      "Total tcc time",
      thread_stats->total_time[acr_thread_time_tcc],
      "Total tcc invocations",
      thread_stats->num_mesurements[acr_thread_time_tcc],
      "Mean time spent in tcc",
      tcc_mean_time,
      "monitor time / frame time",
      monitor_proportion_of_sim_step,
      "cloog time / frame time",
      cloog_proportion_of_sim_step,
      "cc time / frame time",
      cc_proportion_of_sim_step,
      "tcc time / frame time",
      tcc_proportion_of_sim_step);
}

#endif
