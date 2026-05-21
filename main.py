import sys
import math
import time

class Cidade:
    def __init__(self, id_cidade, x, y):
        self.id = int(id_cidade)
        self.x = float(x)
        self.y = float(y)

def calcular_distancia(c1, c2):
    dist_euclidiana = math.sqrt((c1.x - c2.x)**2 + (c1.y - c2.y)**2)

    return math.floor(0.5 + dist_euclidiana)

def ler_instancia():
    dados = {
        'NAME': None,
        'DIMENSION': 0,
        'CIDADES': []
    }

    lendo_coords = False

    for linha in sys.stdin:
        linha = linha.strip()

        if not linha or linha == "EOF":
            break

        if ":" in linha and not lendo_coords:
            chave, valor = linha.split(":", 1)
            chave = chave.strip()
            valor = valor.strip()
            
            if chave == "NAME":
                dados['NAME'] = valor
            elif chave == "DIMENSION":
                dados['DIMENSION'] = int(valor)
            continue

        if linha.startswith("NODE_COORD_SECTION"):
            lendo_coords = True
            continue

        if lendo_coords:
            partes = linha.split()
            
            if len(partes) >= 3:
                cidade = Cidade(partes[0], partes[1], partes[2])
                dados['CIDADES'].append(cidade)

    return dados

def gerar_matriz_distancias(cidades):
    n = len(cidades)
    matriz = [[0] * n for x in range(n)]

    for i in range(n):
        for j in range(i + 1, n):
            dist = calcular_distancia(cidades[i], cidades[j])

            matriz[i][j] = dist
            matriz[j][i] = dist
    
    return matriz

def calcular_custo_total(tour, matriz_distancias, cidades):
    custo = 0
    n = len(tour)

    map_idx = { cidade.id: i for i, cidade in enumerate(cidades) }

    for i in range(n):
        u = map_idx[tour[i]]
        v = map_idx[tour[(i + 1) % n]]

        custo += matriz_distancias[u][v]

    return custo

def exibir_saida(nome, alunos, metodo, dimensao, custo, tour):
    print(f"NAME: {nome}")
    print(f"COMMENT: {alunos} - {metodo}")
    print(f"TYPE: TOUR")
    print(f"DIMENSION: {dimensao}")
    print(f"TOTAL_WEIGHT: {custo}")
    print(f"TOUR_SECTION")
    
    for cidade in tour:
        print(cidade)

    print("EOF")

def vizinho_mais_proximo(matriz_distancias, cidades):
    n = len(cidades)
    visitados = [False] * n
    tour = []
    
    # Começamos pela primeira cidade (índice 0)
    cidade_atual = 0
    tour.append(cidades[cidade_atual].id)
    visitados[cidade_atual] = True
    
    # Precisamos visitar as n-1 cidades restantes
    for _ in range(n - 1):
        proxima_cidade = -1
        menor_distancia = float('inf')
        
        # Busca a cidade mais próxima não visitada
        for j in range(n):
            if not visitados[j] and matriz_distancias[cidade_atual][j] < menor_distancia:
                menor_distancia = matriz_distancias[cidade_atual][j]
                proxima_cidade = j
                
        # Atualiza a rota
        cidade_atual = proxima_cidade
        tour.append(cidades[cidade_atual].id)
        visitados[cidade_atual] = True
        
    return tour


def construir_lista_candidatos(matriz, n, k=20):
    k = min(k, n - 1)
    candidatos = []
    for i in range(n):
        vizinhos = sorted(
            [j for j in range(n) if j != i],
            key=lambda x: matriz[i][x]
        )
        candidatos.append(vizinhos[:k])
    return candidatos

def dois_opt_delta(tour_idx, matriz, n, candidatos):
    melhorou = True
    while melhorou:
        melhorou = False
        posicao_no_tour = [0] * n
        for pos, cidade in enumerate(tour_idx):
            posicao_no_tour[cidade] = pos

        for idx_i in range(n):
            i = idx_i
            ci = tour_idx[i]
            prev_ci = tour_idx[(i - 1) % n]

            for cj in candidatos[ci]:
                j = posicao_no_tour[cj]

                if j < i:
                    i_tmp, j_tmp = j, i
                else:
                    i_tmp, j_tmp = i, j

                if j_tmp - i_tmp < 2:
                    continue
                if i_tmp == 0 and j_tmp == n - 1:
                    continue

                a = tour_idx[i_tmp - 1]
                b = tour_idx[i_tmp]
                c = tour_idx[j_tmp]
                d = tour_idx[(j_tmp + 1) % n]

                delta = (matriz[a][c] + matriz[b][d] - matriz[a][b] - matriz[c][d])

                if delta < -1e-10:
                    tour_idx[i_tmp:j_tmp + 1] = tour_idx[i_tmp:j_tmp + 1][::-1]
                    melhorou = True
                    for pos in range(i_tmp, j_tmp + 1):
                        posicao_no_tour[tour_idx[pos]] = pos

    return tour_idx

def or_opt(tour_idx, matriz, n, tamanho_seg=1):
    melhorou = True
    while melhorou:
        melhorou = False
        i = 0
        while i < n:
            prev_i  = (i - 1) % n
            end_seg = (i + tamanho_seg - 1) % n
            next_seg = (i + tamanho_seg) % n

            seg = [tour_idx[(i + k) % n] for k in range(tamanho_seg)]

            custo_remocao = (
                matriz[tour_idx[prev_i]][seg[0]] +
                matriz[seg[-1]][tour_idx[next_seg]] -
                matriz[tour_idx[prev_i]][tour_idx[next_seg]]
            )

            melhor_delta = 0
            melhor_j = -1

            for j in range(n):
                invalido = False
                for k in range(-1, tamanho_seg + 1):
                    if j == (i + k) % n:
                        invalido = True
                        break
                if invalido:
                    continue

                next_j = (j + 1) % n
                custo_insercao = (
                    matriz[tour_idx[j]][seg[0]] +
                    matriz[seg[-1]][tour_idx[next_j]] -
                    matriz[tour_idx[j]][tour_idx[next_j]]
                )
                delta = custo_insercao - custo_remocao
                
                if delta < melhor_delta:
                    melhor_delta = delta
                    melhor_j = j

            if melhor_j != -1:
                posicoes = [(i + k) % n for k in range(tamanho_seg)]
                novo_tour = [tour_idx[p] for p in range(n) if p not in posicoes]
                
                ref_cidade = tour_idx[melhor_j]
                idx_ref = novo_tour.index(ref_cidade)
                
                for k, cidade in enumerate(seg):
                    novo_tour.insert(idx_ref + 1 + k, cidade)
                
                tour_idx[:] = novo_tour
                melhorou = True
                break  
            i += 1

    return tour_idx

def algoritimo_2opt(tour, matriz_distancias, cidades):
    """
    Versão melhorada do 2-OPT para competição.
    """
    n = len(cidades)

    id_para_idx = {cidade.id: i for i, cidade in enumerate(cidades)}
    idx_para_id = {i: cidade.id for i, cidade in enumerate(cidades)}

    tour_idx = [id_para_idx[cid] for cid in tour]

    k = min(20, n - 1)
    candidatos = construir_lista_candidatos(matriz_distancias, n, k)

    melhorou_geral = True
    while melhorou_geral:
        melhorou_geral = False

        custo_antes = sum(
            matriz_distancias[tour_idx[i]][tour_idx[(i + 1) % n]]
            for i in range(n)
        )

        tour_idx = dois_opt_delta(tour_idx, matriz_distancias, n, candidatos)

        for tam_seg in [1, 2, 3]:
            tour_idx = or_opt(tour_idx, matriz_distancias, n, tam_seg)

        custo_depois = sum(
            matriz_distancias[tour_idx[i]][tour_idx[(i + 1) % n]]
            for i in range(n)
        )

        if custo_depois < custo_antes - 1e-10:
            melhorou_geral = True

    tour_final = [idx_para_id[idx] for idx in tour_idx]
    return tour_final

def main():
    tempo_inicio = time.time()
    instancia = ler_instancia()

    if not instancia['CIDADES']:
        return
    
    matriz_distancias = gerar_matriz_distancias(instancia['CIDADES'])

    tour_final = vizinho_mais_proximo(matriz_distancias, instancia['CIDADES'])
    # Aplicar o 2-OPT melhora a solução inicial obtida pelo vizinho mais próximo
    tour_final = algoritimo_2opt(tour_final, matriz_distancias, instancia['CIDADES'])   

    custo_final = calcular_custo_total(tour_final, matriz_distancias, instancia['CIDADES'])

    exibir_saida(
        instancia['NAME'],
        "CAINÃ FARIAS, GUSTAVO FARIAS, LUIS GABRIEL",
        "Vizinho Mais Próximo + 2-OPT + Or-Opt",
        instancia['DIMENSION'], 
        custo_final, 
        tour_final
    )
    
    tempo_fim = time.time()
    print(f"Tempo de execução: {tempo_fim - tempo_inicio:.2f} segundos")
if __name__ == "__main__":
    main()