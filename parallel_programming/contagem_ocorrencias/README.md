# Contagem de ocorrências de uma palavra em um arquivo gigante

Conta quantas vezes um caractere ou uma palavra aparece em um arquivo de texto
enorme. O texto é dividido em blocos, cada thread conta no seu pedaço e os
resultados são somados no final.

| Arquivo | O que é |
|---|---|
| `gerar_arquivo.c` | gera o arquivo de teste (palavras aleatórias, semente fixa) |
| `contagem_sequencial.c` | versão sequencial, usada como referência |
| `contagem_paralelo.c` | versão paralela com OpenMP (um bloco por thread) |
| `contagem_gpu.c` | versão na placa de vídeo com OpenCL (roda no Windows) |

## Como rodar

```bash
gcc -O2 -Wall -o gerar_arquivo gerar_arquivo.c
gcc -O2 -Wall -o contagem_sequencial contagem_sequencial.c
gcc -O2 -Wall -fopenmp -o contagem_paralelo contagem_paralelo.c

./gerar_arquivo              # cria texto_gigante.txt com 2048 MB (./gerar_arquivo 4096 para 4 GB)
./contagem_sequencial        # [arquivo] [palavra]
./contagem_paralelo          # [arquivo] [palavra] [threads]
./contagem_paralelo texto_gigante.txt paralelo 4
./contagem_sequencial texto_gigante.txt e    # um caractere também funciona
```

Para conferir o resultado: `grep -o paralelo texto_gigante.txt | wc -l`.

## Versão GPU (OpenCL)

O WSL não tem acesso direto à GPU AMD para computação, então o programa é
compilado no WSL como um `.exe` do Windows (com mingw-w64) e usa o OpenCL que
já vem no driver da AMD. O `.exe` roda normalmente a partir do terminal do WSL.

```bash
sudo apt install gcc-mingw-w64-x86-64 opencl-headers    # só na primeira vez

x86_64-w64-mingw32-gcc -O2 -Wall -idirafter /usr/include -o contagem_gpu.exe contagem_gpu.c \
    /mnt/c/Windows/System32/OpenCL.dll -static-libgcc

./contagem_gpu.exe           # [arquivo] [palavra]
```

A leitura do arquivo é lenta (cerca de 9 s para 2 GB) porque o Windows acessa
o disco do WSL pela rede interna. Esse tempo aparece separado e não entra na
contagem.

## Como a divisão em blocos funciona

Uma ocorrência é contada pelo bloco onde ela **começa**. Perto do fim do bloco,
a thread lê até `m-1` bytes do bloco vizinho (`m` = tamanho da palavra) para
terminar a comparação. Isso é só leitura, então não há condição de corrida, e
uma palavra que cruza a fronteira entre dois blocos é contada uma única vez.

Na GPU, a mesma ideia aparece duas vezes:
- o texto é enviado em pedaços de 1 GB, e cada pedaço leva `m-1` bytes extras;
- 2048 grupos de 256 itens de trabalho testam posições ao mesmo tempo. Cada grupo
  soma suas contagens na memória local e devolve um parcial, e a CPU soma os
  2048 parciais no final.

A contagem inclui ocorrências sobrepostas (`aa` aparece 2 vezes em `aaa`) e
ocorrências dentro de outras palavras (`paralelo` dentro de `paralelogramo`).

## Resultados medidos

Arquivo de 2 GB, palavra `paralelo` (12.204.891 ocorrências). Máquina: 12 threads
e RX 9060 XT 16 GB. Os tempos não incluem a leitura do arquivo.

| Versão | Tempo | Speedup |
|---|---|---|
| Sequencial | 2,31 s | 1× |
| OpenMP, 4 threads | 0,75 s | 3,1× |
| OpenMP, 12 threads | 0,35 s | 6,6× |
| GPU (cópia + kernel) | 0,21 s | 11× |
| GPU, só o kernel | 0,033 s | 70× |

Na GPU, a maior parte do tempo é a cópia CPU → GPU pelo PCIe (0,18 s). A
contagem em si é cerca de 10× mais rápida que as 12 threads da CPU.
