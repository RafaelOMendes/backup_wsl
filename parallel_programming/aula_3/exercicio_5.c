#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int numero;
    scanf("%d", &numero);
    int fatorial = 1;

    for (int i = numero; i > 0; i--) {
        fatorial *= i;
    }
    printf("O fatorial do número digitado é: %d\n", fatorial);
    
    return 0;
}