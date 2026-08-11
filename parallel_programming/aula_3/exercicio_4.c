#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int numero;
    scanf("%d", &numero);
    
    printf("O número digitado %d é %s\n", numero, numero % 2 == 0 ? "par": "impar");
    
    return 0;
}