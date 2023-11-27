#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

void printTopInfo() {
    // Abre o arquivo /proc/loadavg para obter informações sobre a carga média do CPU
    FILE *loadavg_file = fopen("/proc/loadavg", "r");
    if (loadavg_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Lê as informações sobre a carga média do CPU
    double loadavg_1min, loadavg_5min, loadavg_15min;
    fscanf(loadavg_file, "%lf %lf %lf", &loadavg_1min, &loadavg_5min, &loadavg_15min);
    fclose(loadavg_file);

    // Imprime as informações
    printf("Load Average: %.2f %.2f %.2f\n", loadavg_1min, loadavg_5min, loadavg_15min);

    // Conta o número total de processos e o número de processos em execução (running)
    int total_processes = 0, running_processes = 0;

    // Abre o diretório /proc para obter informações sobre os processos
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;

    // Loop para percorrer os diretórios de processos
    while ((entry = readdir(proc_dir)) != NULL) {
        // Verifica se o nome do diretório é um número (PID)
        if (atoi(entry->d_name) > 0) {
            total_processes++;

            // Constrói o caminho completo do diretório do processo
            char proc_path[512];
            if (snprintf(proc_path, sizeof(proc_path), "/proc/%s/status", entry->d_name) >= sizeof(proc_path)) {
                fprintf(stderr, "Erro: Caminho do processo truncado\n");
                exit(EXIT_FAILURE);
            }
            // Abre o arquivo /proc/[PID]/status para obter o estado do processo
            FILE *status_file = fopen(proc_path, "r");
            if (status_file == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            // Lê o estado do processo
            char status[256];
            while (fscanf(status_file, "%*s %s", status) == 1) {
                if (strcmp(status, "R") == 0) {
                    running_processes++;
                    break;
                }
            }

            fclose(status_file);
        }
    }

    closedir(proc_dir);

    // Imprime as informações sobre os processos
    printf("Total processes: %d\nRunning processes: %d\n", total_processes, running_processes);
}

void executeCommandTop() {
    char input;

    do {
        // Exibe informações sobre o sistema e os processos
        printTopInfo();

        // Espera 10 segundos antes de atualizar as informações
        sleep(10);

        // Limpa a consola
        system("clear");

        printf("Insira 'q' para terminar ou outra tecla para continuar: ");
        // Limpa o buffer do teclado
        while (getchar() != '\n');
        // Lê o caractere do usuário
        scanf("%c", &input);
    } while (input != 'q');
}

void executeCommand(char *cmd, char *args[]) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Código do processo filho
        // Executa o comando
        execvp(cmd, args);

        // Se o exec falhar, imprime uma mensagem de erro
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Código do processo pai
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "O comando '%s' falhou com o código de saída %d\n", cmd, WEXITSTATUS(status));
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    int args = argc - 1;

    //Verifica se não existe argumentos alem do comando
    if (args == 0) {
        executeCommandTop();
        return 0;
    }

    // Variáveis relativas aos nomes dos arquivos
    char *inputFile = NULL;
    char *outputFile = NULL;

    // Inicializa as flags
    int found = 0;
    char *operator = NULL;
    int argc_cmd1 = 0;
    int argc_cmd2 = 0;

    // Aloca memória para os arrays de argumentos do comando
    char **argv_cmd1 = malloc((argc + 1) * sizeof(char *));
    char **argv_cmd2 = malloc((argc + 1) * sizeof(char *));

    // Verifica se a alocação de memória foi bem-sucedida
    if (argv_cmd1 == NULL || argv_cmd2 == NULL) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        exit(EXIT_FAILURE);
    }

    // Loop para percorrer os argumentos
    for (int i = 1; i < argc; i++) {
        // Verifica se o argumento é um operador de redirecionamento ou um pipe
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "|") == 0) {
            found = 1;
            operator = argv[i];
            continue;
        }

        // Se não foi encontrado nenhum operador, adiciona ao contexto do cmd1
        if (!found) {
            argv_cmd1[argc_cmd1++] = strdup(argv[i]);
        } else {
            // Se um operador foi encontrado, adiciona ao contexto do cmd2
            argv_cmd2[argc_cmd2++] = strdup(argv[i]);
        }
    }

    // Adiciona NULL no final dos arrays para indicar o fim
    argv_cmd1[argc_cmd1] = NULL;
    argv_cmd2[argc_cmd2] = NULL;

    // Verifica se um operador foi encontrado
    if (found) {
        // Se o cmd2 tiver mais de 1 argumento, assume-se que o primeiro é o arquivo de entrada e o segundo é o de saída
        if (argc_cmd2 > 1) {
            inputFile = argv_cmd2[0];
            outputFile = argv_cmd2[1];
        } else if (argc_cmd2 > 0) {
            // Se o cmd2 tiver apenas 1 argumento, assume-se que é o arquivo de entrada
            inputFile = argv_cmd2[0];
        }
    }

    // Executa o comando cmd1, possivelmente com redirecionamento
    executeCommand(argv_cmd1[0], argv_cmd1);

    // Libera a memória alocada anteriormente para evitar "memory leaks"
    for (int i = 0; i < argc_cmd1; i++) {
        free(argv_cmd1[i]);
    }
    for (int i = 0; i < argc_cmd2; i++) {
        free(argv_cmd2[i]);
    }
    free(argv_cmd1);
    free(argv_cmd2);

    return 0;
}