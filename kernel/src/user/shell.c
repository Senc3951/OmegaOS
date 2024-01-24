#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

void shell()
{
    char root[] = "/";
    DIR *dir = opendir(root);
    if (!dir)
        exit(1);
    
    struct dirent *d;
    char dn[] = "test_dir";
    while ((d = readdir(dir)))
    {
        char new[MAX_PATH];
        size_t l = strlen(d->d_name);
        strcpy(new, d->d_name);
        new[l] = '\n';
        new[l + 1] = '\0';
        
        write(stdout, new, l + 1);
    }
    closedir(dir);
    
    int r = mkdir(dn, 0);
    if (r == 0 || r == EEXIST)
    {
        char cwd[MAX_PATH];
        getcwd(cwd, MAX_PATH);
        cwd[1] = '\n';
        cwd[2] = '\0';
        write(stdout, cwd, strlen(cwd));
        
        char newcwd[] = "/test_dir";
        int chdir_r = chdir(newcwd);

        getcwd(cwd, MAX_PATH);
        write(stdout, cwd, strlen(cwd));

        exit(chdir_r);
    }
    
    exit(r);
}