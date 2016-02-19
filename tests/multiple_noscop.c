#define N 15
#define MAX 150
#include "acr/acr_openscop.h"

int temporary_array[MAX][MAX];

void lin_solve_computation(int k, int i, int j);
void lin_solve_computation2(float alpha, int i, int j);

#pragma acr init(void a(void))

int kernel1() {

  while (1) {
#pragma acr grid(5)
#pragma acr monitor(temporary_array[l][m], max)
#pragma acr alternative \
    low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr \
    strategy direct(1, low)
#pragma acr strategy direct(2, medium)
#pragma acr strategy direct(3, high)
    for (int k=0; k <= N; ++k)
      for (int i=0; i <= MAX; ++i)
        for (int j=0; j <= MAX; ++j)
          lin_solve_computation(k,i,j);
  }
#pragma acr destroy
}

#pragma acr init(void b(float pedro, float* pedro_addr))

int kernel2() {

  float pedro = 3.14f;
  float* pedro_addr = &pedro;

  while (1) {
#pragma acr monitor(temporary_array[l][m], max)
#pragma acr grid(5)
#pragma acr alternative low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr strategy direct(1, low)
#pragma acr strategy direct(2, medium)
#pragma acr strategy range(-5, 5, medium)
#pragma acr strategy direct(3, high)
#pragma acr grid (112)
    for (int n=0; n <= N; ++n)
      for (int l=0; l <= MAX; ++l)
        for (int m=0; m <= l; ++m) {
          lin_solve_computation2(pedro,l,m);
          *pedro_addr += 3;
        }
  }
#pragma acr destroy
}

void lin_solve_computation(int k, int i, int j) {
  temporary_array[i][j] += k;
}

void lin_solve_computation2(float alpha, int i, int j) {
  temporary_array[i][j] += alpha;
}

int main() {
  osl_scop_p scop = acr_read_scop_from_buffer(a_acr_scop, a_acr_scop_size);
  osl_scop_print(stderr, scop);
  osl_scop_free(scop);
  return 0;
}
