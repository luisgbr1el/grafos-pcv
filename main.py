import sys
import math

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


def main():
    instancia = ler_instancia()

    if not instancia['CIDADES']:
        return
    
    matriz_distancias = gerar_matriz_distancias(instancia['CIDADES'])

    # implementar vizinho mais próximo

    tour_final = [c.id for c in instancia['CIDADES']]

    # implementar o 2-OPT

    custo_final = calcular_custo_total(tour_final, matriz_distancias, instancia['CIDADES'])

    exibir_saida(
        instancia['NAME'],
        "CAINÃ FARIAS, GUSTAVO FARIAS, LUIS GABRIEL",
        "Vizinho Mais Próximo + 2-OPT",
        instancia['DIMENSION'], 
        custo_final, 
        tour_final
    )

if __name__ == "__main__":
    main()