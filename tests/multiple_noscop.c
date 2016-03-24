#define N 15
#define MAX1 10
#define MAX2 15
#define MAX3 10

int temporary_array[MAX1][MAX2];
int temporary_array_bis[MAX1-5][MAX2+MAX1][MAX3+5-MAX1];

void lin_solve_computation(int k, int i, int j);
void lin_solve_computation2(float alpha, int i, int j, int k);
static inline unsigned char solve_to_char(int i) {
  return (unsigned char) i;
}

#pragma acr init(void a(void))

int kernel1() {

  while (1) {
#pragma acr grid(4)
#pragma acr monitor(int temporary_array[i][j], avg, solve_to_char)
#pragma acr alternative \
    low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr \
    strategy direct(1, low)
#pragma acr strategy direct(2, medium)
#pragma acr strategy direct(3, high)
#pragma acr strategy direct(55, low)
#pragma acr strategy direct(254, medium)
    for (int k=0; k < N; ++k)
        for (int i=0; i < MAX1; ++i)
            for (int j=i; j < MAX2; ++j)
              lin_solve_computation(k,i,j);
  }
#pragma acr destroy
}

#pragma acr init(void b(float pedro, float* pedro_addr))

int kernel2() {

  float pedro = 3.14f;
  float* pedro_addr = &pedro;

  while (1) {
#pragma acr monitor(int temporary_array_bis[l][m][n], min)
#pragma acr grid (6)
#pragma acr alternative low(parameter, N = 1)
#pragma acr alternative medium(parameter, N = 2)
#pragma acr alternative high(parameter, N = 3)
#pragma acr strategy direct(0, low)
#pragma acr strategy range(1, 2, medium)
#pragma acr strategy direct(3, high)
    for (int a=0; a < N; ++a)
      for (int l=0; l < MAX1-5; ++l) {
        *pedro_addr += 3;
        for (int m=0; m < MAX2+MAX1; ++m) {
          for (int n=0; n < MAX3+5-MAX1; ++n) {
            lin_solve_computation2(pedro, l, m, n);
          }
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
  for(int i = 0; i < MAX1; ++i) {
    for (int j = 0; j < MAX2/2; ++j) {
      temporary_array[i][j] = 1;
    }
    for (int j = MAX2/2; j < MAX2/2+MAX2/4; ++j) {
      temporary_array[i][j] = 2;
    }
    for (int j = MAX2/2+MAX2/4; j < MAX2; ++j) {
      temporary_array[i][j] = 3;
    }
  }
  a_monitoring_function();

  float pedro = 3.14f;
  a();
  CloogNamedDomainList *domain = a_runtime_data.cloog_input->ud->domain;
  while(domain) {
    cloog_domain_print_structure(stderr, domain->domain, 0, "A");
    domain = domain->next;
  }
  free_acr_runtime_data(&a_runtime_data);
  b(pedro, &pedro);
  domain = b_runtime_data.cloog_input->ud->domain;
  while(domain) {
    cloog_domain_print_structure(stderr, domain->domain, 0, "B");
    domain = domain->next;
  }
  free_acr_runtime_data(&b_runtime_data);
  return 0;
}
