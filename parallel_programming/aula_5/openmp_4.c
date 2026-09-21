#include <stdio.h>
#include <omp.h>

#define TAMANHO 999999

int main() {
    omp_set_num_threads(5);

    int dados[TAMANHO];

    for (int i = 0; i < TAMANHO; i++)
    {
        dados[i] = 100 + i;
    }

    double cronometro_inicio = omp_get_wtime();

    //int somatorio = 0;
    unsigned long somatorio = 1;
    //#pragma omp parallel for reduction(+ : somatorio)
    int par = 0;
    #pragma omp parallel for reduction(+ : somatorio)
    for (int i = 0; i < TAMANHO; i++) {
        somatorio += dados[i];
        if (dados[i] % 2 == 0) {
            #pragma omp critical
            par++;
        }
        
        //printf("Thread_%d -> %ld\n", omp_get_thread_num(), somatorio);
    }

    double cronometro_final = omp_get_wtime();
    double tempo_processamento = cronometro_final - cronometro_inicio;

    printf("O número de pares é: %d\n", par);
    printf("O somatório total do vetor é: %ld\n", somatorio);
    printf("As threads levaram %f segundos\n", tempo_processamento);

    return 0;
}