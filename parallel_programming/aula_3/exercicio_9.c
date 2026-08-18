#include <stdio.h>
#include <stdlib.h>

int main() {
    int vetor[10];
    int vetor_ordenado[10];

    for(int i = 0; i < 10; i++) {
        printf("Valor para a posição %d: ", i + 1);
        scanf("%d", &vetor[i]); 
    }

    for (int i = 0; i < 10; i++) {
        int menor = vetor[0];
        int posicao_menor = 0;
        for (int index = 0; index < 10 - i; index++) {
            if (menor > vetor[index]) {
                menor = vetor[index];
                posicao_menor = index;
            }
        }
        vetor_ordenado[i] = menor;
        for (int j = posicao_menor; j < (10 - i) - 1; j++) {
            vetor[j] = vetor[j + 1];
        }
    }

    printf("\nVetor final ordenado: ");
    for(int i = 0; i < 10; i++) {
        printf("%d ", vetor_ordenado[i]);
    }
    printf("\n");
    
    return 0;
}