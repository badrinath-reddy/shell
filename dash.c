#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 1000 // max number of arguments to a command

void add_path(char ***path, int *path_size, char *new_path);
void print_paths(char **path, int path_size);
void free_paths(char **path, int path_size);
char *fetch_command(char *line);
bool find_in_path(char **path, int path_size, char **command);
void clean_line(char **line);
FILE *get_file(char *file_name);
void print_error(void);

int main(int argc, char *argv[])
{

    // Prompt variables
    char prompt[] = "dash>";

    // Batch mode variables
    bool is_batch = false;

    // Path variables
    char **paths = NULL;
    int path_size = 0;
    add_path(&paths, &path_size, "/bin");

    // Default input source
    FILE *input_file = stdin;

    // Incorrect number of arguments error
    if (argc > 2)
    {
        print_error();
        exit(1);
    }
    // Batch mode handling
    else if (argc == 2)
    {
        is_batch = true;
        input_file = fopen(argv[1], "r");
        if (input_file == NULL)
        {
            print_error();
            exit(1);
        }
    }

    // Main Loop
    while (1)
    {
        if (!is_batch)
        {
            printf("%s", prompt);
        }
        char *input = NULL;
        size_t len = 0;
        ssize_t read = getline(&input, &len, input_file);

        // check for EOF
        if (read == -1 && is_batch)
        {
            exit(0);
        }

        // clean line
        clean_line(&input);

        char *line = NULL;
        char *save_ptr_input = input;

        if (strcmp(input, "&") == 0)
        {
            print_error();
            continue;
        }

        int num_commands = 0;
        while ((line = strtok_r(save_ptr_input, "&", &save_ptr_input)))
        {
            num_commands++;

            // clean line
            clean_line(&line);

            // Simple enter
            if (strcmp(line, "") == 0)
            {
                continue;
            }

            char *un_modified_command = NULL;

            un_modified_command = strdup(line);

            // Fetch command from line
            char *save_ptr = line;
            char *command = strtok_r(save_ptr, " ", &save_ptr);

            // Built-in commands
            // Exit command
            if (strcmp(command, "exit") == 0 || strcmp(command, "exit\0") == 0)
            {
                bool has_args = false;
                while (strtok_r(save_ptr, " ", &save_ptr) != NULL)
                {
                    has_args = true;
                    break;
                }
                if (has_args)
                {
                    print_error();
                    continue;
                }
                exit(0);
            }

            // CD command
            else if (strcmp(command, "cd") == 0)
            {
                char *cdpath = NULL;
                int countArgs = 0;
                char *destinationPath = NULL;
                while ((cdpath = strtok_r(save_ptr, " ", &save_ptr)))
                {
                    countArgs++;
                    destinationPath = cdpath;
                }

                // check if no arguments are present or more than one argument is present
                // no error found, verify if directory exists and change directory
                if (countArgs == 0 || countArgs > 1 || chdir(destinationPath) == -1)
                {
                    print_error();
                }
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
                int x = fork();
                if (x < 0)
                {
                    print_error();
                }
                else if (x == 0)
                {
                    // process command
                    int i = 0;
                    save_ptr = un_modified_command;
                    char *out_file_name = NULL;
                    char *part = NULL;
                    bool is_redirect = false;

                    if (strstr(save_ptr, ">"))
                    {
                        is_redirect = true;
                    }

                    while ((part = strtok_r(save_ptr, ">", &save_ptr)))
                    {
                        // Extra params after redirect
                        if (i > 1)
                        {
                            print_error();
                            exit(1);
                        }

                        if (i == 0)
                        {
                            command = part;
                        }

                        // Redirect
                        else if (i == 1)
                        {
                            // verify if multiple files are passed with space in the
                            // redirection output and throw error

                            int count_files = 0;
                            char *mini_part = NULL;
                            while ((mini_part = strtok_r(part, " ", &part)))
                            {
                                if (count_files > 0 && (mini_part != NULL))
                                {
                                    print_error();
                                    exit(1);
                                }

                                if (count_files == 0)
                                {
                                    out_file_name = mini_part;
                                }
                                count_files++;
                            }
                        }

                        i++;
                    }

                    // redirect with no output file
                    if (is_redirect && out_file_name == NULL)
                    {
                        print_error();
                        exit(1);
                    }

                    // clean line
                    if (command != NULL)
                    {
                        clean_line(&command);
                    }
                    if (out_file_name != NULL)
                    {
                        clean_line(&out_file_name);
                    }

                    save_ptr = command;
                    char *args[MAX_ARGS];
                    int j = 0;

                    while ((part = strtok_r(save_ptr, " ", &save_ptr)))
                    {
                        args[j] = part;
                        j++;
                    }

                    args[j] = NULL;

                    // Check if command is in path
                    bool found = false;
                    found = find_in_path(paths, path_size, &args[0]);

                    // Command not found
                    if (!found)
                    {
                        print_error();
                        exit(1);
                    }

                    // Command found
                    else
                    {
                        // Redirect
                        if (i > 1)
                        {
                            FILE *out_file = get_file(out_file_name);
                            if (out_file == NULL)
                            {
                                print_error();
                                exit(1);
                            }
                            dup2(fileno(out_file), STDOUT_FILENO);
                            dup2(fileno(out_file), STDERR_FILENO);
                        }
                        execv(args[0], args);
                        print_error();
                        exit(1);
                    }
                }
            }
        }

        // Wait for all commands to finish
        int i;
        for (i = 0; i < num_commands; i++)
        {
            int status;
            int k = wait(&status);
            if (k != -1 && status != 0 && is_batch)
            {
                exit(1);
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
    int i;
    for (i = 0; i < path_size; i++)
    {
        printf("\n%s", path[i]);
    }
}

// Free paths array
void free_paths(char **path, int path_size)
{
    int i;
    for (i = 0; i < path_size; i++)
    {
        free(path[i]);
    }
    free(path);
}

// Clean line by removing spaces and newlines from the end and beginning
void clean_line(char **line)
{
    int i = strlen(*line);
    if (i == 0)
    {
        return;
    }

    i = 0;
    while ((*line)[i] == ' ' || (*line)[i] == '\t')
    {
        i++;
    }
    *line = *line + i;
    i = strlen(*line) - 1;
    while ((*line)[i] == ' ' || (*line)[i] == '\t' || (*line)[i] == '\n' || (*line)[i] == EOF)
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
    int i;
    for (i = 0; i < path_size; i++)
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

// get file from string if not found create it
FILE *get_file(char *file_name)
{
    FILE *file = fopen(file_name, "w");
    return file;
}

void print_error()
{
    // Error message
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
}