#include <stdio.h>
#include <stdlib.h>

void somarCem(int* num) {
    *num += 100;
}

int main() {
    
    int rafaravel = 21;
    printf("O valor da minha variável é %d \n", rafaravel);
    printf("------------------------------------\n");
    
    int* rafateiro = &rafaravel;
    printf("O ponteiro vai para %p \n", rafateiro);
    printf("O que tem no ponteiro %p? O valor: %d\n", rafateiro, rafaravel);
    printf("------------------------------------\n");
    
    somarCem(&rafaravel);
    printf("O valor da minha variável depois de somar cem é %d \n", rafaravel);
    printf("------------------------------------\n");

    int tamanho = 10;
    int *notas = malloc(tamanho * sizeof(int));

    for (int i = 0; i < tamanho; i++) {
        notas[i] = i;
    }
    
    for (int i = 0; i < tamanho; i++) {
        printf("Na posição %d que está na posição de memória %p tem armazenado %d\n", i, &notas[i], notas[i]);
    }
    printf("------------------------------------\n");

    printf("Digite seu nome: ");
    char nomeAluno[20];
    scanf("%s", nomeAluno);
    printf("O nome é: %s \n", nomeAluno);
    
    return 0;
}