// Eric Cordts and Jonathan Hsin
// NUID: 001431954 (Eric)
// NUID: (Jonathan, TODO)
// EECE7376: Operating Systems: Interface and Implementation
// Course Project Problem 1

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

// These are constants defined from HW2
#define MAX_SUB_COMMANDS    5
#define MAX_ARGS            10

struct SubCommand
{
    char *line;
    char *argv[MAX_ARGS];
};

struct Command
{
    struct SubCommand sub_commands[MAX_SUB_COMMANDS];
    int num_sub_commands;
    char* stdin_redirect;
    char* stdout_redirect;
    int background;
};

// Function Prototypes
void PrintArgs(char** argv);
void ReadArgs(char *in, char **argv, int size);
void ReadCommand(char* line, struct Command* command);
void PrintCommand(struct Command *command);
void ReadRedirectsAndBackground(struct Command* command);
bool KeepRunningShell(const char* userInput);
void ExecuteCommand(struct Command* command);

void PrintArgs(char** argv)
{
    int i = 0;
    // Until the null termination is reached, print out
    // the elements of argv
    while(argv[i] != NULL)
    {
        printf("argv[%d] = '%s'\n", i, argv[i]);
        i++;
    }
    printf("\n");
}

void ReadArgs(char *in, char **argv, int size)
{
    // Initialize the space character that serves
    // as the delimiter.
    const char delimToken[] = " ";
    char* token;

    // First call to strtok
    // expects a C string.
    // Populate the first index of argv
    token = strtok(in, delimToken);

    int count = 0;

    // Populate the rest of the elements
    // in argv up to max_args.
    while(token != NULL && count < (size-1))
    {
        argv[count] = strdup(token);
        count++;
        token = strtok(NULL, delimToken);
    }
    
    // Guarantee that the array is null-terminated
    argv[count] = NULL;
}

void ReadCommand(char* line, struct Command* command)
{
    // Initialize the "|" character that serves
    // as the delimiter.
    const char delimToken[] = "|";
    char* token;

    // First call to strtok
    // expects a C string.
    // Populate the first index of argv
    token = strtok(line, delimToken);
    
    int count = 0;

    // Populate the rest of the elements
    // in argv up to max_args.
    while(token != NULL && count < MAX_SUB_COMMANDS)
    {
        // Duplicate the string and store it into the
        // sub-command's line data member
        command->sub_commands[count].line = strdup(token);
        count++;
        token = strtok(NULL, delimToken);
    }
    
    command->num_sub_commands = count;
    
    int i;
    for(i = 0; i < command->num_sub_commands; i++)
    {
        // Call ReadArgs to populate the SubCommand's
        // argv data member. Use a copy of the
        // line so that the original string does not get modified
        // by strtok.
        ReadArgs(strdup(command->sub_commands[i].line), command->sub_commands[i].argv, MAX_ARGS);
    }
}

void PrintCommand(struct Command *command)
{
    int i;
    // based on the number of sub commands, print out the arguments
    // for each one.
    for(i = 0; i < command->num_sub_commands; i++)
    {
        printf("Command %d:\n", i);
        PrintArgs(command->sub_commands[i].argv);
    }
    
    // Print out the Redirect stdin.
    // If it is null, then print out "N/A"
    printf("Redirect stdin: %s\n", command->stdin_redirect == NULL ? "N/A" : command->stdin_redirect);
    
    // Print out the Redirect stout.
    // If it is null, then print out "N/A"
    printf("Redirect stdout: %s\n", command->stdout_redirect == NULL ? "N/A" : command->stdout_redirect);
    
    printf("Background: %s\n", command->background ? "Yes" : "No");
}

void ReadRedirectsAndBackground(struct Command* command)
{
    char backgroundChar[] = "&";
    char inputRedirectChar[] = "<";
    char outputRedirectChar[] = ">";
    
    // Write defaults to Command
    command->background = false;
    command->stdin_redirect = NULL;
    command->stdout_redirect = NULL;
    
    // Find the last valid element in the argv
    int index = 0;
    while(command->sub_commands[command->num_sub_commands-1].argv[index] != NULL)
    {
        index++;
    }
    
    // Iterate backwarrds through the last
    // sub-command's argv array
    while(index >= 0)
    {
        char* value = command->sub_commands[command->num_sub_commands-1].argv[index];
        if(value != NULL)
        {
            // Do string comparisons to find the background character
            // or the input/output re-directions
            if(strcmp(value, backgroundChar) == 0)
            {
                command->background = true;
                command->sub_commands[command->num_sub_commands-1].argv[index] = NULL;
            }
            else if(strcmp(value, inputRedirectChar) == 0)
            {
                // If the input redirection character is found,
                // set the stdin_redirect and set the index and index + 1
                // values in argv to NULL. This is because it is guaranteed
                // that the file name is after the "<" character.
                command->stdin_redirect = strdup(command->sub_commands[command->num_sub_commands-1].argv[index+1]);
                command->sub_commands[command->num_sub_commands-1].argv[index] = NULL;
                command->sub_commands[command->num_sub_commands-1].argv[index+1] = NULL;
            }
            else if(strcmp(value, outputRedirectChar) == 0)
            {
                // Do the same as above for input redirect for the output redirect.
                command->stdout_redirect = strdup(command->sub_commands[command->num_sub_commands-1].argv[index+1]);
                command->sub_commands[command->num_sub_commands-1].argv[index] = NULL;
                command->sub_commands[command->num_sub_commands-1].argv[index+1] = NULL;
            }
        }
        index--;
    }
}

// WIP
void ExecuteCommand(struct Command* command)
{
    // Structure
    // Need to figure out how many subcommands there are
    // This will tell us how many pipes we need
    
    // Basic structure:
    // Fork and then call execvp
    // Check if the command should run in the background
    bool runInBackground = command->background == 1 ? true : false;
    
    if(runInBackground)
    {
        // Destroy children as they finish without waiting
        signal(SIGCHLD, SIG_IGN);
    }
    pid_t childID = fork();
    if(childID < 0)
    {
        // fork failed; exit
        fprintf(stderr, "fork failed\n");
        exit(0);
    }
    else if(childID == 0)
    {
        // child (new process)
        // Currently hardcoding to only first sub-command.
        execvp(command->sub_commands[0].argv[0], command->sub_commands[0].argv);
    }
    else
    {
        // parent (original process)
        // If not running in background (running in the
        // foreground), then the parent should wait.
        
        if(runInBackground)
        {
            printf("[%d]\n", childID);
            fflush(stdout);
        }
        else
        {
            wait(NULL);
        }
    }
}

bool KeepRunningShell(const char* userInput)
{
    const char exitCommand[] = "exit";
    if(strcmp(userInput, exitCommand) == 0)
    {
        return false;
    }
    return true;
}

int main()
{
    char s[1000];
    struct Command command;
    bool runningShell = true;
    while(runningShell)
    {
        struct Command command;

        // Read a string from the user
        printf("$ ");
        fflush(stdout);
        fgets(s, sizeof(s), stdin);
        
        // Check if user has empty input
        if(strcmp(&s[0], "\n") == 0)
        {
            continue;
        }
        
        // Remove newline character, if it exists from the input from user.
        s[strcspn(s,"\n")] = '\0';
        
        // Check if the exit command was typed.
        runningShell = KeepRunningShell(s);
        
        if(runningShell)
        {
            // Read commands based on user input
            ReadCommand(s, &command);
            // Read any re-directions and background characters
            // from the last sub-command.
            ReadRedirectsAndBackground(&command);
            
            // Print all commands and their contents
            //PrintCommand(&command);
            
            ExecuteCommand(&command);
        }
    }
    return 0;
}
