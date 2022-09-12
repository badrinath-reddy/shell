#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_ARGS 1000 // max number of arguments to a command

void add_path(char ***path, int *path_size, char *new_path);
void print_paths(char **path, int path_size);
void free_paths(char **path, int path_size);
char *fetch_command(char *line);
bool find_in_path(char **path, int path_size, char **command);
void clean_line(char **line);

int main(int argc, char *argv[])
{

    // Prompt variables
    char prompt[] = "dash>";

    // Error message
    char error_message[30] = "An error has occurred\n";

    // Batch mode variables
    bool is_batch = false;

    // Path variables
    char **paths = NULL;
    int path_size = 0;
    add_path(&paths, &path_size, "/bin");

    // Default input source
    FILE *file = stdin;

    // Incorrect number of arguments error
    if (argc > 2)
    {
        printf("Usage: ./dash [path_file]\n");
        return EXIT_FAILURE;
    }
    // Batch mode handling
    else if (argc == 2)
    {
        is_batch = true;
        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            printf("Error: File not found\n");
            return EXIT_FAILURE;
        }
    }

    // Main Loop
    while (1)
    {
        if (!is_batch)
        {
            printf("%s", prompt);
        }
        char *line = NULL;
        size_t len = 0;
        ssize_t read = getline(&line, &len, file);

        // check for EOF
        if (read == -1)
        {
            exit(0);
        }

        // clean line
        clean_line(&line);

        // Simple enter
        if (strcmp(line, "") == 0)
        {
            continue;
        }

        // Fetch command from line
        char *save_ptr = line;
        char *command = strtok_r(save_ptr, " ", &save_ptr);

        // Built-in commands
        // Exit command
        if (strcmp(command, "exit") == 0 || strcmp(command, "exit\0") == 0)
        {
            exit(0);
        }

        // Path command
        else if (strcmp(command, "path") == 0)
        {
            free_paths(paths, path_size);
            paths = NULL;
            path_size = 0;
            char *path = NULL;
            while ((path = strtok_r(save_ptr, " ", &save_ptr)))
            {
                add_path(&paths, &path_size, path);
            }
        }

        // Other commands
        else
        {
            // Check if command is in path
            bool found = false;
            found = find_in_path(paths, path_size, &command);

            // Command not found
            if (!found)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }

            // Command found
            // Fork and execute command
            else
            {
                int rc = fork();
                if (rc < 0)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (rc == 0)
                {
                    char *args[MAX_ARGS];
                    int i = 0;
                    args[i] = command;
                    i++;
                    while ((args[i] = strtok_r(save_ptr, " ", &save_ptr)))
                    {
                        i++;
                    }
                    args[i] = NULL;
                    execv(command, args);
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else
                {
                    wait(NULL);
                }
            }
        }
    }
}

// Add a path to the paths array
void add_path(char ***paths, int *path_size, char *new_path)
{
    *paths = (char **)realloc(*paths, (*path_size + 1) * sizeof(char *));
    (*paths)[*path_size] = (char *)malloc(strlen(new_path) * sizeof(char));
    strcpy((*paths)[*path_size], new_path);
    *path_size += 1;
}

// Print paths for debugging
void print_paths(char **path, int path_size)
{
    for (int i = 0; i < path_size; i++)
    {
        printf("\n%s", path[i]);
    }
}

// Free paths array
void free_paths(char **path, int path_size)
{
    for (int i = 0; i < path_size; i++)
    {
        free(path[i]);
    }
    free(path);
}

// Clean line by removing spaces and newlines from the end and beginning
void clean_line(char **line)
{
    int i = 0;
    while ((*line)[i] == ' ' || (*line)[i] == '\t')
    {
        i++;
    }
    *line = *line + i;
    i = strlen(*line) - 1;
    while ((*line)[i] == ' ' || (*line)[i] == '\t' || (*line)[i] == '\n')
    {
        (*line)[i] = '\0';
        i--;
    }
}

// Fetch command from line
char *fetch_command(char *line)
{
    char *command = NULL;
    char *save_ptr = line;
    command = strtok_r(save_ptr, " ", &save_ptr);
    return command;
}

// Check if command is in path
bool find_in_path(char **path, int path_size, char **command)
{
    char *full_path = NULL;
    for (int i = 0; i < path_size; i++)
    {
        full_path = (char *)malloc(strlen(path[i]) + strlen(*command) + 1);
        strcpy(full_path, path[i]);
        strcat(full_path, "/");
        strcat(full_path, *command);
        if (access(full_path, X_OK) != -1)
        {
            *command = full_path;
            return true;
        }
    }
    return false;
}