#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// Contagem de ocorrências de um caractere/palavra em um arquivo de texto gigante:
// o arquivo inteiro é carregado na memória e, para cada posição do texto,
// verifica se a palavra começa ali (um caractere é só uma palavra de tamanho 1).
// Conta também ocorrências dentro de outras palavras: procurar "paralelo"
// encontra o "paralelo" de "paralelogramo".
// Versão PARALELA com OpenMP: o texto é dividido em blocos contíguos, cada
// thread conta as ocorrências no seu bloco e as contagens parciais são
// somadas no final.
// Uso: ./contagem_paralelo [arquivo] [palavra] [threads]

#define ARQUIVO_PADRAO "texto_gigante.txt"
#define PALAVRA_PADRAO "paralelo"
#define TAM_LEITURA (64 << 20)       // lê o arquivo em pedaços de 64 MB

// carrega o arquivo inteiro para a memória e devolve o tamanho em *tamanho.
// A leitura do disco não é paralelizada: ela é limitada pelo disco, não pela CPU.
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
    int num_threads = (argc > 3) ? atoi(argv[3]) : omp_get_max_threads();
    size_t m = strlen(palavra);

    if (m == 0) {
        fprintf(stderr, "A palavra procurada não pode ser vazia\n");
        return 1;
    }
    if (num_threads < 1) num_threads = 1;

    omp_set_num_threads(num_threads);

    double inicio_leitura = omp_get_wtime();
    size_t n;
    char *texto = carregar_arquivo(caminho, &n);
    double fim_leitura = omp_get_wtime();

    // posições onde uma ocorrência pode começar sem passar do fim do texto
    size_t n_posicoes = (n >= m) ? n - m + 1 : 0;

    // resultado de cada thread: a thread t escreve só em parciais[t]
    long long *parciais = calloc(num_threads, sizeof(long long));
    size_t *bloco_ini = calloc(num_threads, sizeof(size_t));
    size_t *bloco_fim = calloc(num_threads, sizeof(size_t));
    int threads_usadas = 0;

    double inicio = omp_get_wtime();

    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        int nt = omp_get_num_threads();

        // divide as posições em nt blocos de tamanho (quase) igual
        size_t tam_bloco = (n_posicoes + nt - 1) / nt;
        size_t ini = (size_t) id * tam_bloco;
        size_t fim = ini + tam_bloco;
        if (ini > n_posicoes) ini = n_posicoes;
        if (fim > n_posicoes) fim = n_posicoes;

        // Fronteira entre blocos: a thread conta as ocorrências que COMEÇAM no
        // seu bloco, e para isso pode ler até m-1 bytes do bloco da vizinha.
        // Como é só leitura, não há condição de corrida, e uma palavra que
        // cruza a fronteira é contada exatamente uma vez (pela thread da esquerda).
        parciais[id] = contar_ocorrencias(texto, ini, fim, palavra, m);
        bloco_ini[id] = ini;
        bloco_fim[id] = fim;

        if (id == 0) threads_usadas = nt;
    }

    // soma no final: junta as contagens parciais de cada thread
    long long total = 0;
    for (int t = 0; t < threads_usadas; t++) total += parciais[t];

    double fim = omp_get_wtime();

    printf("=== Contagem de Ocorrências (PARALELA - OpenMP) ===\n");
    printf("Arquivo: %s (%.2f MB) | Palavra: \"%s\"\n", caminho, n / (1024.0 * 1024.0), palavra);
    printf("Número de threads utilizadas: %d\n", threads_usadas);
    for (int t = 0; t < threads_usadas; t++) {
        printf("  Thread %2d: posições [%zu, %zu) -> %lld ocorrências\n",
               t, bloco_ini[t], bloco_fim[t], parciais[t]);
    }
    printf("Ocorrências encontradas: %lld\n", total);
    printf("Tempo de leitura do arquivo: %.4f segundos (não entra na contagem)\n", fim_leitura - inicio_leitura);
    printf("Tempo de execução: %.4f segundos (%.2f GB/s)\n", fim - inicio, n / (fim - inicio) / 1e9);

    free(parciais);
    free(bloco_ini);
    free(bloco_fim);
    free(texto);
    return 0;
}
