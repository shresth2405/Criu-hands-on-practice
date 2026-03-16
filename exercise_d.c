#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAX_FDS 1024

struct fd_info
{
    int fd;
    char target[4096];
    int flags;
    __off_t offset;
};

int is_number(const char *s)
{
    for (; *s; s++)
    {
        if (*s < '0' || *s > 9)
        {
            return 0;
        }
    }
    return 1;
}

void save_fd_info(struct fd_info *info, int fd)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

    __ssize_t len = readlink(path, info->target, sizeof(info->target) - 1);

    if (len < 0)
    {
        perror("readlink");
        exit(1);
    }
    info->target[len] = '\0';
    info->flags = fcntl(fd, F_GETFL);
    info->offset = fcntl(fd, 0, SEEK_CUR);
}

void restore_fd(struct fd_info fds[], int count){
    for(int i=0; i<count; i++){
        int newfd = open(fds[i].target, fds[i].flags);
        if(newfd<0){
            perror("open");
            continue;
        }
        if(newfd != fds[i].fd){
            dup2(newfd, fds[i].fd);
            close(newfd);
        }

        lseek(fds[i].fd, fds[i].offset, SEEK_SET);
    }
    printf("Fds restored successfully\n");
}

int main(int argc, char argv[])
{
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
    {
        perror("opendir");
        return -1;
    }

    struct fd_info fds[MAX_FDS];
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (!is_number(ent->d_name))
        {
            continue;
        }
        int fd = atoi(ent->d_name);
        if (fd == dirfd(dir))
        {
            continue;
        }
        fds[count].fd = fd;

        // char path[64];

        // snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

        // ssize_t len = readlink(path, fds[count].target, sizeof(fds[count].target) - 1);

        // if (len < 0)
        // {
        //     return 1;
        // }

        // fds[count].target[len] = '\0';
        // fds[count].flags = fcntl(fd, F_GETFL);
        // fds[count].offset = fcntl(fd, 0, SEEK_CUR);

        save_fd_info(fds, fd);
        count++;
    }
    for (int i = 0; i < count; i++)
    {
        close(fds[i].fd);
    }

    printf("All FDs closed.\n");

    closedir(dir);

    printf("Saved file descriptors\n");

    restore_fd(fds,count);

    return 0;
}