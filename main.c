#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
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



void delete_at_index(char** args, int index)
{
    for (int i = index; args[i] != NULL; i++)
    {
        args[i] = args[i + 1];
    }
}



int evaluate_mul_div(char** args)
{
    double result = 0;
    double curNum = 0;
    char curOp = '+';
    int i = 0;
    int finishedOp = 0;

    do
    {
        // Ex. "9 +"
        if (args[i] == NULL)
        {
            fprintf(stderr, "shell: incomplete expression\n");
            return 1;
        }

        char* endPtr;
        errno = 0; 

        curNum = strtod(args[i], &endPtr);
 
        if (endPtr == args[i])
        {
            fprintf(stderr, "shell: no numeric digits found, conversion failed\n");
            return 1;
        }
        else if (errno == ERANGE)
        {
            fprintf(stderr, "shell: value out of range for a double\n");
            return 1;
        }
        else if (*endPtr != '\0')
        {
            fprintf(stderr, "shell: partial conversion (%f), stopped at '%s'\n", curNum, endPtr);
            return 1;
        }
        else if (curOp == '/' && curNum == 0)
        {
            fprintf(stderr, "shell: division by zero is not allowed\n");
            return 1;
        }

        switch (curOp)
        {
            case '*':
                result *= curNum;
                finishedOp = 1;
                break;
            case '/':
                result /= curNum;
                finishedOp = 1;
                break;
            default:
                result = curNum;
                break;
        }


        // 0 1 2 3 4
        // 3 + 2 * 6       i = 4

        // delete_at_index(args, i (4))
        // 0 1 2 3
        // 3 + 2 *

        // delete_at_index(args, i - 1 (3))
        // 0 1 2
        // 3 + 2

        // args[i (4) - 2] = newToken
        // 0 1 2
        // 3 + newToken
        if (finishedOp)
        {
            char* newToken = malloc(64);
            snprintf(newToken, 64, "%g", result);

            delete_at_index(args, i);
            delete_at_index(args, i - 1);

            args[i - 2] = newToken;
            i -= 2;
        }

        finishedOp = 0;

        // No more operators, so we are done
        if (args[i + 1] == NULL)
            break;

        curOp = args[i + 1][0];
        // Ex. "25 5"
        if (curOp != '+' && curOp != '-' &&
            curOp != '*' && curOp != '/')
        {
            fprintf(stderr, "shell: expected an operator, found '%c'\n", curOp);
            return 1;
        }

        i += 2;
    } while (1);
    return 0;
}



int evaluate_add_sub(char** args, double* answer)
{
    double curNum = 0;
    char curOp = '+';
    int i = 0;

    do
    {
        // Ex. "9 +"
        if (args[i] == NULL)
        {
            fprintf(stderr, "shell: incomplete expression\n");
            return 1;
        }

        char* endPtr;
        errno = 0; 

        curNum = strtod(args[i], &endPtr);

        if (endPtr == args[i])
        {
            fprintf(stderr, "shell: no numeric digits found, conversion failed\n");
            return 1;
        }
        else if (errno == ERANGE)
        {
            fprintf(stderr, "shell: value out of range for a double\n");
            return 1;
        }
        else if (*endPtr != '\0')
        {
            fprintf(stderr, "shell: partial conversion (%f), stopped at '%s'\n", curNum, endPtr);
            return 1;
        }
        else if (curOp == '/' && curNum == 0)
        {
            fprintf(stderr, "shell: division by zero is not allowed\n");
            *answer = 0;
            return 1;
        }

        switch (curOp)
        {
            case '+':
                *answer += curNum;
                break;
            case '-':
                *answer -= curNum;
                break;
            case '*':
                *answer *= curNum;
                break;
            case '/':
                *answer /= curNum;
                break;
            default:
                break;
        }

        // No more operators, so we are done
        if (args[i + 1] == NULL)
            break;

        curOp = args[i + 1][0];
        // Ex. "25 5"
        if (curOp != '+' && curOp != 
            '-' && curOp != '*' && curOp != '/')
        {
            fprintf(stderr, "shell: expected an operator, found '%c'\n", curOp);
            return 1;
        }

        i += 2;
    } while (1);
    return 0;
}



int parse_arithmetic_expression(char** args, double* answer)
{
    // Multiply and divide before addition and subtraction
    if (evaluate_mul_div(args) != 0)
        return 1;

    for (int i = 0; args[i] != NULL; i++)
    {
        printf("%s\n", args[i]);
    }
    
    return evaluate_add_sub(args, answer);
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

    char* endPtr;
    errno = 0; 

    double value = strtod(args[0], &endPtr);
    double answer = 0;

    if (errno == ERANGE)
    {
        fprintf(stderr, "shell: value out of range for a double\n");
        return 1;
    }
    else if (*endPtr == '\0')
    {
        if(parse_arithmetic_expression(args, &answer) == 0)
            printf("= %.3f\n", answer);
        return 1;
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



// Allow the user to use <, >, and >>
int handle_redirection(char** args, char** inputFileName, char** outputFileName, char* outputOp)
{
    int i = 0;
    char* arg = args[i];
    int firstOpIdx = -1; // first index we see <, >, or >>

    while (arg != NULL)
    {
        if (strcmp(arg, ">") == 0 || strcmp(arg, ">>") == 0 || strcmp(arg, "<") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Expected file name after redirection operator\n");
                return 1;
            }
            else if (strcmp(arg, ">") == 0)
            {
                *outputOp = 't';
                *outputFileName = args[i + 1];
            }
            else if (strcmp(arg, ">>") == 0)
            {
                *outputOp = 'a';
                *outputFileName = args[i + 1];
            }
            else // arg == '<'
                *inputFileName = args[i + 1];

            if (firstOpIdx == -1)
                firstOpIdx = i;
        }
        arg = args[++i];
    }

    // Strip redirection operators/filenames from args so execvp only sees the command and its real arguments
    if (firstOpIdx != -1)
        args[firstOpIdx] = NULL;

    return 0;
}



// Check to see if the user is using I/O redirection 
void apply_redirection(char* fileName, char outputOp)
{
    // No <, >, or >> were found in args
    if (fileName == NULL)
        return;

    int fd;
    int flags;

    if (outputOp != '\0')
    {
        flags = O_WRONLY | O_CREAT;
        flags |= (outputOp == 't') ? O_TRUNC : O_APPEND;
    }
    else
        flags = O_RDONLY;

    // Get the file descriptor from the file
    fd = open(fileName, flags, 0644);

    if (fd == -1)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Write/read to the file instead of stdout/stdin
    if (dup2(fd, (outputOp != '\0') ? STDOUT_FILENO : STDIN_FILENO) < 0)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
}



int shell_launch(char** args)
{
    pid_t pid, wpid;
    int status;

    char* inputFileName = NULL;
    char* outputFileName = NULL;
    char outputOp; // t = truncate, a = append

    if (handle_redirection(args, &inputFileName, &outputFileName, &outputOp) == 1)
        return 1;

    pid = fork();
    // Child process
    if (pid == 0)
    {
        apply_redirection(outputFileName, outputOp);
        apply_redirection(inputFileName, '\0');

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
    int status; // 1 = continue, 0 = stop

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



int main(int arg, char** argv)
{
    shell_loop();

    return EXIT_SUCCESS;
}