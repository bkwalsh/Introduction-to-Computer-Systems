#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

// Global Variables
char *redir_char = "//";
bool if_batch = false;
char error_message[30] = "An error has occurred\n";

// Helper Functions
void myPrint(char *msg)
{
    assert(msg);
    write(STDOUT_FILENO, msg, strlen(msg));
}

char *rmvwhitesp(char *input)
{
    if (input == NULL)
    {
        return input;
    }
    char *tl = strlen(input) + input - 1;
    char *ld = input;
    for (int i = 0; i < 514; i++)
    {
        if (!isspace(*tl) || !(ld < tl))
        {
            break;
        }
        tl--;
    }
    for (int i = 0; i < 514; i++)
    {
        if (!isspace(*ld))
        {
            break;
        }
        ld++;
    }
    *(tl + 1) = '\0';
    int length = strlen(ld) + 1;
    char *out = memcpy((void *)input, (void *)ld, length);
    return out;
}

bool blankcheck(char *st)
{
    assert(st);
    bool out;
    char *dup = strdup(st);
    rmvwhitesp(dup);
    out = !strcmp(dup, "");
    free(dup);
    return out;
}

unsigned int getac(char *cmd)
{
    assert(cmd);
    unsigned int arc = 0;
    while (*cmd)
    {
        if (*cmd == ' ')
        {
            arc = arc + 1;
        }
        cmd++;
    }
    arc++;
    return arc;
}

// System Helpers
bool potentialPath(char *fp)
{
    assert(fp);
    struct stat file_stats;
    return stat(fp, &file_stats) == 0;
}

void fileDuplicate(int cp_f, int og_f)
{
    ssize_t bytes;
    char b[1];
    if ((bytes = read(og_f, b, 1)) < 0)
    {
        perror("copyToFile");
    }
    while (bytes > 0)
    {
        if ((write(cp_f, b, bytes) < 0) || ((bytes = read(og_f, b, 1)) < 0))
        {
            perror("copyToFile");
        }
    }
}

int RedirectCheck(char *red_cmd)
{
    return (strstr(red_cmd, ">+")) || (strstr(red_cmd, ">"));
}

char *sliceRedirect(char *red_cmd)
{
    assert(red_cmd);
    if (strstr(red_cmd, ">+"))
    {
        return ">+";
    }
    return ">";
}

bool potentialRedirectPath(char *r_p, char *r_cmd)
{
    return !((!strcmp(r_p, "")) || (strchr(r_p, '>') || strchr(r_p, ' ')) ||
             (!strcmp(r_cmd, ">") && potentialPath(r_p)));
}

bool executeRedirect(int *t_fd, int *o_fd, char **out_p)
{
    assert(out_p && o_fd && t_fd);
    if (!strcmp(redir_char, ">") || !potentialPath(*out_p))
    {
        return ((*o_fd = creat(*out_p, S_IRWXU)) < 0);
    }
    if ((*t_fd = creat("temp", S_IRUSR | S_IWUSR)) < 0)
    {
        return true;
    }
    if ((*o_fd = open(*out_p, O_RDWR | O_APPEND)) < 0)
    {
        return true;
    }
    fileDuplicate(*t_fd, *o_fd);
    return (ftruncate(*o_fd, 0) < 0);
}

// Built in Commands
int BuiltinHelper(char *cmd)
{
    assert(cmd);
    if (!strncmp(cmd, "pwd", strlen("pwd")))
    {
        return 0;
    }
    if (!strncmp(cmd, "cd", strlen("cd")))
    {
        return 1;
    }
    if (!strncmp(cmd, "exit", strlen("exit")))
    {
        return 2;
    }
    return -1;
}

void pwdcmd(char *input)
{
    assert(input);
    if ((strcmp("pwd", input)))
    {
        myPrint(error_message);
        return;
    }
    char *cw_b;
    size_t cw_l = (size_t)pathconf(".", _PC_PATH_MAX);
    if ((cw_b = (char *)malloc(cw_l)))
    {
        getcwd(cw_b, cw_l);
        myPrint(cw_b);
        myPrint("\n");
        free(cw_b);
        return;
    }
}

void exitcmd(char *input)
{
    assert(input);
    if (strcmp("exit", input))
    {
        myPrint(error_message);
        return;
    }
    exit(0);
}

void cdcmd(char *input)
{
    assert(input);
    char *pth;
    if (!input[2])
    {
        chdir(getenv("HOME"));
        return;
    }
    if ((pth = rmvwhitesp(strchr(input, ' '))))
    {
        if (potentialPath(pth))
        {
            chdir(pth);
            return;
        }
    }
    myPrint(error_message);
    return;
}

// Terminal Body
char **lineParser(char **out_ph, char *cmd)
{
    assert(out_ph && cmd);
    char **aV;
    unsigned int aC;
    char *red_val = NULL;
    if (RedirectCheck(cmd))
    {
        red_val = sliceRedirect(cmd);
        char *temp = strstr(cmd, red_val) + strlen(red_val);
        *out_ph = rmvwhitesp(temp);
        if (potentialRedirectPath(*out_ph, red_val))
        {
            redir_char = cmd;
            cmd = rmvwhitesp(strtok_r(cmd, red_val, &cmd));
        }
        else
        {
            myPrint(error_message);
            return NULL;
        }
    }
    aC = getac(cmd);
    aV = (char **)malloc(sizeof(char *) * (aC + 1));
    int place = 0;
    while (place < aC)
    {
        aV[place] = strtok_r(cmd, " ", &cmd);
        place++;
    }
    aV[place] = NULL;
    return aV;
}

bool readLn(FILE **io_stream, char (*ln_buff)[514], char **ln_p)
{
    assert(io_stream && ln_buff && ln_p);
    *ln_p = fgets(*ln_buff, 514, *io_stream);
    if (!(*ln_p))
    {
        exit(0);
    }
    bool out;
    char *dup = strdup(*ln_p);
    rmvwhitesp(dup);
    out = strcmp(dup, "");
    free(dup);
    if (!out)
    {
        return false;
    }
    if (strchr(*ln_p, '\n'))
    {
        if (if_batch)
        {
            myPrint(*ln_p);
        }
        *ln_p = rmvwhitesp(*ln_p);
        return true;
    }
    while (!strchr(*ln_p, '\n'))
    {
        myPrint(*ln_p);
        *ln_p = fgets(*ln_buff, 514, *io_stream);
        if (!(*ln_p))
        {
            exit(0);
        }
    }
    myPrint(*ln_p);
    myPrint(error_message);
    return false;
}

int main(int argc, char *argv[])
{
    char cmd_buff[514];
    char *pinput;
    FILE *incoming_files = stdin;
    if (argc < 3)
    {
        if (argc == 2)
        {
            if_batch = true;
            incoming_files = fopen(argv[1], "r");
            if (!incoming_files)
            {
                myPrint(error_message);
                exit(1);
            }
        }
        while (1)
        {
            if (!if_batch)
            {
                myPrint("myshell> ");
            }
            if ((readLn(&incoming_files, &cmd_buff, &pinput)))
            {
                int t_fd = 0;
                int out_fd;
                char *out_pth;
                pid_t id;
                char **ar;
                char *cmd = rmvwhitesp(strtok_r(pinput, ";", &pinput));
                while (cmd)
                {
                    int spot;
                    if ((spot = BuiltinHelper(cmd)) != -1)
                    {
                        switch (spot)
                        {
                        case 0:
                            pwdcmd(cmd);
                            break;
                        case 1:
                            cdcmd(cmd);
                            break;
                        case 2:
                            exitcmd(cmd);
                            break;
                        default:
                            break;
                        }
                    }
                    else
                    {
                        ar = lineParser(&out_pth, cmd);
                        if (strcmp(redir_char, "//"))
                        {
                            if (executeRedirect(&t_fd, &out_fd, &out_pth))
                            {
                                myPrint(error_message);
                                ar = NULL;
                            }
                        }

                        if (!(id = fork()))
                        {
                            if (strcmp(redir_char, "//"))
                            {
                                if (dup2(out_fd, STDOUT_FILENO) < 0)
                                {
                                    exit(1);
                                }
                            }
                            if (execvp(ar[0], ar) < 0)
                            {
                                myPrint(error_message);
                            }
                            exit(1);
                        }
                        else
                        {
                            wait(NULL);
                            free(ar);
                            if (t_fd)
                            {
                                if ((out_fd = open(out_pth, O_WRONLY | O_APPEND)) < 0)
                                {
                                    perror("");
                                }
                                if ((t_fd = open("temp", O_RDONLY)) < 0)
                                {
                                    perror("");
                                }
                                fileDuplicate(out_fd, t_fd);
                            }
                        }
                    }
                    cmd = rmvwhitesp(strtok_r(pinput, ";", &pinput));
                    redir_char = "//";
                }
            }
        }
    }
    else
    {
        myPrint(error_message);
        exit(1);
    }
}
