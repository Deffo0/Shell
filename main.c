#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <string.h>


FILE *log_file;

// helping functions
char *remove_quotes(char *string) {
    if (string[0] == '"' && string[strlen(string) - 1] == '"') {
        string[strlen(string) - 1] = '\0';
        return string + 1;
    }
    return string;
}

char *split(char *string) {
    while (string[0] == ' ') {
        string = string + 1;
    }
    while (string[strlen(string) - 1] == ' ') {
        string[strlen(string) - 1] = '\0';
    };

    return string;
}

char *replace(char *original, char *replaced, char *replacement) {

    char *result, *ptr, *tmp;
    int len_replaced, len_replacement, len_front, count;

    len_replaced = strlen(replaced);

    if (!replacement)
        replacement = "";
    len_replacement = strlen(replacement);

    // count the number of replacements needed
    ptr = original;
    tmp = strstr(ptr, replaced);
    for (count = 0; tmp; ++count) {
        ptr = tmp + len_replaced;
        tmp = strstr(ptr, replaced);
    }

    tmp = result = malloc(strlen(original) + (len_replacement - len_replaced) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ptr = strstr(original, replaced);
        len_front = ptr - original;
        tmp = strncpy(tmp, original, len_front) + len_front;
        tmp = strcpy(tmp, replacement) + len_replacement;
        original += len_front + len_replaced;
    }
    strcpy(tmp, original);
    return result;
}

void setup_environment() {
    char cwd[1000];
    getcwd(cwd, sizeof(cwd));
    chdir(cwd);
}

void *parse_input(char input[], char *parameters[500]) {
    int parameter_counter = 0;

    //apply the effect of export to the expression
    for (int i = 1; i < strlen(input); i++) {
        if (strchr(input, '$') != NULL) {
            input = replace(input, strchr(input, '$'),
                            getenv(strchr(input, '$') + 1));
        }
    }

    //extract the command and thr parameters from the input
    parameters[0] = strsep(&input, " ");
    parameter_counter++;
    if ((strcmp(parameters[0], "export") == 0 && strchr(input, '"') != NULL && input[strlen(input) - 1] == '"') ||
        strcmp(parameters[0], "echo") == 0) {
        parameters[1] = input;
        parameter_counter++;
    } else {
        while (input != NULL) {
            parameters[parameter_counter] = strsep(&input, " ");
            parameter_counter++;
        }
    }


}

void export(char string[]) {
    char *key;
    char *data;
    key = strsep(&string, "=");
    data = strdup(string);

    // save the var in the environment variables
    setenv(key, remove_quotes(data), 1);
}

void execute_shell_bultin(char *parameters[500]) {


    if (strcmp(parameters[0], "cd") == 0) {
        chdir(parameters[1]);
    } else if (strcmp(parameters[0], "echo") == 0) {
        printf("%s\n", remove_quotes(parameters[1]));
    } else if (strcmp(parameters[0], "export") == 0) {
        export(parameters[1]);
    }


}


void execute_command(char *parameters[500], int background) {
    int status;
    pid_t child_id = fork();
// error in forking
    if (child_id < 0) {
        printf("Forking child process failed\n");
    }
// child case
    else if (child_id == 0) {

        if (execvp(*parameters, parameters) < 0) {
            printf("Error\n");
            exit(1);
        }
// parent case
    } else {
        if (background) {
            waitpid(child_id, &status, WNOHANG);

        } else {
            waitpid(child_id, &status, WEXITSTATUS(status));

        }

    }
}

void shell() {
    int command_is_not_exit = 1;
    do {
// initialize variables
        char *parameters[500];
// manual free the pointers for the parameters 2d array of char
        for (int i = 0; parameters[i] != NULL; i++) {
            parameters[i] = NULL;
        }
        int command_type = 0, background = 0;
        char input[600];
        char directory[1000];

        getcwd(directory, 1000);
        printf("\033[1;36m");
        printf(":%s >> ", directory);
        printf("\033[0m");

// get the input and remove the space char from the left most and most right
        fgets(input, 600, stdin);
        input[strlen(input) - 1] = '\0';

        strcpy(input, split(input));
// check for background and set its flag
        if (input[strlen(input) - 1] == '&') {
            background = 1;
            input[strlen(input) - 1] = '\0';

        }

// parse the input to parameters ex(command, argument 1, argument 2,...etc)
        parse_input(split(input), parameters);

// specify th type of the command whether it is built in or executive command
        if (strcmp(parameters[0], "cd") == 0 || strcmp(parameters[0], "echo") == 0 ||
            strcmp(parameters[0], "export") == 0) {
            command_type = 1;
        } else if (strcmp(parameters[0], "exit") == 0) {
            command_is_not_exit = 0;
        } else {
            command_type = 2;
        }

// execute command based on its type
        if (command_type == 1) {
            execute_shell_bultin(parameters);
        } else if (command_type == 2) {
            execute_command(parameters, background);

        }

    } while (command_is_not_exit);

}

void on_child_exit() {
    int wstat;
    pid_t pid;

// reaping zombies
    while (1) {
        pid = wait3(&wstat, WNOHANG, (struct rusage *) NULL);
        if (pid == 0)
            break;
        else if (pid == -1)
            break;

        // Reap Zombie
        else{}

    }

// append child's termination to the log file
    log_file = fopen("log_file.txt", "a+");
    fprintf(log_file, "Child process was terminated\n");
    fclose(log_file);


}


int main() {

    fclose(fopen("log_file.txt", "w"));
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();


}