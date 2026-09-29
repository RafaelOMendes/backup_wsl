#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// Gera um arquivo de texto gigante com palavras sorteadas de um vocabulário,
// para testar as versões de contagem de ocorrências. A semente é fixa, então
// o arquivo gerado (e a contagem esperada) é sempre o mesmo.
// Uso: ./gerar_arquivo [tamanho_em_MB] [arquivo_saida]

#define TAMANHO_PADRAO_MB 2048
#define ARQUIVO_PADRAO "texto_gigante.txt"
#define PALAVRAS_POR_LINHA 12
#define TAM_BUFFER (1 << 20)         // escreve no disco em blocos de 1 MB
#define SEMENTE 42

static const char *vocabulario[] = {
    "programacao", "paralelo", "thread", "processo", "memoria", "nucleo",
    "desempenho", "tempo", "dados", "vetor", "matriz", "soma",
    "arquivo", "texto", "palavra", "contagem", "bloco", "tarefa",
    "sincronizacao", "mutex", "semaforo", "barreira", "escalonador", "cache",
    "processador", "placa", "video", "kernel", "compilador", "algoritmo",
    "sequencial", "paralelismo", "paralelogramo", "concorrencia", "speedup", "eficiencia",
    "o", "a", "de", "que", "e", "do", "da", "em", "um", "para", "com", "uma",
};

static double agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// gerador pseudoaleatório xorshift64*: bem mais rápido que rand() e
// gera a mesma sequência em qualquer máquina
static uint64_t estado = SEMENTE;
static uint64_t aleatorio(void) {
    estado ^= estado >> 12;
    estado ^= estado << 25;
    estado ^= estado >> 27;
    return estado * 2685821657736338717ULL;
}

int main(int argc, char **argv) {
    long long tamanho_mb = (argc > 1) ? atoll(argv[1]) : TAMANHO_PADRAO_MB;
    const char *saida = (argc > 2) ? argv[2] : ARQUIVO_PADRAO;
    long long alvo = tamanho_mb * 1024 * 1024;
    int n_palavras = sizeof(vocabulario) / sizeof(vocabulario[0]);

    FILE *f = fopen(saida, "wb");
    if (!f) {
        perror(saida);
        return 1;
    }

    printf("Gerando %s com %lld MB...\n", saida, tamanho_mb);
    double inicio = agora();

    // folga no fim do buffer para a última palavra que passar de TAM_BUFFER
    static char buffer[TAM_BUFFER + 64];
    long long escritos = 0;
    int na_linha = 0;

    while (escritos < alvo) {
        size_t usado = 0;
        while (usado < TAM_BUFFER) {
            const char *p = vocabulario[aleatorio() % n_palavras];
            size_t len = strlen(p);
            memcpy(buffer + usado, p, len);
            usado += len;

            na_linha++;
            if (na_linha == PALAVRAS_POR_LINHA) {
                buffer[usado++] = '\n';
                na_linha = 0;
            } else {
                buffer[usado++] = ' ';
            }
        }

        // o último bloco é cortado para o arquivo ficar com o tamanho exato
        size_t a_escrever = (alvo - escritos < (long long) usado) ? (size_t) (alvo - escritos) : usado;
        if (fwrite(buffer, 1, a_escrever, f) != a_escrever) {
            perror("fwrite");
            return 1;
        }
        escritos += a_escrever;
    }

    fclose(f);
    double fim = agora();

    printf("Arquivo gerado: %s (%lld bytes) em %.2f segundos\n", saida, escritos, fim - inicio);

    return 0;
}
