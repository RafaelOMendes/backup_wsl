#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int numero;
    scanf("%d", &numero);
    int fibonacci = 0;

    int anterior = 0;
    int temp = 0;
    for (int i = 0; i < numero; i++) {
        temp = fibonacci;
        fibonacci += anterior;
        anterior = temp;
        if (fibonacci == 0 && i > 0) {
            fibonacci++;
        }
    }
    printf("O valor da posição na sequência fibonacci digitado é: %d\n", fibonacci);
    
    return 0;
}