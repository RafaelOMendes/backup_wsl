#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void *processarThread(void *parametro)
{
    int *numero_mostrar = (int *)parametro;
    sleep(5);
    printf("A thread %lu recebeu o valor %d \n", pthread_self(), *numero_mostrar);
}

int main()
{
    pthread_t id_thread;
    pthread_t id_thread2;

    int numero = 10;
    int numero2 = 500;

    pthread_create(&id_thread, NULL, processarThread, &numero);
    pthread_create(&id_thread2, NULL, processarThread, &numero2);

    printf("Ta rodando fi \n");

    pthread_join(id_thread, NULL);
    pthread_join(id_thread2, NULL);

    printf("Todas as thread finalizaram \n");
    return 0;
}
