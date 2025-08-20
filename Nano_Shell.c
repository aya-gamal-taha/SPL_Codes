#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_VARS 100
#define MAX_INPUT 1024

typedef struct {
    char *name;
    char *value;
    int exported;
} Variable;

Variable variables[MAX_VARS];
int var_count = 0;

// Find a variable by name
Variable* find_variable(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }
    return NULL;
}

// Set or update a variable
void set_variable(const char *name, const char *value, int exported) {
    Variable *var = find_variable(name);
    if (var != NULL) {
        free(var->value);
        var->value = strdup(value);
        var->exported = exported;
    } else {
        if (var_count < MAX_VARS) {
            variables[var_count].name = strdup(name);
            variables[var_count].value = strdup(value);
            variables[var_count].exported = exported;
            var_count++;
        }
    }
}

// Free all variables
void free_variables() {
    for (int i = 0; i < var_count; i++) {
        free(variables[i].name);
        free(variables[i].value);
    }
    var_count = 0;
}

// Check if a string is a valid variable name
int is_valid_var_name(const char *name) {
    if (name == NULL || *name == '\0') return 0;
    if (!((*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z') || *name == '_')) return 0;
    
    for (int i = 1; name[i] != '\0'; i++) {
        if (!((name[i] >= 'a' && name[i] <= 'z') || 
              (name[i] >= 'A' && name[i] <= 'Z') || 
              (name[i] >= '0' && name[i] <= '9') || 
              name[i] == '_')) {
            return 0;
        }
    }
    return 1;
}

// Expand variables in a string
char* expand_variables(const char *input) {
    char *result = malloc(MAX_INPUT);
    result[0] = '\0';
    const char *ptr = input;
    
    while (*ptr != '\0') {
        if (*ptr == '$') {
            ptr++; // skip '$'
            
            // Extract variable name
            char var_name[100];
            int i = 0;
            while ((*ptr >= 'a' && *ptr <= 'z') || 
                   (*ptr >= 'A' && *ptr <= 'Z') || 
                   (*ptr >= '0' && *ptr <= '9') || 
                   *ptr == '_') {
                var_name[i++] = *ptr++;
            }
            var_name[i] = '\0';
            
            // Look up variable
            Variable *var = find_variable(var_name);
            if (var != NULL) {
                strcat(result, var->value);
            }
            // If variable not found, leave it empty (as per requirement)
        } else {
            // Copy regular character
            char temp[2] = {*ptr, '\0'};
            strcat(result, temp);
            ptr++;
        }
    }
    
    return result;
}

// Check if input is a valid variable assignment
int is_valid_assignment(const char *input) {
    // Check for spaces around '=' or other characters
    const char *equal = strchr(input, '=');
    if (equal == NULL) return 0;
    
    // Check if there's only one '='
    if (strchr(equal + 1, '=') != NULL) return 0;
    
    // Extract variable name
    int name_len = equal - input;
    if (name_len == 0) return 0;
    
    char var_name[100];
    strncpy(var_name, input, name_len);
    var_name[name_len] = '\0';
    
    // Remove trailing spaces from variable name
    while (name_len > 0 && (var_name[name_len-1] == ' ' || var_name[name_len-1] == '\t')) {
        var_name[--name_len] = '\0';
    }
    
    return is_valid_var_name(var_name);
}

// Parse assignment statement
void parse_assignment(const char *input, char **name, char **value) {
    const char *equal = strchr(input, '=');
    int name_len = equal - input;
    
    *name = malloc(name_len + 1);
    strncpy(*name, input, name_len);
    (*name)[name_len] = '\0';
    
    // Remove trailing spaces from name
    int len = strlen(*name);
    while (len > 0 && ((*name)[len-1] == ' ' || (*name)[len-1] == '\t')) {
        (*name)[--len] = '\0';
    }
    
    *value = strdup(equal + 1);
    
    // Remove leading spaces from value
    while (**value == ' ' || **value == '\t') {
        (*value)++;
    }
}

// Execute export command
int execute_export(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "export: missing variable name\n");
        return 1;
    }
    
    Variable *var = find_variable(args[1]);
    if (var == NULL) {
        fprintf(stderr, "export: %s: variable not found\n", args[1]);
        return 1;
    }
    
    var->exported = 1;
    setenv(var->name, var->value, 1);
    return 0;
}

// Setup environment for child process
void setup_environment() {
    for (int i = 0; i < var_count; i++) {
        if (variables[i].exported) {
            setenv(variables[i].name, variables[i].value, 1);
        }
    }
}

int nanoshell_main(int argc, char *argv[]) {
    char input[MAX_INPUT];

    while (1) {
        printf("Nano Shell Prompt > ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) {
            continue;
        }

        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Check if this is a variable assignment
        if (strchr(input, '=') != NULL) {
            if (!is_valid_assignment(input)) {
                printf("Invalid command\n");
                continue;
            }
            
            char *var_name, *var_value;
            parse_assignment(input, &var_name, &var_value);
            set_variable(var_name, var_value, 0);
            free(var_name);
            free(var_value);
            continue;
        }

        // Parse command with redirection and variable expansion
        char *args[100];
        char *infile = NULL;
        char *outfile = NULL;
        char *errfile = NULL;
        int arg_count = 0;
        int parsing_error = 0;

        char input_copy[MAX_INPUT];
        strcpy(input_copy, input);
        
        char *token = strtok(input_copy, " ");
        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    fprintf(stderr, "syntax error: expected filename after <\n");
                    parsing_error = 1;
                    break;
                }
                infile = token;
            } else if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    fprintf(stderr, "syntax error: expected filename after >\n");
                    parsing_error = 1;
                    break;
                }
                outfile = token;
            } else if (strcmp(token, "2>") == 0) {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    fprintf(stderr, "syntax error: expected filename after 2>\n");
                    parsing_error = 1;
                    break;
                }
                errfile = token;
            } else if (strcmp(token, "export") == 0) {
                // Handle export command
                char *export_args[10];
                int export_argc = 0;
                export_args[export_argc++] = token;
                
                while ((token = strtok(NULL, " ")) != NULL) {
                    export_args[export_argc++] = token;
                }
                export_args[export_argc] = NULL;
                
                execute_export(export_args);
                parsing_error = 1; // Skip fork/exec
                break;
            } else {
                // Expand variables in arguments
                char *expanded = expand_variables(token);
                args[arg_count++] = expanded;
                token = strtok(NULL, " ");
            }
        }
        args[arg_count] = NULL;

        if (parsing_error) {
            continue;
        }
        if (arg_count == 0) {
            continue;
        }

        // Fork and execute command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            setup_environment();
            
            if (infile != NULL) {
                int fd = open(infile, O_RDONLY);
                if (fd < 0) {
                    fprintf(stderr, "cannot access %s: %s\n", infile, strerror(errno));
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (outfile != NULL) {
                int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    fprintf(stderr, "%s: %s\n", outfile, strerror(errno));
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            if (errfile != NULL) {
                int fd = open(errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    fprintf(stderr, "%s: %s\n", errfile, strerror(errno));
                    exit(1);
                }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            execvp(args[0], args);
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(127);
            
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            fprintf(stderr, "fork failed: %s\n", strerror(errno));
        }

        // Free expanded arguments
        for (int i = 0; i < arg_count; i++) {
            free(args[i]);
        }
    }

    free_variables();
    return 0;
}
