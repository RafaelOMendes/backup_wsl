#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define TAMANHO 10000

int soma = 0;
pthread_mutex_t semaforo;

void * processar_thread(void * parametro) {
    int *id_thread = (int *) parametro;

    pthread_mutex_lock(&semaforo);
    soma++;
    pthread_mutex_unlock(&semaforo);

    printf("THREAD_%d => Somou 1 na variável %d \n", *id_thread, soma);
}

int main() {
    pthread_mutex_init(&semaforo, NULL);

    pthread_t vetor_threads[TAMANHO];
    for (int i = 0; i < TAMANHO; i++)
    {
        pthread_create(&vetor_threads[i], NULL, processar_thread, &i);
    }

    printf("O código main está executando\n");

    for (int i = 0; i < TAMANHO; i++)
    {
        pthread_join(vetor_threads[i], NULL);
    }

    printf("Todas as threads finalizaram!\n");

    printf("O valor da soma do vetor é: %d \n", soma);

    return 0;
}