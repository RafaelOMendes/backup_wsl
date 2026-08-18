#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Digite seu palindromo (100 caracteres): ");
    char palindromo[100];
    scanf("%s", palindromo);
    
    int tamanho = (int) strlen(palindromo);

    for (int i = 0; i < tamanho / 2; i++) {
        if (palindromo[i] != palindromo[(tamanho - 1) - i]) {
            printf("Não é um palindromo\n");
            return 0;
        }
    }
    printf("É um palindromo\n");

    return 0;
}