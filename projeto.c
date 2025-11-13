#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Protótipos das funções - como em prog3.pdf
void exibeAjuda();
int** leituraManual(int *altura, int *largura);
int** leituraPBM(const char *nome_arquivo, int *altura, int *largura);
int ehUniforme(int **imagem, int lin_ini, int col_ini, int lin_fim, int col_fim);
void codificaRegiao(int **imagem, int lin_ini, int col_ini, int lin_fim, int col_fim, char *saida, int *pos);
char* codificaImagem(int **imagem, int altura, int largura);
void liberaImagem(int **imagem, int altura);

int main(int argc, char *argv[]) {
    // Se nenhum argumento for passado, exibe ajuda e sai
    if (argc == 1) {
        exibeAjuda();
        return 0;
    }

    // Se for passado -? ou --help, exibe ajuda e sai
    if (argc == 2 && (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "--help") == 0)) {
        exibeAjuda();
        return 0;
    }

    // Aloca variáveis para armazenar altura e largura da imagem
    // Usamos ponteiros para que as funções de leitura possam alterar esses valores
    // Isso é o mesmo conceito de passagem por referência do PDF de ponteiros
    int *altura = malloc(sizeof(int));
    int *largura = malloc(sizeof(int));
    int **imagem = NULL;

    // Verifica qual modo de entrada o usuário escolheu
    if (argc == 2 && (strcmp(argv[1], "-m") == 0 || strcmp(argv[1], "--manual") == 0)) {
        // Modo manual: leitura via teclado
        imagem = leituraManual(altura, largura);
    } else if (argc == 3 && (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "--file") == 0)) {
        // Modo arquivo: leitura de um arquivo PBM
        imagem = leituraPBM(argv[2], altura, largura);
        // Verifica se o arquivo foi lido corretamente
        if (imagem == NULL) {
            fprintf(stderr, "Erro: não foi possível ler o arquivo '%s'.\n", argv[2]);
            return 1;
        }
    } else {
        // Se não for -m nem -f com nome do arquivo, argumentos inválidos
        fprintf(stderr, "Argumentos inválidos. Use -? para ajuda.\n");
        return 1;
    }

    // Se a leitura da imagem falhou, encerra o programa
    if (imagem == NULL) {
        fprintf(stderr, "Erro ao carregar a imagem.\n");
        return 1;
    }

    // Codifica a imagem e imprime o código resultante
    char *codigo = codificaImagem(imagem, *altura, *largura);
    printf("%s\n", codigo);

    // Libera a memória alocada para o código e a imagem
    free(codigo);
    liberaImagem(imagem, *altura);
    free(altura);
    free(largura);

    return 0;
}

// Função que exibe a mensagem de ajuda do programa
void exibeAjuda() {
    printf("Uso: ImageEncoder [-? | -m | -f ARQ]\n");
    printf("Codifica imagens binárias dadas em arquivos PBM ou por dados informados manualmente.\n");
    printf("\n");
    printf("Argumentos:\n");
    printf(" -?, --help : apresenta essa orientação na tela.\n");
    printf(" -m, --manual: ativa o modo de entrada manual, em que o usuário fornece todos os dados da imagem informando-os através do teclado.\n");
    printf(" -f, --file: considera a imagem representada no arquivo PBM (Portable bitmap).\n");
}

// Função que lê os dados da imagem manualmente via stdin
int** leituraManual(int *altura, int *largura) {
    printf("Digite a altura e largura da imagem (max 1024x768): ");
    scanf("%d %d", altura, largura);

    // Verifica se as dimensões são válidas
    if (*altura <= 0 || *largura <= 0 || *altura > 768 || *largura > 1024) {
        fprintf(stderr, "Dimensões inválidas.\n");
        return NULL;
    }

    // Aloca a matriz dinamicamente - como em prog5.pdf
    int **imagem = malloc(*altura * sizeof(int*));
    for (int i = 0; i < *altura; i++) {
        imagem[i] = malloc(*largura * sizeof(int));
    }

    printf("Digite os pixels (0=branco, 1=preto), linha por linha:\n");
    for (int i = 0; i < *altura; i++) {
        for (int j = 0; j < *largura; j++) {
            scanf("%d", &imagem[i][j]);
            // Verifica se o pixel é 0 ou 1
            if (imagem[i][j] != 0 && imagem[i][j] != 1) {
                fprintf(stderr, "Pixel inválido: %d. Use 0 ou 1.\n", imagem[i][j]);
                liberaImagem(imagem, *altura);
                return NULL;
            }
        }
    }

    return imagem;
}

// Função que lê os dados da imagem a partir de um arquivo PBM (P1 ASCII)
int** leituraPBM(const char *nome_arquivo, int *altura, int *largura) {
    FILE *arq = fopen(nome_arquivo, "r");
    if (!arq) return NULL;

    char linha[256];
    int p1_ok = 0;
    int w = 0, h = 0;

    // Lê o cabeçalho do arquivo PBM
    while (fgets(linha, sizeof(linha), arq)) {
        // Remove espaços em branco e quebras de linha do final da linha
        int len = strlen(linha);
        while (len > 0 && (linha[len - 1] == ' ' || linha[len - 1] == '\t' || linha[len - 1] == '\n' || linha[len - 1] == '\r')) {
            linha[--len] = '\0';
        }

        if (linha[0] == '#') continue; // Ignora comentários

        if (strncmp(linha, "P1", 2) == 0) {
            p1_ok = 1; // Confirma formato P1
            continue;
        }

        if (p1_ok && sscanf(linha, "%d %d", &w, &h) == 2) {
            *largura = w;
            *altura = h;
            break; // Sai do loop após ler as dimensões
        }
    }

    // Verifica se o formato e as dimensões são válidas
    if (!p1_ok || *altura <= 0 || *largura <= 0 || *altura > 768 || *largura > 1024) {
        fclose(arq);
        return NULL;
    }

    // Aloca a matriz dinamicamente
    int **imagem = malloc(*altura * sizeof(int*));
    for (int i = 0; i < *altura; i++) {
        imagem[i] = malloc(*largura * sizeof(int));
    }

    int px, count = 0;
    // Lê os pixels (0 ou 1) do arquivo
    while (fscanf(arq, "%d", &px) == 1) {
        if (px != 0 && px != 1) {
            liberaImagem(imagem, *altura);
            fclose(arq);
            return NULL;
        }
        int i = count / *largura;
        int j = count % *largura;
        if (i >= *altura) break; // Evita estouro de memória
        imagem[i][j] = px;
        count++;
    }

    fclose(arq);
    return imagem;
}
// Função que verifica se uma região da imagem é uniforme (todos os pixels são 0 ou 1)
// Isso é o "caso base" da recursão - se a região é uniforme, retorna o valor
int ehUniforme(int **imagem, int lin_ini, int col_ini, int lin_fim, int col_fim) {
    int primeiro = imagem[lin_ini][col_ini]; // Pega o valor do primeiro pixel
    // Percorre a região (busca linear) para verificar se todos são iguais
    for (int i = lin_ini; i <= lin_fim; i++) {
        for (int j = col_ini; j <= col_fim; j++) {
            if (imagem[i][j] != primeiro) {
                return -1; // Misturado
            }
        }
    }
    return primeiro; // 0 ou 1
}

// Função recursiva que implementa o algoritmo de codificação quadtree
// Usa o conceito de recursão do segundo bimestre
void codificaRegiao(int **imagem, int lin_ini, int col_ini, int lin_fim, int col_fim, char *saida, int *pos) {
    int status = ehUniforme(imagem, lin_ini, col_ini, lin_fim, col_fim);

    if (status == 0) {
        saida[(*pos)++] = 'B'; // Região toda branca
    } else if (status == 1) {
        saida[(*pos)++] = 'P'; // Região toda preta
    } else { // status == -1, região mista
        saida[(*pos)++] = 'X'; // Divide e codifica os 4 quadrantes

        // Calcula os pontos de divisão, respeitando divisão ímpar (mais à esquerda/cima)
        // Isso está de acordo com o enunciado do projeto2.pdf
        int mid_h = lin_ini + (lin_fim - lin_ini + 1) / 2;
        int mid_w = col_ini + (col_fim - col_ini + 1) / 2;

        // 1º quadrante (superior esquerdo)
        if (lin_ini <= mid_h - 1 && col_ini <= mid_w - 1)
            codificaRegiao(imagem, lin_ini, col_ini, mid_h - 1, mid_w - 1, saida, pos);
        // 2º quadrante (superior direito)
        if (lin_ini <= mid_h - 1 && mid_w <= col_fim)
            codificaRegiao(imagem, lin_ini, mid_w, mid_h - 1, col_fim, saida, pos);
        // 3º quadrante (inferior esquerdo)
        if (mid_h <= lin_fim && col_ini <= mid_w - 1)
            codificaRegiao(imagem, mid_h, col_ini, lin_fim, mid_w - 1, saida, pos);
        // 4º quadrante (inferior direito)
        if (mid_h <= lin_fim && mid_w <= col_fim)
            codificaRegiao(imagem, mid_h, mid_w, lin_fim, col_fim, saida, pos);
    }
}

// Função principal de codificação. Chama a função recursiva para a imagem inteira.
char* codificaImagem(int **imagem, int altura, int largura) {
    // Aloca uma string grande o suficiente para a pior hipótese
    int tamanho_max = altura * largura * 4; // estimativa segura
    char *saida = malloc(tamanho_max * sizeof(char));
    int pos = 0;

    // Chama a função recursiva para a imagem inteira
    codificaRegiao(imagem, 0, 0, altura - 1, largura - 1, saida, &pos);
    saida[pos] = '\0'; // Finaliza a string

    return saida;
}

// Função que libera a memória alocada dinamicamente para a imagem
// Isso evita vazamento de memória
void liberaImagem(int **imagem, int altura) {
    if (imagem) {
        for (int i = 0; i < altura; i++) {
            free(imagem[i]); // Libera cada linha
        }
        free(imagem); // Libera o vetor de ponteiros
    }
}