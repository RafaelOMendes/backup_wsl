#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Contagem de ocorrências de um caractere/palavra em um arquivo de texto gigante:
// o arquivo inteiro é carregado na memória e, para cada posição do texto,
// verifica se a palavra começa ali (um caractere é só uma palavra de tamanho 1).
// Conta também ocorrências dentro de outras palavras: procurar "paralelo"
// encontra o "paralelo" de "paralelogramo".
// Versão SEQUENCIAL (single-thread), usada como referência de tempo.
// Uso: ./contagem_sequencial [arquivo] [palavra]

#define ARQUIVO_PADRAO "texto_gigante.txt"
#define PALAVRA_PADRAO "paralelo"
#define TAM_LEITURA (64 << 20)       // lê o arquivo em pedaços de 64 MB

static double agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// carrega o arquivo inteiro para a memória e devolve o tamanho em *tamanho
static char *carregar_arquivo(const char *caminho, size_t *tamanho) {
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        perror(caminho);
        exit(1);
    }

    fseeko(f, 0, SEEK_END);
    size_t n = ftello(f);
    fseeko(f, 0, SEEK_SET);

    char *texto = malloc(n > 0 ? n : 1);
    if (!texto) {
        fprintf(stderr, "Sem memória para carregar %zu bytes\n", n);
        exit(1);
    }

    size_t lidos = 0;
    while (lidos < n) {
        size_t pedaco = (n - lidos < TAM_LEITURA) ? n - lidos : TAM_LEITURA;
        size_t r = fread(texto + lidos, 1, pedaco, f);
        if (r == 0) {
            fprintf(stderr, "Erro ao ler %s\n", caminho);
            exit(1);
        }
        lidos += r;
    }
    fclose(f);

    *tamanho = n;
    return texto;
}

// conta as ocorrências da palavra que COMEÇAM nas posições [ini, fim) do texto
static long long contar_ocorrencias(const char *texto, size_t ini, size_t fim,
                                    const char *palavra, size_t m) {
    long long total = 0;
    for (size_t i = ini; i < fim; i++) {
        size_t k = 0;
        while (k < m && texto[i + k] == palavra[k]) k++;
        if (k == m) total++;
    }
    return total;
}

int main(int argc, char **argv) {
    const char *caminho = (argc > 1) ? argv[1] : ARQUIVO_PADRAO;
    const char *palavra = (argc > 2) ? argv[2] : PALAVRA_PADRAO;
    size_t m = strlen(palavra);

    if (m == 0) {
        fprintf(stderr, "A palavra procurada não pode ser vazia\n");
        return 1;
    }

    double inicio_leitura = agora();
    size_t n;
    char *texto = carregar_arquivo(caminho, &n);
    double fim_leitura = agora();

    // posições onde uma ocorrência pode começar sem passar do fim do texto
    size_t n_posicoes = (n >= m) ? n - m + 1 : 0;

    double inicio = agora();
    long long total = contar_ocorrencias(texto, 0, n_posicoes, palavra, m);
    double fim = agora();

    printf("=== Contagem de Ocorrências (SEQUENCIAL) ===\n");
    printf("Arquivo: %s (%.2f MB) | Palavra: \"%s\"\n", caminho, n / (1024.0 * 1024.0), palavra);
    printf("Número de threads utilizadas: 1 (execução sequencial)\n");
    printf("Ocorrências encontradas: %lld\n", total);
    printf("Tempo de leitura do arquivo: %.4f segundos (não entra na contagem)\n", fim_leitura - inicio_leitura);
    printf("Tempo de execução: %.4f segundos (%.2f GB/s)\n", fim - inicio, n / (fim - inicio) / 1e9);

    free(texto);
    return 0;
}
