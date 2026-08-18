#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int numero;
    int soma = 0;
    scanf("%d", &numero);

    int digitos[10];
    int i = 0;

    while (numero > 0) {
        digitos[i] = numero % 10;
        numero = numero / 10;
        i++;
    }

    for (int j = i - 1; j >= 0; j--) {
        soma += digitos[j];
    }

    printf("A soma dos números digitados é: %d\n", soma);
    
    return 0;
}