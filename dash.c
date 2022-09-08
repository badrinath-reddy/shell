#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

void add_path(char ***path, int *path_size, char *new_path);
void print_paths(char **path, int path_size);
void free_paths(char **path, int path_size);
char *fetch_command(char *line);

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

    FILE *file = stdin;

    if (argc > 2)
    {
        printf("Usage: ./dash [path_file]\n");
        return EXIT_FAILURE;
    }
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

        char *save_ptr = line;
        char *command = strtok_r(save_ptr, " ", &save_ptr);

        // Simple enter
        if (strcmp(command, "\n") == 0)
        {
            continue;
        }

        // Exit command
        if (strcmp(command, "exit\n") == 0 || strcmp(command, "exit\0") == 0)
        {
            exit(0);
        }

        // Path command
        else if (strcmp(command, "path") == 0 || strcmp(command, "path\n") == 0)
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

        else
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    return 0;
}

void add_path(char ***paths, int *path_size, char *new_path)
{
    *paths = (char **)realloc(*paths, (*path_size + 1) * sizeof(char *));
    (*paths)[*path_size] = (char *)malloc(strlen(new_path) * sizeof(char));
    strcpy((*paths)[*path_size], new_path);
    *path_size += 1;
}

void print_paths(char **path, int path_size)
{
    for (int i = 0; i < path_size; i++)
    {
        printf("\n%s", path[i]);
    }
}

void free_paths(char **path, int path_size)
{
    for (int i = 0; i < path_size; i++)
    {
        free(path[i]);
    }
    free(path);
}

char *fetch_command(char *line)
{
    char *command = NULL;
    char *save_ptr = line;
    command = strtok_r(save_ptr, " ", &save_ptr);
    return command;
}