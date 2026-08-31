#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define TAMANHO 10

int dados[TAMANHO];
int soma = 0;

void *processar_thread(void *parametro)
{
    int chunk = TAMANHO / 2;
    int *id_thread = (int *)parametro;

    int posicao_inicio = *id_thread * chunk;

    for (int i = posicao_inicio; i < (1 + *id_thread) * chunk; i++)
    {
        dados[i] += 1000;
        soma += dados[i];
        printf("Thread: %d, Posicao: %d, Valor: %d\n", *id_thread, i, dados[i]);
    }
}

int main()
{

    for (int i = 0; i < TAMANHO; i++)
    {
        dados[i] = i + 100;
        printf("Posicao: %d, Valor: %d\n", i, dados[i]);
    }

    pthread_t id_minha_thread;
    pthread_t id_segunda_thread;

    int inicio_thread_1 = 0;
    int inicio_thread_2 = 1;

    pthread_create(&id_minha_thread, NULL, processar_thread, &inicio_thread_1);
    pthread_create(&id_segunda_thread, NULL, processar_thread, &inicio_thread_2);

    printf("O código main está executando\n");

    pthread_join(id_minha_thread, NULL);
    pthread_join(id_segunda_thread, NULL);

    printf("Todas as threads finalizaram!\n");

    printf("Soma: %d \n", soma);

    return 0;
}