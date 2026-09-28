#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// Simulação de fluidos 2D (método "Stable Fluids" de Jos Stam):
// resolve difusão + advecção de velocidade e densidade numa grade,
// com projeção de pressão para manter o fluido incompressível.
// Versão PARALELA com OpenMP: os loops sobre o grid são paralelizados.
// Como cada célula é calculada só a partir do buffer da iteração
// anterior (Jacobi), não há condição de corrida entre as threads.

#define N 256                        // resolução do grid (células internas)
#define TAM ((N + 2) * (N + 2))      // grid com borda de 1 célula em cada lado
#define IX(i, j) ((i) + (N + 2) * (j))
#define SWAP(a, b) { float *tmp = a; a = b; b = tmp; }

#define DT 0.1f
#define DIFUSAO 0.0001f
#define VISCOSIDADE 0.0001f
#define ITERACOES_SOLVER 20
#define PASSOS_SIMULACAO 500
#define PASSOS_COM_FONTE 50          // por quantos passos injeta densidade/velocidade

// condições de contorno: espelha os valores nas bordas do grid.
// Não é paralelizado: são só O(N) células (não O(N²)), então o overhead
// de abrir uma região paralela custaria mais do que o trabalho em si.
static void set_bnd(int b, float *x) {
    for (int i = 1; i <= N; i++) {
        x[IX(0, i)]     = (b == 1) ? -x[IX(1, i)] : x[IX(1, i)];
        x[IX(N + 1, i)] = (b == 1) ? -x[IX(N, i)] : x[IX(N, i)];
        x[IX(i, 0)]     = (b == 2) ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, N + 1)] = (b == 2) ? -x[IX(i, N)] : x[IX(i, N)];
    }
    x[IX(0, 0)]         = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, N + 1)]     = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
    x[IX(N + 1, 0)]     = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
    x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
}

// soma a fonte externa (dt já aplicado) ao campo x.
// Operação muito barata (uma soma por célula): paralelizar aqui também
// custaria mais em sincronização do que economizaria em cálculo.
static void add_source(float *x, const float *s) {
    for (int k = 0; k < TAM; k++) x[k] += DT * s[k];
}

// resolve o sistema linear de difusão/pressão por iteração de Jacobi
static void lin_solve(int b, float *x, const float *x0, float a, float c) {
    static float buf_a[TAM], buf_b[TAM];
    float *velho = buf_a, *novo = buf_b;

    memcpy(velho, x, sizeof(buf_a));

    for (int k = 0; k < ITERACOES_SOLVER; k++) {
        #pragma omp parallel for collapse(2)
        for (int j = 1; j <= N; j++) {
            for (int i = 1; i <= N; i++) {
                novo[IX(i, j)] = (x0[IX(i, j)] + a * (velho[IX(i - 1, j)] + velho[IX(i + 1, j)] +
                                                       velho[IX(i, j - 1)] + velho[IX(i, j + 1)])) / c;
            }
        }
        set_bnd(b, novo);
        SWAP(velho, novo);
    }

    memcpy(x, velho, sizeof(buf_a));
}

static void difundir(int b, float *x, const float *x0, float diff) {
    float a = DT * diff * N * N;
    lin_solve(b, x, x0, a, 1 + 4 * a);
}

// transporta a grandeza (densidade ou velocidade) ao longo do campo de velocidade
static void advectar(int b, float *d, const float *d0, const float *u, const float *v) {
    float dt0 = DT * N;

    #pragma omp parallel for collapse(2)
    for (int j = 1; j <= N; j++) {
        for (int i = 1; i <= N; i++) {
            float x = i - dt0 * u[IX(i, j)];
            float y = j - dt0 * v[IX(i, j)];

            if (x < 0.5f) x = 0.5f;
            if (x > N + 0.5f) x = N + 0.5f;
            int i0 = (int) x, i1 = i0 + 1;

            if (y < 0.5f) y = 0.5f;
            if (y > N + 0.5f) y = N + 0.5f;
            int j0 = (int) y, j1 = j0 + 1;

            float s1 = x - i0, s0 = 1 - s1;
            float t1 = y - j0, t0 = 1 - t1;

            d[IX(i, j)] = s0 * (t0 * d0[IX(i0, j0)] + t1 * d0[IX(i0, j1)]) +
                          s1 * (t0 * d0[IX(i1, j0)] + t1 * d0[IX(i1, j1)]);
        }
    }
    set_bnd(b, d);
}

// projeta o campo de velocidade num campo com divergência zero (incompressibilidade)
static void projetar(float *u, float *v, float *p, float *div) {
    #pragma omp parallel for collapse(2)
    for (int j = 1; j <= N; j++) {
        for (int i = 1; i <= N; i++) {
            div[IX(i, j)] = -0.5f * (u[IX(i + 1, j)] - u[IX(i - 1, j)] +
                                      v[IX(i, j + 1)] - v[IX(i, j - 1)]) / N;
            p[IX(i, j)] = 0;
        }
    }
    set_bnd(0, div);
    set_bnd(0, p);

    lin_solve(0, p, div, 1, 4);

    #pragma omp parallel for collapse(2)
    for (int j = 1; j <= N; j++) {
        for (int i = 1; i <= N; i++) {
            u[IX(i, j)] -= 0.5f * N * (p[IX(i + 1, j)] - p[IX(i - 1, j)]);
            v[IX(i, j)] -= 0.5f * N * (p[IX(i, j + 1)] - p[IX(i, j - 1)]);
        }
    }
    set_bnd(1, u);
    set_bnd(2, v);
}

// avança o campo de velocidade um passo de tempo (fonte -> difusão -> projeção
// -> advecção -> projeção). Usa o clássico truque de troca de ponteiros: cada
// SWAP só troca as variáveis locais, mas como sempre há um número par de trocas
// por buffer, o resultado final acaba de volta no buffer original de u/v.
static void passo_velocidade(float *u, float *v, float *u0, float *v0) {
    add_source(u, u0);
    add_source(v, v0);

    SWAP(u0, u); difundir(1, u, u0, VISCOSIDADE);
    SWAP(v0, v); difundir(2, v, v0, VISCOSIDADE);
    projetar(u, v, u0, v0);

    SWAP(u0, u); SWAP(v0, v);
    advectar(1, u, u0, u0, v0);
    advectar(2, v, v0, u0, v0);
    projetar(u, v, u0, v0);
}

static void passo_densidade(float *d, float *d0, const float *u, const float *v) {
    add_source(d, d0);

    SWAP(d0, d); difundir(0, d, d0, DIFUSAO);
    SWAP(d0, d); advectar(0, d, d0, u, v);
}

int main(int argc, char **argv) {
    int passos = (argc > 1) ? atoi(argv[1]) : PASSOS_SIMULACAO;
    int num_threads = (argc > 2) ? atoi(argv[2]) : omp_get_max_threads();

    omp_set_num_threads(num_threads);

    // confirma quantas threads a equipe OpenMP realmente vai usar
    int threads_usadas = 0;
    #pragma omp parallel
    {
        #pragma omp single
        threads_usadas = omp_get_num_threads();
    }

    static float u[TAM], v[TAM], u0[TAM], v0[TAM];
    static float dens[TAM], dens0[TAM];

    memset(u, 0, sizeof(u));
    memset(v, 0, sizeof(v));
    memset(dens, 0, sizeof(dens));

    double inicio = omp_get_wtime();

    for (int passo = 0; passo < passos; passo++) {
        memset(u0, 0, sizeof(u0));
        memset(v0, 0, sizeof(v0));
        memset(dens0, 0, sizeof(dens0));

        if (passo < PASSOS_COM_FONTE) {
            int centro = IX(N / 2, N / 2);
            dens0[centro] = 200.0f;
            u0[centro] = 30.0f;
            v0[centro] = 30.0f;
        }

        passo_velocidade(u, v, u0, v0);
        passo_densidade(dens, dens0, u, v);
    }

    double fim = omp_get_wtime();

    double soma_densidade = 0.0;
    for (int k = 0; k < TAM; k++) soma_densidade += dens[k];

    printf("=== Simulação de Fluidos (PARALELA - OpenMP) ===\n");
    printf("Grid: %dx%d | Passos: %d | Iterações do solver: %d\n", N, N, passos, ITERACOES_SOLVER);
    printf("Número de threads utilizadas: %d\n", threads_usadas);
    printf("Soma total da densidade final: %.4f\n", soma_densidade);
    printf("Tempo de execução: %.4f segundos\n", fim - inicio);

    return 0;
}
