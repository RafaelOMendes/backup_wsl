#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SIZE 10

int dados[SIZE];

void motrar_vetor(int vetor[], int tamanho);

int main()
{
    for (int i = 0; i < SIZE; i++)
    {
        dados[i] = i;
    }

    printf("--------------------------------\n");

    int shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);

    if (shm_id == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int *somatorio_compartilhado = shmat(shm_id, NULL, 0);

    if (somatorio_compartilhado == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    *somatorio_compartilhado = 0;

    int id_processo = fork();

    int soma_auxiliar = 0;
    if (id_processo != 0)
    {
        for (int i = 0; i < SIZE / 2; i++)
        {
            soma_auxiliar += dados[i];
        }
    }
    else
    {
        for (int i = SIZE / 2; i < SIZE; i++)
        {
            soma_auxiliar += dados[i];
        }
    }

    *somatorio_compartilhado += soma_auxiliar;

    printf("a posicao de memoria %p tem valor %d \n", somatorio_compartilhado, *somatorio_compartilhado);

    return 0;
}

void motrar_vetor(int vetor[], int tamanho)
{
    for (int i = 0; i < tamanho; i++)
    {
        printf("%d\n", vetor[i]);
    }
}