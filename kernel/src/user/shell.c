#include <libc/string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_LINE        100
#define USERNAME        "user2"
#define STRTOK_DELIM    " \t"

static char g_cwd[MAX_PATH];

static void ls()
{
    DIR *dir = opendir(g_cwd);
    if (!dir)
    {
        puts("No such file or directory\n");
        return;
    }

    struct dirent *d;
    while ((d = readdir(dir)))
        printf("%s\n", d->d_name);
    
    closedir(dir);
}

static void cd()
{
    char *dir = strtok(NULL, STRTOK_DELIM);
    if (!dir)
    {
        chdir("/");
        getcwd(g_cwd, MAX_PATH);
    }
    else
    {
        int res = chdir(dir);
        if (res == ENOER)
            getcwd(g_cwd, MAX_PATH);
        else if (res == ENOENT)
            printf("%s: No such file or directory\n", dir);
        else if (res == ENOTDIR)
            printf("%s: Not a directory\n", dir);
        else
            printf("%s: Error %d\n", dir, res);
    }
}

static void pwd()
{
    printf("%s\n", g_cwd);
}

static void mdir()
{
    char *dir = strtok(NULL, STRTOK_DELIM);
    if (!dir)
        puts("Missing operand\n");
    else
    {
        int res = mkdir(dir, 0);
        if (res == EEXIST)
            puts("File exists\n");
    }
}

static void echo()
{
    char *rest = strtok(NULL, "");
    if (!rest)
        putchar('\n');
    else
        printf("%s\n", rest);
}

static void help()
{
    printf("ls\ncd [dir]\npwd\nmkdir <dir>\necho <text>\nexit\n");
}

static void runCommand(const char *op)
{
    if (!strcmp(op, "ls"))
        ls();
    else if (!strcmp(op, "cd"))
        cd();
    else if (!strcmp(op, "pwd"))
        pwd();
    else if (!strcmp(op, "mkdir"))
        mdir();
    else if (!strcmp(op, "echo"))
        echo();
    else if (!strcmp(op ,"help"))
        help();
    else
        printf("%s: Command not found\n");
}

void shell()
{
    char line[MAX_LINE];
    getcwd(g_cwd, MAX_PATH);
    
    while (1)
    {
        printf("%s@%s:%s$ ", USERNAME, USERNAME, g_cwd);
        gets(line);
        if (!strlen(line))
            continue;
        
        char *op = strtok(line, STRTOK_DELIM);
        if (!op || !strcmp(op, "exit"))
            break;
        
        runCommand(op);
    }
    
    exit(0);
}