# Trabalho Prático – Problema do Caixeiro Viajante

- **Integrantes:** Cainã Farias, Gustavo Farias e Luis Gabriel
- **Método:** ILS: VMP-multistart + 2opt + ILS(double-bridge) + OrOpt (time=1.9s)

# Instruções
Você precisa ter instalado o **GCC (GNU Compiler Collection)** em sua máquina.

## 1. Compilação
- **No Linux:**
    ```bash
    gcc -O3 main.c -o programa -lm
    ```
    > **Nota:** A flag `-lm` é obrigatória no Linux para vincular a biblioteca matemática `<math.h>` usada no cálculo de distância.

- **No Windows (MinGW / GCC):**
    ```cmd
    gcc -O3 main.c -o programa.exe
    ```

## 2. Execução
### Exibindo o resultado direto no terminal
Se quiser apenas testar a saída e visualizar a estrutura dos campos no prompt:
* **Linux:** `./programa < instancia.tsp`
* **Windows:** `programa.exe < instancia.tsp`

### Salvando a solução em um arquivo `.tour`
Para gerar o arquivo final de saída:
* **No Linux:**
    ```bash
    ./programa < instancia.tsp > instancia.tsp.tour
    ```

* **No Windows:**
    ```cmd
    programa.exe < instancia.tsp > instancia.tsp.tour
    ```
## 3. Execução dos testes em lote (Medição de tempos)

Antes de executar, crie uma pasta chamada `instancias` no mesmo diretório do programa e coloque os arquivos `.tsp` dentro dela. O script processará todos os arquivos encontrados e consolidará os resultados no arquivo `resultados.csv`.

### Como executar o script:
```bash
python testes.py