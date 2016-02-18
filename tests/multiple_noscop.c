#define N 15
#define MAX 150

int temporary_array[MAX][MAX];

void lin_solve_computation(int k, int i, int j);

#pragma acr init(void a(int i, int j, int k))

int kernel1() {
  int i, j, k;

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
    for (k=0; k <= N; ++k)
      for (i=0; i <= MAX; ++i)
        for (j=0; j <= MAX; ++j)
          lin_solve_computation(k,i,j);
  }
#pragma acr destroy
}

#pragma acr init(void b(int l, int m, int n))

int kernel2() {
  int l, m, n;

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
    for (n=0; n <= N; ++n)
      for (l=0; l <= MAX; ++l)
        for (m=0; m <= l; ++m)
          lin_solve_computation(n,l,m);
  }
#pragma acr destroy
}

void lin_solve_computation(int k, int i, int j) {
  temporary_array[i][j] += k;
}
