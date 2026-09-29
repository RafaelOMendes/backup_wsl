#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <CL/cl_ext.h>

// Contagem de ocorrências de um caractere/palavra em um arquivo de texto gigante:
// o arquivo inteiro é carregado na memória e, para cada posição do texto,
// verifica se a palavra começa ali (um caractere é só uma palavra de tamanho 1).
// Conta também ocorrências dentro de outras palavras: procurar "paralelo"
// encontra o "paralelo" de "paralelogramo".
// Versão GPU com OpenCL: o texto é copiado para a memória da placa de vídeo e
// centenas de milhares de itens de trabalho (work-items) testam posições ao
// mesmo tempo. Cada grupo de itens soma suas contagens e devolve um parcial;
// a CPU soma os parciais no final.
// Roda no Windows (usa o OpenCL do driver AMD), compilado a partir do WSL
// com mingw-w64 -- veja o README.md.
// Uso: ./contagem_gpu.exe [arquivo] [palavra]

#define ARQUIVO_PADRAO "texto_gigante.txt"
#define PALAVRA_PADRAO "paralelo"
#define TAM_LEITURA (64 << 20)                   // lê o arquivo em pedaços de 64 MB
#define TAM_PEDACO_GPU ((size_t) 1 << 30)       // envia o texto para a GPU em pedaços de 1 GB
#define ITENS_POR_GRUPO 256
#define NUM_GRUPOS 2048

#ifdef _WIN32
#include <windows.h>
#define fseeko _fseeki64
#define ftello _ftelli64

static double agora(void) {
    static LARGE_INTEGER freq;
    LARGE_INTEGER t;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double) t.QuadPart / freq.QuadPart;
}
#else
#include <time.h>

static double agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

#define VERIFICA(erro, onde)                                              \
    if ((erro) != CL_SUCCESS) {                                           \
        fprintf(stderr, "Erro OpenCL %d em %s\n", (int) (erro), (onde)); \
        exit(1);                                                          \
    }

// O código do kernel é passado ao OpenCL como string e compilado na hora
// para a GPU. A macro transforma o código abaixo em string, então dá para
// escrevê-lo (e comentá-lo) como C normal.
#define CODIGO_OPENCL(...) #__VA_ARGS__

static const char *codigo_kernel = CODIGO_OPENCL(
    // Cada item de trabalho testa várias posições, pulando de "total de itens"
    // em "total de itens": no mesmo instante, itens vizinhos leem bytes vizinhos
    // da memória, que é o padrão de acesso mais rápido na GPU.
    __kernel void contar(__global const uchar *texto, ulong n_posicoes,
                         __constant uchar *palavra, uint m,
                         __global uint *parciais, __local uint *soma_grupo)
    {
        uint conta = 0;
        for (ulong i = get_global_id(0); i < n_posicoes; i += get_global_size(0)) {
            uint k = 0;
            while (k < m && texto[i + k] == palavra[k]) k++;
            if (k == m) conta++;
        }

        // soma as contagens dos itens do grupo na memória local, em árvore:
        // 256 -> 128 -> 64 -> ... -> 1, com uma barreira a cada nível
        uint lid = get_local_id(0);
        soma_grupo[lid] = conta;
        barrier(CLK_LOCAL_MEM_FENCE);
        for (uint s = get_local_size(0) / 2; s > 0; s /= 2) {
            if (lid < s) soma_grupo[lid] += soma_grupo[lid + s];
            barrier(CLK_LOCAL_MEM_FENCE);
        }

        if (lid == 0) parciais[get_group_id(0)] = soma_grupo[0];
    }
);

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
        fprintf(stderr, "Sem memória para carregar %llu bytes\n", (unsigned long long) n);
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

// procura, em todas as plataformas OpenCL, a GPU com mais unidades de computação
// (o PC também tem a GPU integrada do processador, que é bem mais fraca)
static cl_device_id escolher_gpu(void) {
    cl_platform_id plataformas[8];
    cl_uint n_plataformas = 0;
    VERIFICA(clGetPlatformIDs(8, plataformas, &n_plataformas), "clGetPlatformIDs");

    cl_device_id melhor = NULL;
    cl_uint melhor_cus = 0;
    for (cl_uint p = 0; p < n_plataformas; p++) {
        cl_device_id dispositivos[8];
        cl_uint n_dispositivos = 0;
        if (clGetDeviceIDs(plataformas[p], CL_DEVICE_TYPE_GPU, 8, dispositivos, &n_dispositivos) != CL_SUCCESS)
            continue;

        for (cl_uint d = 0; d < n_dispositivos; d++) {
            cl_uint cus = 0;
            clGetDeviceInfo(dispositivos[d], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cus), &cus, NULL);
            if (cus > melhor_cus) {
                melhor_cus = cus;
                melhor = dispositivos[d];
            }
        }
    }

    if (!melhor) {
        fprintf(stderr, "Nenhuma GPU com suporte a OpenCL encontrada\n");
        exit(1);
    }
    return melhor;
}

int main(int argc, char **argv) {
    const char *caminho = (argc > 1) ? argv[1] : ARQUIVO_PADRAO;
    const char *palavra = (argc > 2) ? argv[2] : PALAVRA_PADRAO;
    size_t m = strlen(palavra);
    cl_int erro;

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

    // ---- preparação do OpenCL: escolhe a GPU e compila o kernel ----
    double inicio_init = agora();

    cl_device_id gpu = escolher_gpu();
    char nome_gpu[256] = "";
#ifdef CL_DEVICE_BOARD_NAME_AMD
    // na AMD, CL_DEVICE_NAME devolve o codinome do chip (ex.: gfx1200)
    clGetDeviceInfo(gpu, CL_DEVICE_BOARD_NAME_AMD, sizeof(nome_gpu), nome_gpu, NULL);
#endif
    if (nome_gpu[0] == '\0') clGetDeviceInfo(gpu, CL_DEVICE_NAME, sizeof(nome_gpu), nome_gpu, NULL);
    cl_uint unidades = 0;
    clGetDeviceInfo(gpu, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(unidades), &unidades, NULL);

    cl_context contexto = clCreateContext(NULL, 1, &gpu, NULL, NULL, &erro);
    VERIFICA(erro, "clCreateContext");
    cl_command_queue fila = clCreateCommandQueue(contexto, gpu, 0, &erro);
    VERIFICA(erro, "clCreateCommandQueue");

    cl_program programa = clCreateProgramWithSource(contexto, 1, &codigo_kernel, NULL, &erro);
    VERIFICA(erro, "clCreateProgramWithSource");
    if (clBuildProgram(programa, 1, &gpu, NULL, NULL, NULL) != CL_SUCCESS) {
        char log[16384];
        clGetProgramBuildInfo(programa, gpu, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "Erro ao compilar o kernel:\n%s\n", log);
        return 1;
    }
    cl_kernel kernel = clCreateKernel(programa, "contar", &erro);
    VERIFICA(erro, "clCreateKernel");

    // o texto vai para a GPU em pedaços de TAM_PEDACO_GPU posições; o buffer
    // tem m-1 bytes a mais para a palavra que começa no fim do pedaço caber inteira
    size_t pos_por_pedaco = (n_posicoes < TAM_PEDACO_GPU) ? n_posicoes : TAM_PEDACO_GPU;
    size_t tam_buffer = pos_por_pedaco + m - 1;
    if (tam_buffer == 0) tam_buffer = 1;     // arquivo vazio: OpenCL não aceita buffer de 0 bytes

    cl_mem d_texto = clCreateBuffer(contexto, CL_MEM_READ_ONLY, tam_buffer, NULL, &erro);
    VERIFICA(erro, "clCreateBuffer (texto)");
    cl_mem d_palavra = clCreateBuffer(contexto, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m, (void *) palavra, &erro);
    VERIFICA(erro, "clCreateBuffer (palavra)");
    cl_mem d_parciais = clCreateBuffer(contexto, CL_MEM_WRITE_ONLY, NUM_GRUPOS * sizeof(cl_uint), NULL, &erro);
    VERIFICA(erro, "clCreateBuffer (parciais)");

    cl_uint m_cl = (cl_uint) m;
    VERIFICA(clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_texto), "clSetKernelArg 0");
    VERIFICA(clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_palavra), "clSetKernelArg 2");
    VERIFICA(clSetKernelArg(kernel, 3, sizeof(cl_uint), &m_cl), "clSetKernelArg 3");
    VERIFICA(clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_parciais), "clSetKernelArg 4");
    VERIFICA(clSetKernelArg(kernel, 5, ITENS_POR_GRUPO * sizeof(cl_uint), NULL), "clSetKernelArg 5");

    double fim_init = agora();

    // ---- contagem: copia cada pedaço para a GPU e roda o kernel sobre ele ----
    size_t itens_globais = (size_t) NUM_GRUPOS * ITENS_POR_GRUPO;
    size_t itens_por_grupo = ITENS_POR_GRUPO;
    static cl_uint parciais[NUM_GRUPOS];
    long long total = 0;
    double tempo_copia = 0.0, tempo_kernel = 0.0;

    double inicio = agora();

    for (size_t pos = 0; pos < n_posicoes; pos += pos_por_pedaco) {
        cl_ulong pos_pedaco = (n_posicoes - pos < pos_por_pedaco) ? n_posicoes - pos : pos_por_pedaco;
        size_t bytes_pedaco = pos_pedaco + m - 1;

        double t0 = agora();
        VERIFICA(clEnqueueWriteBuffer(fila, d_texto, CL_TRUE, 0, bytes_pedaco, texto + pos, 0, NULL, NULL),
                 "clEnqueueWriteBuffer");
        double t1 = agora();

        VERIFICA(clSetKernelArg(kernel, 1, sizeof(cl_ulong), &pos_pedaco), "clSetKernelArg 1");
        VERIFICA(clEnqueueNDRangeKernel(fila, kernel, 1, NULL, &itens_globais, &itens_por_grupo, 0, NULL, NULL),
                 "clEnqueueNDRangeKernel");
        VERIFICA(clEnqueueReadBuffer(fila, d_parciais, CL_TRUE, 0, sizeof(parciais), parciais, 0, NULL, NULL),
                 "clEnqueueReadBuffer");

        // soma no final: junta os parciais de cada grupo
        for (int g = 0; g < NUM_GRUPOS; g++) total += parciais[g];
        double t2 = agora();

        tempo_copia += t1 - t0;
        tempo_kernel += t2 - t1;
    }

    double fim = agora();

    printf("=== Contagem de Ocorrências (GPU - OpenCL) ===\n");
    printf("Arquivo: %s (%.2f MB) | Palavra: \"%s\"\n", caminho, n / (1024.0 * 1024.0), palavra);
    printf("GPU utilizada: %s (%u unidades de computação)\n", nome_gpu, unidades);
    printf("Itens de trabalho: %d grupos x %d itens = %d\n", NUM_GRUPOS, ITENS_POR_GRUPO, NUM_GRUPOS * ITENS_POR_GRUPO);
    printf("Ocorrências encontradas: %lld\n", total);
    printf("Tempo de leitura do arquivo: %.4f segundos (não entra na contagem)\n", fim_leitura - inicio_leitura);
    printf("Tempo de preparação do OpenCL: %.4f segundos (compilação do kernel, não entra na contagem)\n",
           fim_init - inicio_init);
    printf("  Cópia CPU -> GPU: %.4f segundos (%.2f GB/s)\n", tempo_copia, n / tempo_copia / 1e9);
    printf("  Kernel na GPU:    %.4f segundos (%.2f GB/s)\n", tempo_kernel, n / tempo_kernel / 1e9);
    printf("Tempo de execução: %.4f segundos (%.2f GB/s)\n", fim - inicio, n / (fim - inicio) / 1e9);

    clReleaseMemObject(d_texto);
    clReleaseMemObject(d_palavra);
    clReleaseMemObject(d_parciais);
    clReleaseKernel(kernel);
    clReleaseProgram(programa);
    clReleaseCommandQueue(fila);
    clReleaseContext(contexto);
    free(texto);
    return 0;
}
