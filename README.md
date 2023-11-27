# PROJETO GRUPO SO 2023/24
Este Projeto de Grupo foi realizado em virtude da Unidade Curricular Sistemas Operativos, do 2ºAno de LEGSI da Universidade do Minho.

## COMANDOS DISPONÍVEIS
- mycmd top - Apresenta uma linha com a seguinte informação sobre o
sistema: carga média do CPU (load average) nos últimos 1,
5 e 15 minutos; número total de processos e número total
de processos no estado de running. Apresenta ainda a
informação sobre os processos correntemente ativos no
sistema, nomeadamente o pid, o estado, a linha de
comando e o username. O output deste comando deve ser
refrescado em intervalos de 10s até receber o input ‘q’. Em
cada ciclo deve apresentar os processos no estado running,
ordenados por ordem crescente do PID, no máximo de 20
processos. Caso não existam 20 processos no estado
running, deverá completar a lista com os restantes
processos, também ordenados por ordem crescente do PID.

- mycmd cmd arg1 ... argn - Executa o comando Linux cmd. Por exemplo: **mycmd ls -t**.

- mycmd cmd arg1 ... argn ">" file - Executa o comando Linux cmd; redireciona a saída (output) do comando para o ficheiro file. Por exemplo: **mycmd ls -t ">" ls.txt**.

- mycmd cmd arg1 ... argn "<" file - Executa o comando Linux cmd; redireciona a entrada (input) do comando para o ficheiro file. Por exemplo: **mycmd grep Move "<" ls.txt**.

- mycmd cmd1 arg1 ... argn “|” cmd2 arg1 ... argn - Executa os comandos Linux cmd1 e cmd2; redireciona a saída do comando cmd1 para a entrada do comando cmd2. Por exemplo: **mycmd ps "|" grep bash**.
