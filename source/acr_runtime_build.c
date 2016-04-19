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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static const size_t acr_temporary_file_length = 29;
static const char acr_temporary_file_prefix[] = "/tmp/acr-runtime-temp-XXXXXX";

static const size_t num_additional_flags = 6;
static char* acr_compiler_shared_lib_flags[] = {
  [0] = "-fPIC",
  [1] = "-shared",
  [2] = "-I.",
  [3] = "-xc",
  [4] = "-",
  [5] = "-o",
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
    const char *string_to_compile,
    size_t num_options,
    char** options) {

  char *output_filename =
    malloc(acr_temporary_file_length * sizeof(*output_filename));
  memcpy(output_filename, acr_temporary_file_prefix, acr_temporary_file_length);
  int fd = mkstemp(output_filename);
  if (fd == -1) {
    perror("mkstemp");
    exit(EXIT_FAILURE);
  }

  options[num_options-2] = output_filename;

  close(fd);
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
    free(output_filename);
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
