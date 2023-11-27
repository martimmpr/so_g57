#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

typedef struct {
    int found;
    const char *operator;
    int argc_cmd1;
    char **argv_cmd1;
    int argc_cmd2;
    char **argv_cmd2;
} commands;

// Função para analisar os argumentos da linha de comando
commands parse(int argc, char *argv[]) {
    commands result;

    // Inicia a estrutura result
    result.found = 0;
    result.operator = NULL;
    result.argc_cmd1 = 0;
    result.argc_cmd2 = 0;

    // Aloca memória para os arrays de argumentos do comando
    result.argv_cmd1 = malloc((argc + 1) * sizeof(char *));
    result.argv_cmd2 = malloc((argc + 1) * sizeof(char *));

    // Verifica se a alocação de memória foi bem-sucedida
    if (result.argv_cmd1 == NULL || result.argv_cmd2 == NULL) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        exit(EXIT_FAILURE);
    }

    // Inicializa a flag "found" como 0
    int found = 0;

    // Loop para percorrer os argumentos
    for (int i = 0; i < argc; i++) {
        // Verifica se o argumento é um operador de redirecionamento ou um pipe
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "|") == 0) {
            found = 1;
            result.found = 1;
            result.operator = argv[i];
            continue;
        }

        // Se não foi encontrado nenhum operador, adiciona ao contexto do cmd1
        if (!found) {
            result.argv_cmd1[result.argc_cmd1++] = strdup(argv[i]);
        } else {
            // Se um operador foi encontrado, adiciona ao contexto do cmd2
            result.argv_cmd2[result.argc_cmd2++] = strdup(argv[i]);
        }
    }

    // Adiciona NULL no final dos arrays para indicar o fim
    result.argv_cmd1[result.argc_cmd1] = NULL;
    result.argv_cmd2[result.argc_cmd2] = NULL;

    return result;
}

// Função para executar comandos Linux, possivelmente com redirecionamento
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
    // Verifica se não existe argumentos alem do comando
    if (argc == 1) {
        executeCommand("top", (char *[]){"top", "-b", "-n", "1", NULL});
        return 0;
    }

    // Variaveis relativas aos nomes dos ficheiros 
    char *inputFile = NULL;
    char *outputFile = NULL;

    commands parsed = parse(argc - 1, argv + 1);

    // Verifica se um operador foi encontrado
    if (parsed.found) {
        // Se o cmd2 tiver mais de 1 argumento, assume-se que o primeiro é o arquivo de entrada e o segundo é o de saída
        if (parsed.argc_cmd2 > 1) {
            inputFile = parsed.argv_cmd2[0];
            outputFile = parsed.argv_cmd2[1];
        } else if (parsed.argc_cmd2 > 0) {
            // Se o cmd2 tiver apenas 1 argumento, assume-se que é o arquivo de entrada
            inputFile = parsed.argv_cmd2[0];
        }
    }

    // Executa o comando cmd1, possivelmente com redirecionamento
    executeCommand(parsed.argv_cmd1[0], parsed.argv_cmd1);

    // Liberta a memória alocada anterioremente para evitar "memory leaks"
    for (int i = 0; i < parsed.argc_cmd1; i++) {
        free(parsed.argv_cmd1[i]);
    }
    for (int i = 0; i < parsed.argc_cmd2; i++) {
        free(parsed.argv_cmd2[i]);
    }
    free(parsed.argv_cmd1);
    free(parsed.argv_cmd2);

    return 0;
}