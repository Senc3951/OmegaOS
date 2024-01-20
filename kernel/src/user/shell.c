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
        size_t l = strlen(d->name);
        strcpy(new, d->name);
        new[l] = '\n';
        new[l + 1] = '\0';
        
        write(stdout, new, l + 1);
    }
    closedir(dir);
    
    int r = mkdir(dn, 0);
    exit(r);
}