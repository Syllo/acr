
#define N 15

int test() {
#pragma acr init(void a(int i, int j, int k))
  int i, j, k

  while (1) {
#pragma acr grid(5)
#pragma acr alternative low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr strategy direct(1, low)
#pragma acr strategy direct(2, medium)
#pragma acr strategy direct(3, high)
#pragma scop
    for (k=0; k <= N; ++k)
      for (i=0; i <= N; ++i)
        for (j=0; j <= N; ++j)
          lin_solve_computation(k,i,j);
#pragma endscop
  }
}
#pragma acr destroy
}
