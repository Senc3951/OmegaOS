#include <user_test/unistd.h>
#include <user_test/fcntl.h>
#include <user_test/string.h>

void shell()
{
    char root[] = "/";
    DIR *dir = opendir(root);
    if (!dir)
        exit(1);
    
    struct dirent *d;
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
    
    int r = mkdir("test_dir", 0);
    exit(r);
}