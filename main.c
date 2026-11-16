#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int shell_cd(char** args);
int shell_help(char** args);
int shell_exit(char** args);

// List of builtin commands, followed by their corresponding functions.
char* builtin_str[] = {
    "cd",
    "help",
    "exit"
};

// Function pointers to the different builtin functions
int (*builtin_func[]) (char**) = {
    &shell_cd,
    &shell_help,
    &shell_exit
};

int shell_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char*);
}

int shell_cd(char** args)
{
    if(args[1] == NULL)
        fprintf(stderr, "shell: expected argument to \"cd\"\n");
    else
    {
        if (chdir(args[1]) != 0)
            perror("shell");
    }

    return 1;
}

int shell_help(char** args)
{
    printf("Camden Kirkpatrick's Shell\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (int i = 0; i < shell_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int shell_exit(char** args)
{
    return 0;
}



int shell_execute(char** args)
{
    if (args[0] == NULL)
        // An empty command was entered
        return 1;
    
    for (int i = 0; i < shell_num_builtins(); i++)
    {
        // See if the command entered is a builtin command
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
    }

    // Not a builtin command
    return shell_launch(args);
}

#define SHELL_RL_BUFSIZE 1024
char* shell_read_line(void)
{
    int bufSize = SHELL_RL_BUFSIZE;
    int position = 0;
    char* buffer = malloc(sizeof(char) * bufSize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Read a character
        c = getchar();

        // Add character to the buffer unless we hit EOF or newline
        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
            buffer[position] = c;
        position++;

        // If we have exceeded the buffer, reallocate
        if (position >= bufSize)
        {
            bufSize += SHELL_RL_BUFSIZE;
            buffer = realloc(buffer, bufSize);

            if (!buffer)
            {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}



#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"
char** shell_split_line(char* line)
{
    int bufSize = SHELL_TOK_BUFSIZE;
    int position = 0;
    char** tokens = malloc(bufSize * sizeof(char*));
    char* token;

    if (!tokens)
    {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Read from line until a delimeter is encountered
    token = strtok(line, SHELL_TOK_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        // Allocate more memory for tokens (like in shell_read_line)
        if (position >= bufSize)
        {
            bufSize += SHELL_TOK_BUFSIZE;
            tokens = realloc(tokens, bufSize * sizeof(char*));
            
            if (!tokens)
            {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        // Keep reading the line from where you left off
        token = strtok(NULL, SHELL_TOK_DELIM);
    }

    // bufSize may be larger than position (we grow in chunks of 64); unused slots are fine, we never read past the NULL
    tokens[position] = NULL;
    return tokens;
}



int shell_launch(char** args)
{
    pid_t pid, wpid;
    int status;

    int i = 0;
    char* arg = args[i];
    char* fileName = NULL;
    while (arg != NULL)
    {
        if (strcmp(arg, ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Expected file name after '>'\n");
                return 1;
            }

            fileName = args[i + 1];
            break;
        }
        arg = args[++i];
    }

    pid = fork();
    if (pid == 0)
    {
        if (fileName != NULL)
        {
            int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1)
                perror("Error opening file\n");

            if (dup2(fd, STDOUT_FILENO) < 0)
                perror("dup2");

            args[i] = NULL;

            close(fd);
        }

        // Child process
        if (execvp(args[0], args) == -1)
            perror("shell");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
        // fork() failed
        perror("shell");
    else
    {
        // Parent process
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        
    }

    return 1;
}



void shell_loop(void)
{
    char* line;
    char** args;
    int status;

    do
    {
        printf("> ");
        line = shell_read_line();
        args = shell_split_line(line);
        status = shell_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int arg, char* argv[])
{
    shell_loop();

    return EXIT_SUCCESS;
}