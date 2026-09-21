#include <stdio.h>
#include <omp.h>

int main() {
    omp_set_num_threads(5);

    int dados[10];

    for (int i = 0; i < 10; i++)
    {
        dados[i] = 100 + i;
    }

    unsigned long somatorio = 1;
    #pragma omp parallel for reduction(* : somatorio)
    for (int i = 0; i < 10; i++) {
        somatorio += dados[i];
        printf("Thread_%d -> %ld\n", omp_get_thread_num(), somatorio);
    }

    printf("O somatório total do vetor é: %ld\n", somatorio);

    for (int i = 0; i < 10; i++)
    {
        printf("A posição %d tem o valor de: %d \n", i, dados[i]);
    }

    return 0;
}