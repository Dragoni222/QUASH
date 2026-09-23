
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <valgrind/memcheck.h>

typedef struct job_t
{
    int JOBID;
    int PID;
    const char *command;
    int background;
    int killed;
} job_t;
job_t jobList[1000];
int lenJobList = 0;
int currentJobID = 0;

void printJob(job_t job)
{

    printf("[%d] %d %s\n", job.JOBID, job.PID, job.command);
}

void closePipes(int **pipes, int excludeout, int excludein, int totalPipes)
{
    for (int i = 0; i < totalPipes; i++)
    {
        if (i != excludeout && pipes[i][1] >= 0)
        {
            close(pipes[i][1]);
        }
        if (i != excludein && pipes[i][0] >= 0)
        {
            close(pipes[i][0]);
        }
    }
}

void updateJobs()
{
    int status;
    for (int i = 0; i < lenJobList; i++)
    {
        pid_t result = waitpid(jobList[i].PID, &status, WNOHANG);
        if (result != 0)
        {
            for (int j = i; j < lenJobList; j++)
            {
                jobList[j] = jobList[j + 1];
            }
            lenJobList--;
            i--;
        }
    }
}
void on_child_exit(int sig)
{
    int status;
    pid_t pid;
    job_t job;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < lenJobList; i++)
        {
            if (jobList[i].PID == pid)
            {
                job = jobList[i];
            }
        }
        if (job.background == 1 && !job.killed)
        {
            printf("Completed: [%d] %d %s\n", job.JOBID, pid, job.command);
        }

        updateJobs();
    }
}
void jobs()
{
    updateJobs();
    for (int i = 0; i < lenJobList; i++)
    {
        printJob(jobList[i]);
    }
}

char *upper(const char *strin, char *strout)
{

    for (int i = 0; i < strlen(strin); i++)
    {
        strout[i] = toupper(strin[i]);
    }
    strout[strlen(strin)] = '\0';
    return strout;
}

char *slice(const char *strin, char *strout, int start, int end)
{
    memmove(strout, strin + start, end - start);
    strout[end - start] = '\0';
    return strout;
}

char *strip(const char *strin, char *strout)
{
    int start = -1;
    int stop = -1;
    int length = strlen(strin);

    memcpy(strout, strin, length + 1);

    for (int i = 0; i < length; i++)
    {
        if (strout[i] != ' ' && strout[i] != '\t' && strout[i] != '\n')
        {
            start = i;
            break;
        }
    }
    if (start == -1)
    {
        strout[0] = '\0';
        return strout;
    }

    for (int i = length - 1; i >= 0; i--)
    {
        if (strout[i] != ' ' && strout[i] != '\t' && strout[i] != '\n')
        {
            stop = i;
            break;
        }
    }

    int newlen = stop - start + 1;
    memmove(strout, strout + start, newlen);
    strout[newlen] = '\0';

    return strout;
}

void echo(const char *str)
{
    char copyofstr[strlen(str) + 1];
    strcpy(copyofstr, str);
    for (int i = 0; i < strlen(copyofstr); i++)
    {
        if (copyofstr[i] == '\'' || copyofstr[i] == '\"')
        {
            for (int j = i; j < strlen(copyofstr) - 1; j++)
            {
                copyofstr[j] = copyofstr[j + 1];
            }
            copyofstr[strlen(copyofstr) - 1] = '\0';
            i--;
        }
    }
    printf("%s\n", copyofstr);
}

void pwd()
{

    char wd[1000];
    getcwd(wd, sizeof(wd));
    echo(wd);
}

void export(const char *var, const char *value)
{
    setenv(var, value, 1);
}

void cd(const char *dir)
{
    chdir(dir);
    char wd[1000];
    getcwd(wd, sizeof(wd));
    export("PWD", wd);
}



char *parse(char *preinput)
{
    fgets(preinput, 1000, stdin);
    preinput[strlen(preinput) - 1] = '\0';
    int envStart = -1;
    int envEnd = 0;
    char envBuffer[100];

    for (int i = 0; i < strlen(preinput); i++)
    {
        if (preinput[i] == '#')
        {
            preinput[i] = '\0';
            break;
        }
        else if (envStart != -1)
        {

            if (preinput[i + 1] == ' ' || preinput[i + 1] == '\0' || preinput[i + 1] == '\t' || preinput[i + 1] == '\n' || preinput[i + 1] == '\\' || preinput[i + 1] == '/')
            {
                envBuffer[envEnd - envStart] = preinput[i];
                envEnd++;
                envBuffer[envEnd - envStart] = '\0';
                char *env = getenv(envBuffer);
                char firstHalf[1000];
                char secondHalf[1000];
                slice(preinput, firstHalf, 0, envStart);
                slice(preinput, secondHalf, envEnd + 1, strlen(preinput));
                char fixedStr[30000];
                strcpy(fixedStr, firstHalf);
                strcat(fixedStr, env);
                strcat(fixedStr, secondHalf);
                strcpy(preinput, fixedStr);
                i += (strlen(env) - (envEnd - envStart));
                envStart = -1;
                envEnd = 0;
                envBuffer[0] = '\0';
            }
            else
            {
                envBuffer[envEnd - envStart] = preinput[i];
                envEnd++;
            }
        }
        else if (preinput[i] == '$')
        {
            envStart = i;
            envEnd = i;
        }
    }

    strip(preinput, preinput);
    return preinput;
}

int charInString(char charin, char *charsToSearch)
{
    for (int i = 0; i < strlen(charsToSearch); i++)
    {
        if (charsToSearch[i] == charin)
        {
            return 1;
        }
    }
    return 0;
}

void runProcess(char ***splitInputIn, const char *input, const int background, pid_t pid, int **pipes, char *delims, int index, int numOfDelimiters)
{

    char delimIn;
    char delimOut;
    char nextDelim;
    char **splitInput = splitInputIn[index];
    if (index == numOfDelimiters)
    {
        delimOut = ' ';
    }
    else
    {
        delimOut = delims[index];
    }
    if (index + 1 == numOfDelimiters)
    {
        nextDelim = ' ';
    }
    else
    {
        nextDelim = delims[index + 1];
    }
    if (index == 0)
    {
        delimIn = ' ';
    }
    else
    {
        delimIn = delims[index - 1];
    }

    if (pid == 0)
    {

        fflush(stdout);
        if (delimIn == '<')
        {
        }
        else if (delimOut == '|')
        {
            dup2(pipes[index][1], STDOUT_FILENO);
        }
        else if (delimOut == '<')
        {
            int file = open(splitInputIn[index + 1][0], O_RDONLY);
            dup2(file, STDIN_FILENO);
            close(file);
            if (nextDelim == '|')
            {
                dup2(pipes[index + 1][1], STDOUT_FILENO);
            }
            else if (nextDelim == '>')
            {
                int file = open(splitInputIn[index + 2][0], O_WRONLY  | O_TRUNC, 0644);
                dup2(file, STDOUT_FILENO);
                close(file);
            }
            else if (nextDelim == 'a')
            {
                int file = open(splitInputIn[index + 2][0], O_WRONLY  | O_APPEND, 0644);
                dup2(file, STDOUT_FILENO);
                close(file);
            }
        }
        else if (delimOut == '>')
        {
            int file = open(splitInputIn[index + 1][0], O_WRONLY | O_TRUNC, 0644);
            dup2(file, STDOUT_FILENO);
            close(file);
        }
        else if (delimOut == 'a')
        {
            int file = open(splitInputIn[index + 1][0], O_WRONLY | O_APPEND, 0644);
            dup2(file, STDOUT_FILENO);
            close(file);
        }
        if (delimIn == '|')
        {

            dup2(pipes[index - 1][0], STDIN_FILENO);
        }

        closePipes(pipes, -1, -1, numOfDelimiters);

        if (!charInString(delimIn, "a<>"))
        {
            execvp(splitInput[0], splitInput);
        }
    }
    for (int i = 0; i < numOfDelimiters; i++)
    {
        free(pipes[i]);
    }
    free(pipes);

    for (int i = 0; i < 20; i++)
    {

        for (int j = 0; j < 20; j++)
        {
            free(splitInputIn[i][j]);
        }
        free(splitInputIn[i]);
    }
    free(splitInputIn);
    exit(0);
}

char **split(const char *strin, char **strout, char *delimiters)
{
    int argnum = 0;
    int argstart = 0;

    for (int i = 0; i < strlen(strin); i++)
    {
        if (charInString(strin[i], delimiters))
        {
            slice(strin, strout[argnum], argstart, i);

            strip(strout[argnum], strout[argnum]);

            argnum++;
            argstart = i + 1;
        }
    }

    if (argstart < strlen(strin))
    {

        slice(strin, strout[argnum], argstart, strlen(strin));

        strip(strout[argnum], strout[argnum]);

        argnum++;
    }
    free(strout[argnum]);
    strout[argnum] = NULL;
    return strout;
}

int main()
{
    struct sigaction sa = {0};
    sa.sa_handler = on_child_exit;
    sigaction(SIGCHLD, &sa, NULL);
    int exitQuash = 0;
    while (exitQuash == 0)
    {
        printf("\n[QUASH]$ ");
        char preinput[1000];
        const char *input = parse(preinput);
        char upperInput[strlen(input) + 1];
        upper(input, upperInput);

        char buf1[1000], buf2[1000], buf3[1000], buf4[1000], buf5[1000], buf6[1000];

        if (strcmp(upperInput, "QUIT") == 0 || strcmp(upperInput, "EXIT") == 0)
        {
            exitQuash = 1;
        }
        else if (strcmp(slice(upperInput, buf1, 0, 4), "ECHO") == 0)
        {
            char echoBuffer[strlen(input) * sizeof(char)];
            echo(slice(input, echoBuffer, 5, strlen(input)));
        }
        else if (strcmp(slice(upperInput, buf2, 0, 3), "PWD") == 0)
        {
            pwd();
        }
        else if (strcmp(slice(upperInput, buf3, 0, 2), "CD") == 0)
        {
            
            char cdBuffer[strlen(input) * sizeof(char)];
            slice(input, cdBuffer, 3, strlen(input));
            cd(cdBuffer);
        }
        else if (strcmp(slice(upperInput, buf4, 0, 6), "EXPORT") == 0)
        {
            char postCommand[strlen(input) * sizeof(char)];
            slice(input, postCommand, 7, strlen(input));

            char varBuffer[strlen(input) * sizeof(char)];
            char valueBuffer[strlen(input) * sizeof(char)];
            int eqPos = 0;
            for (int i = 0; i < strlen(postCommand); i++)
            {
                if (postCommand[i] != '=' && eqPos == 0)
                {
                    varBuffer[i] = postCommand[i];
                }
                else if (postCommand[i] == '=')
                {
                    eqPos = i;
                    varBuffer[i] = '\0';
                }
                else
                {
                    valueBuffer[i - eqPos - 1] = postCommand[i];
                }
            }
            valueBuffer[strlen(postCommand) - eqPos - 1] = '\0';
            export(varBuffer, valueBuffer);
        }
        else if (strcmp(slice(upperInput, buf5, 0, 4), "KILL") == 0)
        {
            char **splitInput = malloc(sizeof(char *) * 20);
            for (int i = 0; i < 20; i++)
            {
                splitInput[i] = malloc(1000);
                splitInput[i][0] = '\0';
            }
            split(input, splitInput, " ");
            for (int i = 0; i < lenJobList; i++)
            {
                if (atoi(splitInput[2]) == jobList[i].PID)
                {
                    jobList[i].killed = 1;
                    kill(atoi(splitInput[2]), atoi(splitInput[1]));
                }
            }

            for (int i = 0; i < 20; i++)
            {
                free(splitInput[i]);
            }
            free(splitInput);
        }
        else if (strcmp(slice(upperInput, buf6, 0, 4), "JOBS") == 0)
        {
            jobs();
        }
        else if (strlen(input) > 0)
        {
            char ***splitInput = malloc(sizeof(char **) * 20);
            char **splitIntoCommands = malloc(sizeof(char *) * 20);
            for (int i = 0; i < 20; i++)
            {
                splitInput[i] = malloc(sizeof(char *) * 20);
                splitIntoCommands[i] = malloc(1000);
                splitIntoCommands[i][0] = '\0';
                for (int j = 0; j < 20; j++)
                {
                    splitInput[i][j] = malloc(1000);
                    splitInput[i][j][0] = '\0';
                }
            }

            char delimiters[1000] = {0};
            int numberOfDelimiters = 0;
            for (int i = 0; i < strlen(input); i++)
            {
                if (charInString(input[i], "|<>"))
                {
                    delimiters[numberOfDelimiters] = input[i];
                    numberOfDelimiters++;
                }
            }
            delimiters[numberOfDelimiters] = '\0';

            split(input, splitIntoCommands, "|<>");

            int background = 0;

            int lastIndexCommands = 0;
            int totalAllocIndexedCommands = 20;
            while (splitIntoCommands[lastIndexCommands] != NULL)
            {
                lastIndexCommands++;
            }

            if (strlen(splitIntoCommands[lastIndexCommands - 1]) > 0 && splitIntoCommands[lastIndexCommands - 1][strlen(splitIntoCommands[lastIndexCommands - 1]) - 1] == '&')
            {
                background = 1;
                splitIntoCommands[lastIndexCommands - 1][strlen(splitIntoCommands[lastIndexCommands - 1]) - 1] = '\0';
                strip(splitIntoCommands[lastIndexCommands - 1], splitIntoCommands[lastIndexCommands - 1]);
            }
            for (int i = 1; i < lastIndexCommands; i++)
            {
                if (delimiters[i] == '>' && delimiters[i - 1] == '>' && strlen(splitIntoCommands[i]) == 0)
                {
                    delimiters[i - 1] = 'a';
                    for (int j = i; j < numberOfDelimiters - 1; j++)
                    {
                        delimiters[j] = delimiters[j + 1];
                    }
                    delimiters[numberOfDelimiters - 1] = '\0';
                    free(splitIntoCommands[i]);
                    for (int j = i; j < totalAllocIndexedCommands - 1; j++)
                    {
                        splitIntoCommands[j] = splitIntoCommands[j + 1];
                    }
                    free(splitIntoCommands[totalAllocIndexedCommands - 1]);
                    splitIntoCommands[totalAllocIndexedCommands - 1] = NULL;
                    totalAllocIndexedCommands--;
                    numberOfDelimiters--;
                    lastIndexCommands--;
                }
            }
            for (int i = 0; i < lastIndexCommands; i++)
            {
                split(splitIntoCommands[i], splitInput[i], " ");
            }
            for (int i = 0; i < totalAllocIndexedCommands; i++)
            {
                free(splitIntoCommands[i]);
            }
            free(splitIntoCommands);

            int **pipes = malloc(sizeof(int *) * numberOfDelimiters);

            for (int i = 0; i < numberOfDelimiters; i++)
            {
                pipes[i] = malloc(sizeof(int) * 2);
                pipe(pipes[i]);
            }
            pid_t pids[lastIndexCommands];
            int jobID = currentJobID;
            currentJobID++;

            for (int i = 0; i < lastIndexCommands; i++)
            {
                int lastIndex = 0;
                while (splitInput[i][lastIndex] != NULL)
                {
                    lastIndex++;
                }
                pids[i] = fork();
                if (pids[i] == 0)
                {
                    runProcess(splitInput, input, background, pids[i], pipes, delimiters, i, numberOfDelimiters);
                }
            }

            if (pids[0] != 0)
            {

                closePipes(pipes, -1, -1, numberOfDelimiters);
                if (background == 0)
                {
                    for (int i = 0; i < lastIndexCommands; i++)
                    {
                        waitpid(pids[i], NULL, 0);
                    }
                }
                else
                {
                    struct job_t job = {jobID, pids[0], input, background, 0};
                    jobList[lenJobList] = job;
                    lenJobList++;
                    printf("Background job started: ");
                    printJob(job);
                }
            }
            for (int i = 0; i < numberOfDelimiters; i++)
            {
                free(pipes[i]);
            }
            free(pipes);

            for (int i = 0; i < 20; i++)
            {

                for (int j = 0; j < 20; j++)
                {
                    free(splitInput[i][j]);
                }
                free(splitInput[i]);
            }
            free(splitInput);
        }
    }
}
