import os
import time
import subprocess
import csv
import sys

PASTA_INSTANCIAS = "./instancias"
ARQUIVO_CSV = "resultados.csv"

if sys.platform == "win32":
    EXECUTAVEL = "output/main.exe"
else:
    EXECUTAVEL = "./main"

VALORES_K = [10, 20, 30]
VALORES_TEMPO = [0.5, 1.0, 1.5, 1.9]

if not os.path.exists(EXECUTAVEL):
    print(f"Executavel '{EXECUTAVEL}' nao encontrado.")
    print("Compile com: gcc -O2 -lm main.c -o main")
    sys.exit(1)

if not os.path.exists(PASTA_INSTANCIAS):
    print(f"A pasta '{PASTA_INSTANCIAS}' nao foi encontrada.")
    print("Crie uma pasta chamada 'instancias' e coloque os arquivos .tsp dentro dela.")
    sys.exit(1)

arquivos_tsp = sorted([f for f in os.listdir(PASTA_INSTANCIAS) if f.endswith(".tsp")])

resultados = []

print("Iniciando testes...")
print("---------------------------------------------")

for arquivo in arquivos_tsp:
    caminho_instancia = os.path.join(PASTA_INSTANCIAS, arquivo)
    nome_instancia = arquivo.replace(".tsp", "")

    with open(caminho_instancia, "r") as f_in:
        conteudo_tsp = f_in.read()

    print(f"\n[{nome_instancia}]")

    for k in VALORES_K:
        for tempo in VALORES_TEMPO:
            arquivo_tour = os.path.join(PASTA_INSTANCIAS, f"{nome_instancia}_k{k}_t{tempo}.tsp.tour")

            print(f"  k={k:>2} tempo={tempo}s... ", end="", flush=True)

            try:
                with open(arquivo_tour, "w") as f_out:
                    tempo_inicio = time.perf_counter()

                    subprocess.run(
                        [EXECUTAVEL, str(k), str(tempo)],
                        input=conteudo_tsp,
                        stdout=f_out,
                        stderr=subprocess.DEVNULL,
                        text=True,
                        check=True
                    )

                    tempo_fim = time.perf_counter()

                tempo_puro = tempo_fim - tempo_inicio

                dimensao, custo = "N/A", "N/A"
                with open(arquivo_tour, "r") as f_tour:
                    for linha in f_tour:
                        linha_lower = linha.lower().replace(" ", "_")
                        if linha_lower.startswith("dimension"):
                            dimensao = linha.split(":")[-1].strip()
                        elif linha_lower.startswith("total_weight"):
                            custo = linha.split(":")[-1].strip()

                print(f"OK (Tempo: {tempo_puro:.4f}s | Custo: {custo})")
                resultados.append([nome_instancia, dimensao, k, tempo, custo, f"{tempo_puro:.4f}"])

            except subprocess.CalledProcessError as e:
                print(f"ERRO: {e}")

with open(ARQUIVO_CSV, "w", newline="", encoding="utf-8") as f_csv:
    writer = csv.writer(f_csv)
    writer.writerow(["Instancia", "Dimensao", "K", "Tempo_limite", "Custo_Total", "Tempo_real_s"])
    writer.writerows(resultados)

print("\n---------------------------------------------")
print(f"Testes finalizados! Resultados em '{ARQUIVO_CSV}'.")