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

#include "acr/acr_runtime_build.h"
#include "acr/compiler_name.h"
#include "acr/acr_runtime_data.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static const size_t acr_temporary_file_length = 29;
static const char acr_temporary_file_prefix[] = "/tmp/acr-runtime-temp-XXXXXX";

static const size_t num_additional_flags = 7;
static char* acr_compiler_shared_lib_flags[] = {
  [0] = "-pipe",
  [1] = "-fPIC",
  [2] = "-shared",
  [3] = "-I.",
  [4] = "-xc",
  [5] = "-",
  [6] = "-o",
};

void acr_append_necessary_compile_flags(
    size_t *num_options,
    char ***options) {
  const size_t old_size = *num_options;
  *num_options += num_additional_flags + 3;
  *options = realloc(*options, *num_options * sizeof(**options));
  char** new_pos = &((*options)[old_size+1]);
  for (size_t i = old_size; i > 0; --i) {
    (*options)[i] = (*options)[i-1];
  }
  (*options)[0] = acr_system_compiler_path;
  for (size_t i = 0; i < num_additional_flags; ++i) {
    new_pos[i] = acr_compiler_shared_lib_flags[i];
  }
  (*options)[*num_options-2] = NULL;
  (*options)[*num_options-1] = NULL;
}

char* acr_compile_with_system_compiler(
    char *requested_filename,
    const char *string_to_compile,
    size_t num_options,
    char** options) {

  char *output_filename;
  if (requested_filename) {
    output_filename = requested_filename;
  } else {
    output_filename =
      malloc(acr_temporary_file_length * sizeof(*output_filename));
    memcpy(output_filename, acr_temporary_file_prefix, acr_temporary_file_length);
    int fd = mkstemp(output_filename);
    if (fd == -1) {
      perror("mkstemp");
      exit(EXIT_FAILURE);
    }

    close(fd);
  }
  options[num_options-2] = output_filename;
  int pipedescriptor[2];
  if (pipe(pipedescriptor) != 0) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  pid_t pid = fork();
  if (pid == 0) { // child
    close(pipedescriptor[1]);
    if(dup2(pipedescriptor[0], STDIN_FILENO) == -1) {
      perror("dup2");
      exit(EXIT_FAILURE);
    }
    if(execv(acr_system_compiler_path, options) == -1) {
      perror("execv");
      exit(EXIT_FAILURE);
    }
  }
  close(pipedescriptor[0]);
  FILE *pipe_to_child_stdin = fdopen(pipedescriptor[1], "w");
  if (!pipe_to_child_stdin) {
    perror("fdopen");
    exit(EXIT_FAILURE);
  }
  fprintf(pipe_to_child_stdin, "%s", string_to_compile);
  fclose(pipe_to_child_stdin);
  int exit_status;
  pid_t waited = waitpid(pid, &exit_status, 0);
  if (waited != pid || !WIFEXITED(exit_status) ||
      (WIFEXITED(exit_status) && (WEXITSTATUS(exit_status) != 0))) {
    if(unlink(output_filename) != 0) {
      perror("unlink");
      exit(EXIT_FAILURE);
    }
    return NULL;
  }
  return output_filename;
}

#ifdef TCC_PRESENT

TCCState* acr_compile_with_tcc(
    const char *string_to_compile) {
  TCCState *compile_state = tcc_new();
  tcc_add_include_path(compile_state, ".");
  tcc_set_output_type(compile_state, TCC_OUTPUT_MEMORY);
  if(tcc_compile_string(compile_state, string_to_compile) == -1) {
    fprintf(stderr, "Tcc compilation failed\n%s\n", string_to_compile);
    exit(EXIT_FAILURE);
  }
  if(tcc_relocate(compile_state, TCC_RELOCATE_AUTO) == -1) {
    fprintf(stderr, "Tcc relocation failed\n");
    exit(EXIT_FAILURE);
  }
  return compile_state;
}

#endif

void acr_code_generation_compile_and_get_functions(
    struct acr_runtime_data_static *static_data,
    const char *library_code) {
  static_data->all_functions =
    malloc(static_data->num_alternatives * static_data->num_fun_per_alt
                                         * sizeof(*static_data->functions));
  static_data->functions =
    malloc(static_data->num_fun_per_alt * sizeof(*static_data->functions));

  char **compile_options;
  size_t num_compile_options;
  acr_compile_flags(&compile_options, &num_compile_options);

  char *compiled_filename = acr_compile_with_system_compiler(NULL,
      library_code, num_compile_options, compile_options);

  static_data->dl_handle = dlopen(compiled_filename, RTLD_LAZY);
  if (static_data->dl_handle == NULL) {
    char *dlerror_str = dlerror();
    fprintf(stderr, "%s\n", dlerror_str);
    exit(EXIT_FAILURE);
  }
  unlink(compiled_filename);
  free(compiled_filename);

  char name_buffer[1024];
  name_buffer[0] = 'a';
  size_t index;
  for (size_t alt = 0; alt < static_data->num_alternatives; ++alt) {
    index = 1;
    int printed = snprintf(name_buffer+index, 1024-index, "%zu", alt);
    if (printed < 0 || printed >= 1024-(int)index) {
      fprintf(stderr, "Could not generate function name\n");
      exit(EXIT_FAILURE);
    }
    index += (size_t) printed;
    for(size_t tile_id = 0; tile_id < static_data->num_fun_per_alt; ++tile_id) {
      printed = snprintf(name_buffer+index, 1024-index, "_%zu", tile_id);
      if (printed < 0 || printed >= 1024-(int)index) {
        fprintf(stderr, "Could not generate function name\n");
        exit(EXIT_FAILURE);
      }
      void *function = dlsym(static_data->dl_handle, name_buffer);
      if (function == NULL) {
        char *dlerror_str = dlerror();
        fprintf(stderr, "%s\n", dlerror_str);
        exit(EXIT_FAILURE);
      }
      static_data->all_functions[alt*static_data->num_fun_per_alt + tile_id] =
        function;
    }
  }

  for(size_t tile_id = 0; tile_id < static_data->num_fun_per_alt; ++tile_id) {
    static_data->functions[tile_id] = static_data->all_functions[tile_id];
  }

  acr_free_compile_flags(compile_options);
}
