#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int numero;
    scanf("%d", &numero);

    for (int i = numero - 1; i > 1; i--) {
        if (numero % i == 0) {
            printf("O número digitado NÃO é primo\n");
            return 0;
        }
    }
    printf("O número digitado é primo\n");
    
    return 0;
}