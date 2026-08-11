#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Digite um número: ");
    int num1;
    scanf("%d", &num1);
    printf("Digite outro número: ");
    int num2;
    scanf("%d", &num2);
    int soma = num1 + num2;
    int sub = num1 - num2;
    int mul = num1 * num2;
    int div = num1 / num2;

    printf("A soma do número 1 pelo número 2 é: %d\n", soma);
    printf("A subtração do número 1 pelo número 2 é: %d\n", sub);
    printf("A multiplicação do número 1 pelo número 2 é: %d\n", mul);
    printf("A divisão do número 1 pelo número 2 é: %d\n", div);
    
    return 0;
}