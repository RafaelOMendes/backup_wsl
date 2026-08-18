#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    printf("Olá mundo \n");
    int id_processo = fork();

    printf("Id processo %d\n", getpid());

    int ano;
    scanf("%d", &ano);

    printf("Eu sou o processo %d, e o ano eh %d \n", getpid(), ano);

    return 0;
}