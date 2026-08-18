#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define SIZE 10

int dados[SIZE];

void motrar_vetor(int vetor[], int tamanho);

int main()
{
    for (int i = 0; i < SIZE; i++)
    {
        dados[i] = i;
    }

    int id_processo = fork();

    if (id_processo != 0)
    {
        for (int i = 0; i < SIZE / 2; i++)
        {
            dados[i] += 100;
            // sleep(3);
        }

        motrar_vetor(dados, SIZE);
    }
    else
    {
        for (int i = SIZE / 2; i < SIZE; i++)
        {
            dados[i] += 500;
            // sleep(2);
        }

        motrar_vetor(dados, SIZE);
    }

    return 0;
}

void motrar_vetor(int vetor[], int tamanho)
{
    for (int i = 0; i < tamanho; i++)
    {
        printf("%d\n", vetor[i]);
    }
}