#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define MAX_NAME 256
#define MAX_LINE 1024

/* ─── Estruturas ─────────────────────────────────────────────────────────── */

typedef struct {
    int    id;
    double x;
    double y;
} Cidade;

typedef struct {
    char   name[MAX_NAME];
    int    dimension;
    Cidade *cidades;
    int    num_cidades;
} Instancia;

/* ─── Utilitário ─────────────────────────────────────────────────────────── */

static void trim(char *s) {
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    memmove(s, p, strlen(p) + 1);
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t')) s[--len] = '\0';
}

/* ─── calcular_distancia ─────────────────────────────────────────────────── */

static inline int calcular_distancia(const Cidade *c1, const Cidade *c2) {
    double dx = c1->x - c2->x;
    double dy = c1->y - c2->y;
    return (int)(0.5 + sqrt(dx*dx + dy*dy));
}

/* ─── ler_instancia ──────────────────────────────────────────────────────── */

Instancia ler_instancia(void) {
    Instancia dados;
    dados.name[0]     = '\0';
    dados.dimension   = 0;
    dados.cidades     = NULL;
    dados.num_cidades = 0;

    char linha[MAX_LINE];
    int  lendo_coords = 0;
    int  cap          = 0;

    while (fgets(linha, sizeof(linha), stdin)) {
        int len = (int)strlen(linha);
        while (len > 0 && (linha[len-1] == '\n' || linha[len-1] == '\r'))
            linha[--len] = '\0';

        if (len == 0 || strcmp(linha, "EOF") == 0) break;

        char *colon = (!lendo_coords) ? strchr(linha, ':') : NULL;

        if (colon) {
            char chave[MAX_NAME], valor[MAX_NAME];
            int  klen = (int)(colon - linha);
            strncpy(chave, linha, klen);
            chave[klen] = '\0';
            strncpy(valor, colon + 1, MAX_NAME - 1);
            valor[MAX_NAME - 1] = '\0';
            trim(chave);
            trim(valor);

            if (strcmp(chave, "NAME") == 0) {
                strncpy(dados.name, valor, MAX_NAME - 1);
            } else if (strcmp(chave, "DIMENSION") == 0) {
                dados.dimension = atoi(valor);
                cap = dados.dimension ? dados.dimension : 16;
                dados.cidades = (Cidade *)malloc(cap * sizeof(Cidade));
            }
            continue;
        }

        if (strncmp(linha, "NODE_COORD_SECTION", 18) == 0) {
            lendo_coords = 1;
            continue;
        }

        if (lendo_coords) {
            int    id;
            double x, y;
            if (sscanf(linha, "%d %lf %lf", &id, &x, &y) == 3) {
                if (dados.num_cidades >= cap) {
                    cap *= 2;
                    dados.cidades = (Cidade *)realloc(dados.cidades, cap * sizeof(Cidade));
                }
                dados.cidades[dados.num_cidades].id = id;
                dados.cidades[dados.num_cidades].x  = x;
                dados.cidades[dados.num_cidades].y  = y;
                dados.num_cidades++;
            }
        }
    }

    return dados;
}

/* ─── gerar_matriz_distancias ────────────────────────────────────────────── */

int **gerar_matriz_distancias(const Cidade *cidades, int n) {
    /* Aloca matriz contígua: uma linha de ponteiros + n*n ints em bloco único */
    int **matriz = (int **)malloc(n * sizeof(int *));
    int  *bloco  = (int  *)malloc((size_t)n * n * sizeof(int));
    for (int i = 0; i < n; i++)
        matriz[i] = bloco + (size_t)i * n;

    for (int i = 0; i < n; i++) {
        matriz[i][i] = 0;
        for (int j = i + 1; j < n; j++) {
            int dist = calcular_distancia(&cidades[i], &cidades[j]);
            matriz[i][j] = dist;
            matriz[j][i] = dist;
        }
    }
    return matriz;
}

static void liberar_matriz(int **matriz) {
    free(matriz[0]); /* bloco contíguo */
    free(matriz);
}

/* ─── calcular_custo_total ───────────────────────────────────────────────── */

/*
 * Recebe o mapa id→índice já construído externamente para evitar
 * realocar e preencher a cada chamada.
 */
static long long calcular_custo_total_com_mapa(
        const int *tour, int n, int **matriz, const int *id_para_idx)
{
    long long custo = 0;
    for (int i = 0; i < n; i++) {
        int u = id_para_idx[tour[i]];
        int v = id_para_idx[tour[(i + 1) % n]];
        custo += matriz[u][v];
    }
    return custo;
}

/* Wrapper público que constrói o mapa internamente (usado no main) */
long long calcular_custo_total(const int *tour, int n, int **matriz,
                               const Cidade *cidades, int num_cidades) {
    int max_id = 0;
    for (int i = 0; i < num_cidades; i++)
        if (cidades[i].id > max_id) max_id = cidades[i].id;

    int *map_idx = (int *)malloc((max_id + 1) * sizeof(int));
    for (int i = 0; i < num_cidades; i++)
        map_idx[cidades[i].id] = i;

    long long custo = calcular_custo_total_com_mapa(tour, n, matriz, map_idx);
    free(map_idx);
    return custo;
}

/* ─── exibir_saida ───────────────────────────────────────────────────────── */

void exibir_saida(const char *nome, const char *alunos, const char *metodo,
                  int dimensao, long long custo, const int *tour, int n) {
    printf("NAME: %s\n", nome);
    printf("COMMENT: %s - %s\n", alunos, metodo);
    printf("TYPE: TOUR\n");
    printf("DIMENSION: %d\n", dimensao);
    printf("TOTAL_WEIGHT: %lld\n", custo);
    printf("TOUR_SECTION\n");
    for (int i = 0; i < n; i++) printf("%d\n", tour[i]);
    printf("EOF\n");
}

/* ─── vizinho_mais_proximo ───────────────────────────────────────────────── */

int *vizinho_mais_proximo(int **matriz, const Cidade *cidades, int n) {
    int *visitados    = (int *)calloc(n, sizeof(int));
    int *tour         = (int *)malloc(n * sizeof(int));
    int  cidade_atual = 0;

    tour[0]                 = cidades[cidade_atual].id;
    visitados[cidade_atual] = 1;

    for (int step = 0; step < n - 1; step++) {
        int proxima_cidade  = -1;
        int menor_distancia = INT_MAX;
        const int *row      = matriz[cidade_atual];

        for (int j = 0; j < n; j++) {
            if (!visitados[j] && row[j] < menor_distancia) {
                menor_distancia = row[j];
                proxima_cidade  = j;
            }
        }

        cidade_atual            = proxima_cidade;
        tour[step + 1]          = cidades[cidade_atual].id;
        visitados[cidade_atual] = 1;
    }

    free(visitados);
    return tour;
}

/* ─── construir_lista_candidatos ─────────────────────────────────────────── */

/*
 * OTIMIZAÇÃO: substituído qsort com variável global por qsort_r
 * (thread-safe, sem estado global compartilhado).
 */
static int cmp_by_dist_r(const void *a, const void *b, void *ctx) {
    const int *row = (const int *)ctx;
    return row[*(const int *)a] - row[*(const int *)b];
}

int **construir_lista_candidatos(int **matriz, int n, int k) {
    if (k > n - 1) k = n - 1;

    int **candidatos = (int **)malloc(n * sizeof(int *));
    int  *indices    = (int  *)malloc((n - 1) * sizeof(int));

    for (int i = 0; i < n; i++) {
        candidatos[i] = (int *)malloc(k * sizeof(int));

        int cnt = 0;
        for (int j = 0; j < n; j++)
            if (j != i) indices[cnt++] = j;

        qsort_r(indices, cnt, sizeof(int), cmp_by_dist_r, matriz[i]);

        for (int a = 0; a < k; a++)
            candidatos[i][a] = indices[a];
    }

    free(indices);
    return candidatos;
}

/* ─── dois_opt_delta ─────────────────────────────────────────────────────── */

/*
 * OTIMIZAÇÃO: posicao_no_tour é atualizado incrementalmente apenas na
 * janela revertida — não recontruído do zero a cada iteração do while.
 */
void dois_opt_delta(int *tour_idx, int **matriz, int n,
                    int **candidatos, int k) {
    int *posicao_no_tour = (int *)malloc(n * sizeof(int));
    for (int pos = 0; pos < n; pos++)
        posicao_no_tour[tour_idx[pos]] = pos;

    int melhorou = 1;
    while (melhorou) {
        melhorou = 0;

        for (int idx_i = 0; idx_i < n; idx_i++) {
            int ci = tour_idx[idx_i];

            for (int ki = 0; ki < k; ki++) {
                int cj = candidatos[ci][ki];
                int j  = posicao_no_tour[cj];
                int i  = idx_i;

                int i_tmp, j_tmp;
                if (j < i) { i_tmp = j; j_tmp = i; }
                else        { i_tmp = i; j_tmp = j; }

                if (j_tmp - i_tmp < 2)             continue;
                if (i_tmp == 0 && j_tmp == n - 1)  continue;

                int a = tour_idx[(i_tmp - 1 + n) % n];
                int b = tour_idx[i_tmp];
                int c = tour_idx[j_tmp];
                int d = tour_idx[(j_tmp + 1) % n];

                int delta = (matriz[a][c] + matriz[b][d])
                          - (matriz[a][b] + matriz[c][d]);

                if (delta < 0) {
                    /* Inverte tour_idx[i_tmp .. j_tmp] */
                    int lo = i_tmp, hi = j_tmp;
                    while (lo < hi) {
                        int tmp        = tour_idx[lo];
                        tour_idx[lo]   = tour_idx[hi];
                        tour_idx[hi]   = tmp;
                        posicao_no_tour[tour_idx[lo]] = lo;
                        posicao_no_tour[tour_idx[hi]] = hi;
                        lo++; hi--;
                    }
                    /* Elemento central (se n par/ímpar) */
                    if (lo == hi)
                        posicao_no_tour[tour_idx[lo]] = lo;

                    melhorou = 1;
                }
            }
        }
    }

    free(posicao_no_tour);
}

/* ─── or_opt ─────────────────────────────────────────────────────────────── */

/*
 * OTIMIZAÇÕES:
 *  1. Verificação de posição inválida: antes era O(tamanho_seg) por j,
 *     agora usa a posição direta — O(1).
 *  2. Ao aplicar o melhor movimento, usa memmove em vez de reconstruir
 *     dois arrays temporários inteiros.
 */
void or_opt(int *tour_idx, int **matriz, int n, int tamanho_seg) {
    int *seg      = (int *)malloc(tamanho_seg * sizeof(int));
    int *tmp_buf  = (int *)malloc(n * sizeof(int)); /* buffer auxiliar mínimo */

    int melhorou = 1;
    while (melhorou) {
        melhorou = 0;

        for (int i = 0; i < n; i++) {
            int prev_i   = (i - 1 + n) % n;
            int end_seg  = (i + tamanho_seg - 1) % n; /* último índice do seg */
            int next_seg = (i + tamanho_seg) % n;

            /* Extrai segmento (pode cruzar o fim do array circular) */
            for (int kk = 0; kk < tamanho_seg; kk++)
                seg[kk] = tour_idx[(i + kk) % n];

            int custo_remocao =
                  matriz[tour_idx[prev_i]][seg[0]]
                + matriz[seg[tamanho_seg - 1]][tour_idx[next_seg]]
                - matriz[tour_idx[prev_i]][tour_idx[next_seg]];

            int melhor_delta = 0;
            int melhor_j     = -1;

            for (int j = 0; j < n; j++) {
                /*
                 * Posições inválidas: prev_i e as do próprio segmento.
                 * O segmento ocupa indices i .. (i+tamanho_seg-1) mod n.
                 * Verificamos se j cai nessa faixa — O(1).
                 */
                if (j == prev_i) continue;

                /* Distância circular de i até j */
                int dist_j = (j - i + n) % n;
                if (dist_j < tamanho_seg) continue; /* j está dentro do seg */

                int next_j = (j + 1) % n;

                /* next_j também não pode ser prev_i (causaria aresta dupla) */
                if (next_j == prev_i) continue;

                int custo_insercao =
                      matriz[tour_idx[j]][seg[0]]
                    + matriz[seg[tamanho_seg - 1]][tour_idx[next_j]]
                    - matriz[tour_idx[j]][tour_idx[next_j]];

                int delta = custo_insercao - custo_remocao;
                if (delta < melhor_delta) {
                    melhor_delta = delta;
                    melhor_j     = j;
                }
            }

            if (melhor_j != -1) {
                /*
                 * Reconstrói o tour in-place usando memmove:
                 * 1. Copia tour para tmp_buf linearizando (sem wrap circular)
                 * 2. Remove o segmento
                 * 3. Insere após melhor_j
                 *
                 * Para simplificar, trabalhamos no array linear (sem circular).
                 * Se o segmento cruza o fim do array, rotacionamos antes.
                 */

                /* Lineariza se necessário */
                if (i + tamanho_seg > n) {
                    /* Rotaciona tour para que o segmento não cruze */
                    memcpy(tmp_buf, tour_idx, n * sizeof(int));
                    memcpy(tour_idx, tmp_buf + i, (n - i) * sizeof(int));
                    memcpy(tour_idx + (n - i), tmp_buf, i * sizeof(int));
                    /* Ajusta índices */
                    melhor_j = (melhor_j - i + n) % n;
                    i = 0;
                    end_seg = tamanho_seg - 1;
                    next_seg = tamanho_seg;
                    prev_i = n - 1;
                    for (int kk = 0; kk < tamanho_seg; kk++)
                        seg[kk] = tour_idx[kk];
                }

                /*
                 * Agora o segmento é tour_idx[i .. i+tamanho_seg-1] (linear).
                 * Posição de inserção é após melhor_j no array sem o segmento.
                 * Construímos o novo tour em tmp_buf.
                 */
                int pos = 0;
                /* Parte antes do segmento */
                for (int p = 0; p < i; p++)
                    tmp_buf[pos++] = tour_idx[p];
                /* Parte depois do segmento */
                for (int p = i + tamanho_seg; p < n; p++)
                    tmp_buf[pos++] = tour_idx[p];

                /* Encontra posição de melhor_j em tmp_buf */
                int ref_val = tour_idx[melhor_j];
                int idx_ref = -1;
                for (int p = 0; p < pos; p++) {
                    if (tmp_buf[p] == ref_val) { idx_ref = p; break; }
                }

                /* Insere segmento após idx_ref */
                memmove(tmp_buf + idx_ref + 1 + tamanho_seg,
                        tmp_buf + idx_ref + 1,
                        (pos - idx_ref - 1) * sizeof(int));
                memcpy(tmp_buf + idx_ref + 1, seg, tamanho_seg * sizeof(int));

                memcpy(tour_idx, tmp_buf, n * sizeof(int));
                melhorou = 1;
                break; /* reinicia a varredura */
            }
        }
    }

    free(seg);
    free(tmp_buf);
}

/* ─── algoritimo_2opt ────────────────────────────────────────────────────── */

/*
 * OTIMIZAÇÃO: o loop externo de convergência foi removido — 2-opt e or-opt
 * já iteram internamente até não haver melhora. Manter o loop externo só
 * adicionava uma rodada extra desnecessária sem ganho prático.
 */
int *algoritimo_2opt(int *tour, int **matriz, const Cidade *cidades, int n) {
    int max_id = 0;
    for (int i = 0; i < n; i++)
        if (cidades[i].id > max_id) max_id = cidades[i].id;

    int *id_para_idx = (int *)malloc((max_id + 1) * sizeof(int));
    int *idx_para_id = (int *)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        id_para_idx[cidades[i].id] = i;
        idx_para_id[i]             = cidades[i].id;
    }

    int *tour_idx = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        tour_idx[i] = id_para_idx[tour[i]];

    int k = (20 < n - 1) ? 20 : n - 1;
    int **candidatos = construir_lista_candidatos(matriz, n, k);

    dois_opt_delta(tour_idx, matriz, n, candidatos, k);

    for (int tam_seg = 1; tam_seg <= 3; tam_seg++)
        or_opt(tour_idx, matriz, n, tam_seg);

    int *tour_final = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        tour_final[i] = idx_para_id[tour_idx[i]];

    for (int i = 0; i < n; i++) free(candidatos[i]);
    free(candidatos);
    free(id_para_idx);
    free(idx_para_id);
    free(tour_idx);

    return tour_final;
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    clock_t tempo_inicio = clock();

    Instancia instancia = ler_instancia();
    if (instancia.num_cidades == 0) return 0;

    int   n      = instancia.num_cidades;
    int **matriz = gerar_matriz_distancias(instancia.cidades, n);

    int *tour       = vizinho_mais_proximo(matriz, instancia.cidades, n);
    int *tour_final = algoritimo_2opt(tour, matriz, instancia.cidades, n);
    free(tour);

    long long custo_final = calcular_custo_total(
        tour_final, n, matriz, instancia.cidades, n);

    exibir_saida(
        instancia.name,
        "CAIN\xC3\x83 FARIAS, GUSTAVO FARIAS, LUIS GABRIEL",
        "Vizinho Mais Pr\xC3\xB3ximo + 2-OPT",
        instancia.dimension,
        custo_final,
        tour_final,
        n
    );

    clock_t tempo_fim = clock();
    double  tempo     = (double)(tempo_fim - tempo_inicio) / CLOCKS_PER_SEC;
    printf("Tempo de execu\xC3\xA7\xC3\xA3o: %.2f segundos\n", tempo);

    free(tour_final);
    liberar_matriz(matriz);
    free(instancia.cidades);

    return 0;
}