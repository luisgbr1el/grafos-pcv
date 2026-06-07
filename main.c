#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define MAX_NAME 256
#define MAX_LINE 1024

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

typedef struct {
    int id_cidade;
    int distancia;
} CandidatoAux;

static void trim(char *s) {
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    memmove(s, p, strlen(p) + 1);
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t')) s[--len] = '\0';
}

static inline int calcular_distancia(const Cidade *c1, const Cidade *c2) {
    double dx = c1->x - c2->x;
    double dy = c1->y - c2->y;
    return (int)floor(0.5 + sqrt(dx*dx + dy*dy));
}

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

int **gerar_matriz_distancias(const Cidade *cidades, int n) {
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
    free(matriz[0]);
    free(matriz);
}

static long long calcular_custo_total_com_mapa(
        const int *tour, int n, int **matriz, const int *id_para_idx)
{
    long long custo = 0;
    for (int i = 0; i < n; i++) {
        int u = id_para_idx[tour[i]];
        int v = id_para_idx[tour[(i + 1) % n]];
        ctx: custo += matriz[u][v];
    }
    return custo;
}

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

static int cmp_by_dist(const void *a, const void *b) {
    return ((const CandidatoAux *)a)->distancia - ((const CandidatoAux *)b)->distancia;
}

int **construir_lista_candidatos(int **matriz, int n, int k) {
    if (k > n - 1) k = n - 1;

    int **candidatos = (int **)malloc(n * sizeof(int *));
    CandidatoAux *aux = (CandidatoAux *)malloc((n - 1) * sizeof(CandidatoAux));

    for (int i = 0; i < n; i++) {
        candidatos[i] = (int *)malloc(k * sizeof(int));

        int cnt = 0;
        for (int j = 0; j < n; j++) {
            if (j != i) {
                aux[cnt].id_cidade = j;
                aux[cnt].distancia = matriz[i][j];
                cnt++;
            }
        }

        qsort(aux, cnt, sizeof(CandidatoAux), cmp_by_dist);

        for (int a = 0; a < k; a++)
            candidatos[i][a] = aux[a].id_cidade;
    }

    free(aux);
    return candidatos;
}

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
                    int lo = i_tmp, hi = j_tmp;
                    while (lo < hi) {
                        int tmp        = tour_idx[lo];
                        tour_idx[lo]   = tour_idx[hi];
                        tour_idx[hi]   = tmp;
                        posicao_no_tour[tour_idx[lo]] = lo;
                        posicao_no_tour[tour_idx[hi]] = hi;
                        lo++; hi--;
                    }
                    if (lo == hi)
                        posicao_no_tour[tour_idx[lo]] = lo;

                    melhorou = 1;
                }
            }
        }
    }

    free(posicao_no_tour);
}

void or_opt(int *tour_idx, int **matriz, int n, int tamanho_seg) {
    int *seg      = (int *)malloc(tamanho_seg * sizeof(int));
    int *tmp_buf  = (int *)malloc(n * sizeof(int));

    int melhorou = 1;
    while (melhorou) {
        melhorou = 0;

        for (int i = 0; i < n; i++) {
            int prev_i   = (i - 1 + n) % n;
            int end_seg  = (i + tamanho_seg - 1) % n;
            int next_seg = (i + tamanho_seg) % n;

            for (int kk = 0; kk < tamanho_seg; kk++)
                seg[kk] = tour_idx[(i + kk) % n];

            int custo_remocao =
                  matriz[tour_idx[prev_i]][seg[0]]
                + matriz[seg[tamanho_seg - 1]][tour_idx[next_seg]]
                - matriz[tour_idx[prev_i]][tour_idx[next_seg]];

            int melhor_delta = 0;
            int melhor_j     = -1;

            for (int j = 0; j < n; j++) {
                if (j == prev_i) continue;

                int dist_j = (j - i + n) % n;
                if (dist_j < tamanho_seg) continue;

                int next_j = (j + 1) % n;

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
                if (i + tamanho_seg > n) {
                    memcpy(tmp_buf, tour_idx, n * sizeof(int));
                    memcpy(tour_idx, tmp_buf + i, (n - i) * sizeof(int));
                    memcpy(tour_idx + (n - i), tmp_buf, i * sizeof(int));
                    melhor_j = (melhor_j - i + n) % n;
                    i = 0;
                    end_seg = tamanho_seg - 1;
                    next_seg = tamanho_seg;
                    prev_i = n - 1;
                    for (int kk = 0; kk < tamanho_seg; kk++)
                        seg[kk] = tour_idx[kk];
                }

                int pos = 0;
                for (int p = 0; p < i; p++)
                    tmp_buf[pos++] = tour_idx[p];
                for (int p = i + tamanho_seg; p < n; p++)
                    tmp_buf[pos++] = tour_idx[p];

                int ref_val = tour_idx[melhor_j];
                int idx_ref = -1;
                for (int p = 0; p < pos; p++) {
                    if (tmp_buf[p] == ref_val) { idx_ref = p; break; }
                }

                memmove(tmp_buf + idx_ref + 1 + tamanho_seg,
                        tmp_buf + idx_ref + 1,
                        (pos - idx_ref - 1) * sizeof(int));
                memcpy(tmp_buf + idx_ref + 1, seg, tamanho_seg * sizeof(int));

                memcpy(tour_idx, tmp_buf, n * sizeof(int));
                melhorou = 1;
                break;
            }
        }
    }

    free(seg);
    free(tmp_buf);
}

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
        "CAINA FARIAS, GUSTAVO FARIAS, LUIS GABRIEL",
        "Vizinho Mais Proximo + 2-Opt (Delta-Avaliacao) + Or-Opt (k=20)",
        instancia.dimension,
        custo_final,
        tour_final,
        n
    );

    clock_t tempo_fim = clock();
    double  tempo     = (double)(tempo_fim - tempo_inicio) / CLOCKS_PER_SEC;

    free(tour_final);
    liberar_matriz(matriz);
    free(instancia.cidades);

    return 0;
}