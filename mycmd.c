#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

void printTopInfo() {
    //Abre o arquivo /proc/loadavg para obter informações sobre a carga média do CPU
    FILE *loadavg_file = fopen("/proc/loadavg", "r");
    if (loadavg_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    //Lê as informações sobre a carga média do CPU
    double loadavg_1min, loadavg_5min, loadavg_15min;
    fscanf(loadavg_file, "%lf %lf %lf", &loadavg_1min, &loadavg_5min, &loadavg_15min);
    fclose(loadavg_file);
    printf("Carga media do sistema: %.2f %.2f %.2f\n", loadavg_1min, loadavg_5min, loadavg_15min);

    //Conta o número total de processos e em execução
    int total_processes = 0, running_processes = 0;

    //Abre o diretório /proc para obter informações sobre os processos
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;

    //Loop para percorrer os diretórios de processos
    while ((entry = readdir(proc_dir)) != NULL) {
        //Verifica se o nome do diretório é um número (PID)
        if (atoi(entry->d_name) > 0) {
            total_processes++;

            //Faz o caminho completo do diretório do processo
            char proc_path[512];
            if (snprintf(proc_path, sizeof(proc_path), "/proc/%s/status", entry->d_name) >= sizeof(proc_path)) {
                fprintf(stderr, "Erro: Caminho do processo incompleto!\n");
                exit(EXIT_FAILURE);
            }
            //Abre o arquivo /proc/[PID]/status para obter o estado do processo
            FILE *status_file = fopen(proc_path, "r");
            if (status_file == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            //Lê o estado do processo
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

    //Fecha o diretório /proc onde se obtem as informações sobre os processos
    closedir(proc_dir);

    //Imprime as informações sobre os processos
    printf("Total de processos: %d\nProcessos em execução: %d\n", total_processes, running_processes);

    //Abre novamente o diretório /proc para obter informações detalhadas sobre os processos em execução
    DIR *proc_dir_running = opendir("/proc");
    if (proc_dir_running == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    //Exibe informações detalhadas sobre os processos em execução
    printf("\nProcessos em execução:\n");
    printf("%-8s %-8s %-20s %s\n", "PID", "STATE", "USERNAME", "COMMAND");

    int count_processes = 0;

    //Reinicializa o diretório de processos para percorrê-lo novamente
    rewinddir(proc_dir_running);
    while ((entry = readdir(proc_dir_running)) != NULL && count_processes < 20) {
        if (atoi(entry->d_name) > 0) {
            char proc_path[512];
            if (snprintf(proc_path, sizeof(proc_path), "/proc/%s/status", entry->d_name) >= sizeof(proc_path)) {
                fprintf(stderr, "Erro: Caminho do processo incompleto!\n");
                exit(EXIT_FAILURE);
            }
            FILE *status_file = fopen(proc_path, "r");
            if (status_file == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            char status[256];
            while (fscanf(status_file, "%*s %s", status) == 1) {
                if (strcmp(status, "R") == 0) {
                    char username[256];
                    fscanf(status_file, "%*s %*s %*s %*s %*s %*s %*s %s", username);

                    char cmdline[512];
                    if (snprintf(proc_path, sizeof(proc_path), "/proc/%s/cmdline", entry->d_name) >= sizeof(proc_path)) {
                        fprintf(stderr, "Erro: Caminho do processo incompleto!\n");
                        exit(EXIT_FAILURE);
                    }
                    FILE *cmdline_file = fopen(proc_path, "r");
                    if (cmdline_file == NULL) {
                        perror("fopen");
                        exit(EXIT_FAILURE);
                    }
                    fscanf(cmdline_file, "%s", cmdline);
                    fclose(cmdline_file);

                    //Imprime as informações detalhadas do processo em execução
                    printf("%-8s %-8s %-20s %s\n", entry->d_name, status, username, cmdline);

                    count_processes++;
                    break;
                }
            }
            fclose(status_file);
        }
    }

    closedir(proc_dir_running);
}

void executeCommandTop() {
    char input;

    do {
        //Chama as informações sobre o sistema e os processos
        printTopInfo();

        //Espera 10 segundos antes de atualizar as informações
        sleep(10);

        //Limpa a consola
        system("clear");

        //Imprime a situacao do comando
        printf("Insira 'q' para terminar ou outra tecla para continuar: ");
        //Lê o char introduzido pelo utilizador
        scanf("%c", &input);
    } while (input != 'q');
}

void executeCommand(char *cmd, char *args[]) {
    pid_t pid = fork();

    if (pid == 0) {
        //Código do processo filho

        //Executa o comando
        execvp(cmd, args);

        //Se o exec falhar, imprime uma mensagem de erro
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        //Código do processo pai
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "O comando '%s' falhou com o código de saída %d\n", cmd, WEXITSTATUS(status));
            exit(EXIT_FAILURE);
        }
    } else {
        //Se o fork falhar, imprime uma mensagem de erro
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void executeCommandWithInputRedirection(char *cmd, char **args, char *inputFile) {
    pid_t pid = fork();

    if (pid == 0) {
        //Código do processo filho
        
        //Abre o ficheiro no modo "read"
        int fd = open(inputFile, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        //Redireciona a entrada padrao para o arquivo
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(fd);
            exit(EXIT_FAILURE);
        }

        //Fecha o descritor do arquivo não necessario apos a duplicacao
        close(fd);

        //Executa o comando
        execvp(cmd, args);

        //Se o exec falhar, imprime uma mensagem de erro
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        //Código do processo pai
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "O comando '%s' falhou com o código de saída %d\n", cmd, WEXITSTATUS(status));
            exit(EXIT_FAILURE);
        }
    } else {
        //Se o fork falhar, imprime uma mensagem de erro
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void executeCommandWithOutputRedirection(char *cmd, char **args, char *outputFile) {
    pid_t pid = fork();

    if (pid == 0) {
        //Código do processo filho
        
        //Abre o ficheiro no modo "write"
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        //Redireciona a saida padrao para o arquivo
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            exit(EXIT_FAILURE);
        }

        //Fecha o descritor do arquivo não necessario apos a duplicacao
        close(fd);

        //Executa o comando
        execvp(cmd, args);

        //Se o exec falhar, imprime uma mensagem de erro
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        //Código do processo pai
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "O comando '%s' falhou com o código de saída %d\n", cmd, WEXITSTATUS(status));
            exit(EXIT_FAILURE);
        }
    } else {
        //Se o fork falhar, imprime uma mensagem de erro
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void executeCommandWithPiping(char *cmd1, char **args1, char *cmd2, char **args2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        //Se o pipe falhar, imprime uma mensagem de erro
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 == 0) {
        //Código do processo filho do comando 1

        //Fecha o "read" desnecessario no fim do "pipe" 
        close(pipefd[0]); 
        //Redireciona o "stdout" para o "write" no fim do "pipe"
        dup2(pipefd[1], STDOUT_FILENO); 
        //Fecha o "write" no fim do "pipe" 
        close(pipefd[1]); 

        //Executa o comando 1
        execvp(cmd1, args1);

        //Se o exec do comando 1 falhar, imprime uma mensagem de erro
        perror("execvp cmd1");
        exit(EXIT_FAILURE);
    } else if (pid1 < 0) {
        //Se o fork do comando 1 falhar, imprime uma mensagem de erro
        perror("fork cmd1");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        //Código do processo filho do comando 2
        //Fecha o "write" desnecessario no fim do "pipe"  
        close(pipefd[1]);  
        //Redireciona o "stdin" para o "read" no fim do "pipe"
        dup2(pipefd[0], STDIN_FILENO); 
         //Fecha o "read" no fim do "pipe"   
        close(pipefd[0]); 

        //Executa o comando 2
        execvp(cmd2, args2);

        //Se o exec do comando 2 falhar, imprime uma mensagem de erro
        perror("execvp cmd2");
        exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
        //Se o fork do comando 2 falhar, imprime uma mensagem de erro
        perror("fork cmd2");
        exit(EXIT_FAILURE);
    }

    //Código do processo pai
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

int main(int argc, char *argv[]) {
    int args = argc - 1;

    //Verifica se não existe argumentos além do comando
    if (args == 1 && strcmp(argv[1], "top") == 0) {
        executeCommandTop();
        return 0;
    }

    //Variáveis relativas aos nomes dos arquivos
    char *inputFile = NULL;
    char *outputFile = NULL;

    //Inicializa as flags
    int found = 0;
    char *operator = NULL;
    int argc_cmd1 = 0;
    int argc_cmd2 = 0;

    //Aloca memória para os arrays de argumentos do comando
    char **argv_cmd1 = malloc((argc + 1) * sizeof(char *));
    char **argv_cmd2 = malloc((argc + 1) * sizeof(char *));

    //Verifica se a alocação de memória foi bem-sucedida
    if (argv_cmd1 == NULL || argv_cmd2 == NULL) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        exit(EXIT_FAILURE);
    }

    //Loop para percorrer os argumentos
    for (int i = 1; i < argc; i++) {
        //Verifica se o argumento é um operador de redirecionamento ou um pipe
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "|") == 0) {
            found = 1;
            operator = argv[i];
            continue;
        }

        //Se não foi encontrado nenhum operador, adiciona ao contexto do cmd1
        if (!found) {
            argv_cmd1[argc_cmd1++] = strdup(argv[i]);
        } else {
            //Se um operador foi encontrado, adiciona ao contexto do cmd2
            argv_cmd2[argc_cmd2++] = strdup(argv[i]);
        }
    }

    //Adiciona NULL no final dos arrays para indicar o fim
    argv_cmd1[argc_cmd1] = NULL;
    argv_cmd2[argc_cmd2] = NULL;

    //Verifica se um operador foi encontrado
    if (found) {
        if (strcmp(operator, "<") == 0) {
            //Operador Input
            if (argc_cmd2 > 0) {
                inputFile = argv_cmd2[0];
            }
            executeCommandWithInputRedirection(argv_cmd1[0], argv_cmd1, inputFile);
        } else if (strcmp(operator, ">") == 0) {
            //Operador Output
            if (argc_cmd2 > 0) {
                outputFile = argv_cmd2[0];
            }
            executeCommandWithOutputRedirection(argv_cmd1[0], argv_cmd1, outputFile);
        } else if (strcmp(operator, "|") == 0) {
            //Operador Pipe
            executeCommandWithPiping(argv_cmd1[0], argv_cmd1, argv_cmd2[0], argv_cmd2);
        }
    } else {
        //Nao foi encontrado nenhum operador logo, executa o comando de forma normal
        executeCommand(argv_cmd1[0], argv_cmd1);
    }

    //Libera a memória alocada anteriormente para evitar "memory leaks"
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