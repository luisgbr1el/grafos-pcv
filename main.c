#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#define MAX_NAME 256
#define MAX_LINE 4096

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

static long long custo_tour(const int *tour, int n, int **mat) {
    long long c = 0;
    for (int i = 0; i < n; i++)
        c += mat[tour[i]][tour[(i == n - 1) ? 0 : i + 1]];
    return c;
}

long long calcular_custo_total(const int *tour, int n, int **matriz,
                               const Cidade *cidades, int num_cidades) {
    int max_id = 0;
    for (int i = 0; i < num_cidades; i++)
        if (cidades[i].id > max_id) max_id = cidades[i].id;

    int *map_idx = (int *)malloc((max_id + 1) * sizeof(int));
    for (int i = 0; i < num_cidades; i++)
        map_idx[cidades[i].id] = i;

    long long custo = 0;
    for (int i = 0; i < n; i++) {
        int u = map_idx[tour[i]];
        int v = map_idx[tour[(i == n - 1) ? 0 : i + 1]];
        custo += matriz[u][v];
    }
    free(map_idx);
    return custo;
}

void exibir_saida(const char *nome, const char *alunos, const char *metodo,
                  int dimensao, long long custo, const int *tour, int n) {
    printf("NAME: %s\n", nome);
    printf("COMMENT: %s - %s\n", alunos, metodo);
    printf("TYPE: TOUR\n");
    printf("DIMENSION: %d\n", dimensao);
    printf("TOTAL WEIGHT: %lld\n", custo); /* Ajustado sem underline */
    printf("TOUR_SECTION\n");
    for (int i = 0; i < n; i++) printf("%d\n", tour[i]);
    printf("EOF\n");
}

int *vizinho_mais_proximo(int **matriz, const Cidade *cidades, int n, int num_starts) {
    if (num_starts < 1) num_starts = 1;
    if (num_starts > n) num_starts = n;

    int *melhor_tour      = NULL;
    long long melhor_custo = LLONG_MAX;

    int *visitados = (int *)malloc(n * sizeof(int));
    int *tour_idx  = (int *)malloc(n * sizeof(int));

    int passo = n / num_starts;

    for (int s = 0; s < num_starts; s++) {
        int inicio = s * passo;

        memset(visitados, 0, n * sizeof(int));
        tour_idx[0]       = inicio;
        visitados[inicio] = 1;
        int cidade_atual  = inicio;

        for (int step = 0; step < n - 1; step++) {
            int proxima    = -1;
            int menor_dist = INT_MAX;
            const int *row = matriz[cidade_atual];

            for (int j = 0; j < n; j++) {
                if (!visitados[j] && row[j] < menor_dist) {
                    menor_dist = row[j];
                    proxima    = j;
                }
            }

            cidade_atual            = proxima;
            tour_idx[step + 1]      = cidade_atual;
            visitados[cidade_atual] = 1;
        }

        long long custo = custo_tour(tour_idx, n, matriz);

        if (custo < melhor_custo) {
            melhor_custo = custo;
            if (!melhor_tour) melhor_tour = (int *)malloc(n * sizeof(int));
            for (int i = 0; i < n; i++)
                melhor_tour[i] = cidades[tour_idx[i]].id;
        }
    }

    free(visitados);
    free(tour_idx);
    return melhor_tour;
}

static int cmp_by_dist(const void *a, const void *b) {
    return ((const CandidatoAux *)a)->distancia - ((const CandidatoAux *)b)->distancia;
}

int **construir_lista_candidatos(int **matriz, int n, int k) {
    if (k > n - 1) k = n - 1;

    int **candidatos  = (int **)malloc(n * sizeof(int *));
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

    char *ignorar = (char *)calloc(n, sizeof(char));

    int melhorou = 1;
    while (melhorou) {
        melhorou = 0;

        for (int idx_i = 0; idx_i < n; idx_i++) {
            int ci = tour_idx[idx_i];
            if (ignorar[ci]) continue;

            int gerou_melhoria = 0;

            for (int ki = 0; ki < k; ki++) {
                int cj = candidatos[ci][ki];
                int j  = posicao_no_tour[cj];
                int i  = idx_i;

                int i_tmp, j_tmp;
                if (j < i) { i_tmp = j; j_tmp = i; }
                else       { i_tmp = i; j_tmp = j; }

                if (j_tmp - i_tmp < 2)            continue;
                if (i_tmp == 0 && j_tmp == n - 1) continue;

                /* Remoção do gargalo do módulo % n */
                int prev_i = (i_tmp == 0) ? n - 1 : i_tmp - 1;
                int next_j = (j_tmp == n - 1) ? 0 : j_tmp + 1;

                int a = tour_idx[prev_i];
                int b = tour_idx[i_tmp];
                int c = tour_idx[j_tmp];
                int d = tour_idx[next_j];

                int delta = (matriz[a][c] + matriz[b][d])
                          - (matriz[a][b] + matriz[c][d]);

                if (delta < 0) {
                    int lo = i_tmp, hi = j_tmp;
                    while (lo < hi) {
                        int tmp           = tour_idx[lo];
                        tour_idx[lo]      = tour_idx[hi];
                        tour_idx[hi]      = tmp;
                        posicao_no_tour[tour_idx[lo]] = lo;
                        posicao_no_tour[tour_idx[hi]] = hi;
                        lo++; hi--;
                    }
                    if (lo == hi)
                        posicao_no_tour[tour_idx[lo]] = lo;

                    ignorar[a] = 0; ignorar[b] = 0;
                    ignorar[c] = 0; ignorar[d] = 0;

                    melhorou       = 1;
                    gerou_melhoria = 1;
                    break;
                }
            }

            if (!gerou_melhoria)
                ignorar[ci] = 1;
        }
    }

    free(posicao_no_tour);
    free(ignorar);
}

void or_opt_completo(int *tour_idx, int **matriz, int n, int tamanho_seg) {
    int *seg     = (int *)malloc(tamanho_seg * sizeof(int));
    int *tmp_buf = (int *)malloc(n * sizeof(int));

    int melhorou = 1;
    while (melhorou) {
        melhorou = 0;

        for (int i = 0; i < n; i++) {
            /* Remoção do módulo no or_opt */
            int prev_i   = (i == 0) ? n - 1 : i - 1;
            int next_seg = i + tamanho_seg;
            if (next_seg >= n) next_seg -= n;

            for (int kk = 0; kk < tamanho_seg; kk++) {
                int pos_kk = i + kk;
                if (pos_kk >= n) pos_kk -= n;
                seg[kk] = tour_idx[pos_kk];
            }

            int custo_remocao =
                  matriz[tour_idx[prev_i]][seg[0]]
                + matriz[seg[tamanho_seg - 1]][tour_idx[next_seg]]
                - matriz[tour_idx[prev_i]][tour_idx[next_seg]];

            int melhor_delta = 0;
            int melhor_j     = -1;
            int melhor_rev   = 0;

            for (int j = 0; j < n; j++) {
                if (j == prev_i) continue;

                int dist_j = j - i;
                if (dist_j < 0) dist_j += n;
                if (dist_j < tamanho_seg) continue;

                int next_j = (j == n - 1) ? 0 : j + 1;
                if (next_j == prev_i) continue;

                int custo_ins =
                      matriz[tour_idx[j]][seg[0]]
                    + matriz[seg[tamanho_seg - 1]][tour_idx[next_j]]
                    - matriz[tour_idx[j]][tour_idx[next_j]];

                int delta = custo_ins - custo_remocao;
                if (delta < melhor_delta) {
                    melhor_delta = delta;
                    melhor_j     = j;
                    melhor_rev   = 0;
                }

                if (tamanho_seg > 1) {
                    int custo_rev =
                          matriz[tour_idx[j]][seg[tamanho_seg - 1]]
                        + matriz[seg[0]][tour_idx[next_j]]
                        - matriz[tour_idx[j]][tour_idx[next_j]];

                    delta = custo_rev - custo_remocao;
                    if (delta < melhor_delta) {
                        melhor_delta = delta;
                        melhor_j     = j;
                        melhor_rev   = 1;
                    }
                }
            }

            if (melhor_j != -1) {
                if (melhor_rev) {
                    int lo = 0, hi = tamanho_seg - 1;
                    while (lo < hi) {
                        int tmp = seg[lo]; seg[lo] = seg[hi]; seg[hi] = tmp;
                        lo++; hi--;
                    }
                }

                if (i + tamanho_seg > n) {
                    memcpy(tmp_buf, tour_idx, n * sizeof(int));
                    memcpy(tour_idx, tmp_buf + i, (n - i) * sizeof(int));
                    memcpy(tour_idx + (n - i), tmp_buf, i * sizeof(int));
                    
                    int diff = melhor_j - i;
                    if (diff < 0) diff += n;
                    melhor_j = diff;
                    
                    i        = 0;
                    next_seg = tamanho_seg;
                    prev_i   = n - 1;
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

/* O 2-opt já garante atingir o ótimo local, o laço e reavaliação foram removidos! */
void otimizacao_local_rapida(int *tour_idx, int **matriz, int n, int **candidatos, int k) {
    dois_opt_delta(tour_idx, matriz, n, candidatos, k);
}

void otimizacao_local_pesada(int *tour_idx, int **matriz, int n, int **candidatos, int k) {
    int melhorou_global = 1;
    while (melhorou_global) {
        melhorou_global = 0;

        long long antes = custo_tour(tour_idx, n, matriz);
        dois_opt_delta(tour_idx, matriz, n, candidatos, k);
        long long depois = custo_tour(tour_idx, n, matriz);
        if (depois < antes) melhorou_global = 1;

        for (int tam = 1; tam <= 3; tam++) {
            antes = custo_tour(tour_idx, n, matriz);
            or_opt_completo(tour_idx, matriz, n, tam);
            depois = custo_tour(tour_idx, n, matriz);
            if (depois < antes) melhorou_global = 1;
        }
    }
}

/* Perturbação agora corta de forma UNIFORME na rota toda */
void double_bridge(const int *tour, int *novo_tour, int n) {
    int p1 = 1 + rand() % (n - 1);
    int p2 = 1 + rand() % (n - 1);
    int p3 = 1 + rand() % (n - 1);

    while(p1 == p2) p2 = 1 + rand() % (n - 1);
    while(p1 == p3 || p2 == p3) p3 = 1 + rand() % (n - 1);

    if(p1 > p2) { int t = p1; p1 = p2; p2 = t; }
    if(p2 > p3) { int t = p2; p2 = p3; p3 = t; }
    if(p1 > p2) { int t = p1; p1 = p2; p2 = t; }

    int pos[4] = {0, p1, p2, p3};

    int idx = 0;
    for (int i = pos[0]; i < pos[1]; i++) novo_tour[idx++] = tour[i];
    for (int i = pos[2]; i < pos[3]; i++) novo_tour[idx++] = tour[i];
    for (int i = pos[1]; i < pos[2]; i++) novo_tour[idx++] = tour[i];
    for (int i = pos[3]; i < n;      i++) novo_tour[idx++] = tour[i];
}

int *ils(int **matriz, const Cidade *cidades, int n, int k,
         double tempo_limite, unsigned int seed) {

    clock_t tempo_inicio = clock();
    srand(seed);

    int num_starts = (n <= 100) ? 5 : (n <= 500) ? 3 : 1;
    int *tour_id   = vizinho_mais_proximo(matriz, cidades, n, num_starts);

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
        tour_idx[i] = id_para_idx[tour_id[i]];
    free(tour_id);

    int **candidatos = construir_lista_candidatos(matriz, n, k);

    otimizacao_local_rapida(tour_idx, matriz, n, candidatos, k);

    int *melhor       = (int *)malloc(n * sizeof(int));
    int *tour_perturb = (int *)malloc(n * sizeof(int));

    memcpy(melhor, tour_idx, n * sizeof(int));
    long long melhor_custo = custo_tour(melhor, n, matriz);

    int iteracoes_feitas = 0;

    while (1) {
        double tempo_decorrido = (double)(clock() - tempo_inicio) / CLOCKS_PER_SEC;
        if (tempo_decorrido >= (tempo_limite - 0.08)) break;

        double_bridge(melhor, tour_perturb, n);

        otimizacao_local_rapida(tour_perturb, matriz, n, candidatos, k);

        long long custo_perturb = custo_tour(tour_perturb, n, matriz);

        /* Agora aceita rotas IGUAIS para conseguir "andar em platôs" */
        if (custo_perturb <= melhor_custo) {
            melhor_custo = custo_perturb;
            memcpy(melhor, tour_perturb, n * sizeof(int));
        }
        iteracoes_feitas++;
    }

    otimizacao_local_pesada(melhor, matriz, n, candidatos, k);

    fprintf(stderr, "ILS completou %d iteracoes.\n", iteracoes_feitas);

    int *resultado = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        resultado[i] = idx_para_id[melhor[i]];

    for (int i = 0; i < n; i++) free(candidatos[i]);
    free(candidatos);
    free(id_para_idx);
    free(idx_para_id);
    free(tour_idx);
    free(melhor);
    free(tour_perturb);

    return resultado;
}

int main(int argc, char *argv[]) {
    int k_candidatos    = 25; 
    unsigned int seed   = 12345; 

    if (argc >= 2) {
        seed = (unsigned int)atoi(argv[1]);
    }

    Instancia instancia = ler_instancia();
    if (instancia.num_cidades == 0) return 0;

    int n = instancia.num_cidades;

    if (k_candidatos > n - 1) {
        k_candidatos = n - 1;
    }
    if (k_candidatos < 1) {
        k_candidatos = 1;
    }

    double tempo_limite = (n <= 100) ? 0.4 : (n <= 500) ? 1.2 : 1.85;

    int **matriz = gerar_matriz_distancias(instancia.cidades, n);

    int *tour_final = ils(matriz, instancia.cidades, n,
                          k_candidatos, tempo_limite, seed);

    long long custo_final = calcular_custo_total(
        tour_final, n, matriz, instancia.cidades, n);

    char metodo[256]; 
    snprintf(metodo, sizeof(metodo),
             "ILS: VMP-multistart + 2opt(Fast) + ILS(DB) + OrOpt (k=%d, time=%.1fs, seed=%u)",
             k_candidatos, tempo_limite, seed);

    exibir_saida(
        instancia.name,
        "CAINA FARIAS, GUSTAVO FARIAS, LUIS GABRIEL",
        metodo,
        instancia.dimension,
        custo_final,
        tour_final,
        n
    );

    free(tour_final);
    liberar_matriz(matriz);
    free(instancia.cidades);

    return 0;
}