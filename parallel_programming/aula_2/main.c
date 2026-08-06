#include <stdio.h>

int main() {
    printf("teste rafa\n");
    
    int rafaravel = 18;
    printf("O valor da minha variável é %d \n", rafaravel);
    
    int* rafateiro = &rafaravel;
    printf("O ponteiro vai para %p \n", rafateiro);
    
    return 0;
}