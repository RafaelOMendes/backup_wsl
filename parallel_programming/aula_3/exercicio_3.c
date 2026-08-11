#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int num1;
    scanf("%d", &num1);
    printf("Digite outro número: ");
    int num2;
    scanf("%d", &num2);
    printf("Digite outro número novamente: ");
    int num3;
    scanf("%d", &num3);
    
    if (num1 > num2 && num1 > num3) {
        printf("O maior número é o primeiro digitado: %d\n", num1);
        return 0;
    }
    
    if (num2 > num1 && num2 > num3) {
        printf("O maior número é o segundo digitado: %d\n", num2);
        return 0;
    }
    
    if (num3 > num1 && num3 > num2) {
        printf("O maior número é o terceiro digitado: %d\n", num3);
        return 0;
    }
    printf("O maior número digitado foi repetido\n");
    
    return 0;
}