#include <stdio.h>
#include <omp.h>

int main()
{
    omp_set_num_threads(4);

    int dados[10];

    const int tamanho = sizeof(dados) / sizeof(dados[0]);

    for (int i = 0; i < tamanho; i++)
    {
        dados[i] = 100 + i;
    }

    #pragma omp parallel
    {
        #pragma omp for
        // #pragma omp for schedule (static, 2)
        // #pragma omp for schedule (dynamic)
        for (int i = 0; i < 10; i++)
        {
            dados[i] += 1000;
            printf("Eu sou a thread %d e calculei o valor de %d\n", omp_get_thread_num(), dados[i]);
        }
        
    }

    for (int i = 0; i < tamanho; i++)
    {
        printf("Posicao %d, Valor %d\n", i, dados[i]);
    }

    return 0;
}