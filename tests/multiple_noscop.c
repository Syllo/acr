#define N 15
#define MAX1 10
#define MAX2 10
#define MAX3 10

int temporary_array[MAX1][MAX2];
int temporary_array_bis[MAX1-5][MAX2+MAX1][MAX3+5-MAX1];

void lin_solve_computation(int k, int i, int j);
void lin_solve_computation2(float alpha, int i, int j, int k);
static inline solve_to_char(int i) {
  return (char) i;
}

#pragma acr init(void a(void))

int kernel1() {

  while (1) {
#pragma acr grid(4)
#pragma acr monitor(int temporary_array[MAX1][MAX2], avg, solve_to_char)
#pragma acr alternative \
    low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr \
    strategy direct(1, low)
#pragma acr strategy direct(2, medium)
#pragma acr strategy direct(3, high)
    for (int k=0; k < N; ++k)
      for (int i=0; i < MAX1; ++i)
        for (int j=0; j < MAX2+MAX1*3; ++j)
          lin_solve_computation(k,i,j);
  }
#pragma acr destroy
}

#pragma acr init(void b(float pedro, float* pedro_addr))

int kernel2() {

  float pedro = 3.14f;
  float* pedro_addr = &pedro;

  while (1) {
#pragma acr monitor(int temporary_array_bis[MAX1-5][MAX2+MAX1][MAX3+5-MAX1], min)
#pragma acr grid (6)
#pragma acr alternative low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr strategy direct(1, low)
#pragma acr strategy direct(2, medium)
#pragma acr strategy range(-5, 5, medium)
#pragma acr strategy direct(3, high)
    for (int n=0; n < N; ++n)
      for (int l=0; l < MAX1-5; ++l)
        for (int m=0; m < MAX2+MAX1; ++m) {
          for (int n=0; n < MAX3+5-MAX1; ++n) {
            lin_solve_computation2(pedro, l, m, n);
            *pedro_addr += 3;
          }
        }
  }
#pragma acr destroy
}

void lin_solve_computation(int k, int i, int j) {
  temporary_array[i][j] += k;
}

void lin_solve_computation2(float alpha, int i, int j, int k) {
  temporary_array_bis[i][j][k] += alpha;
}

int main() {
  a_monitoring_function();
  b_monitoring_function();
  return 0;
}
