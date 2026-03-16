#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define MAX_NS 16
#define NS_NAME_LEN 32
#define MAX_FDS 256

struct fd_info
{
    unsigned long inode;
};

struct ns_info
{
    char name[NS_NAME_LEN];
    unsigned long inode;
};

struct mem_summary
{
    int total;
    int anonymous;
    int file_backed;
    int shared;
    int private;

    int has_heap;
    int has_stack;
    int has_vdso;
    int has_vvar;
};

struct proc_info
{
    pid_t pid;
    pid_t tgid;
    pid_t ppid;
    struct ns_info namespaces[MAX_NS];
    int ns_count;
    int fd_count;
    struct fd_info fds[MAX_FDS];
    int fd_stored;

    struct mem_summary mem;
};

int find_status(pid_t pid, struct proc_info *info)
{

    memset(info, 0, sizeof(*info));
    info->pid = pid;

    char ns_path[64];
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns", pid);

    DIR *dir = opendir(ns_path);
    if (dir == NULL)
    {
        perror("opendir");
        return 1;
    }
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        // printf("Entry: %s\n", entry->d_name);

        if (info->ns_count >= MAX_NS)
            break;

        char link_path[128];
        char link_target[128];

        snprintf(link_path, sizeof(link_path), "/proc/%d/ns/%s", pid, entry->d_name);

        ssize_t len = readlink(link_path, link_target, sizeof(link_target) - 1);
        if (len == -1)
        {
            perror("readlink");
            continue;
        }
        link_target[len] = '\0';
        // printf("%s -> %s\n", entry->d_name, link_target);

        unsigned long inode;

        if (sscanf(link_target, "%*[^[][%lu]", &inode) != 1)
        {
            continue;
        }
        strncpy(info->namespaces[info->ns_count].name, entry->d_name, NS_NAME_LEN - 1);
        info->namespaces[info->ns_count].inode = inode;
        info->ns_count++;
    }

    closedir(dir);

    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd", pid);
    dir = opendir(fd_path);
    if (!dir)
    {
        perror("Opendir fd");
        return 1;
    }
    info->fd_stored = 0;
    int count = 0;
    struct dirent *fd_entry;

    while ((fd_entry = readdir(dir)) != NULL)
    {
        if (fd_entry->d_name[0] == '.')
            continue;

        if (info->fd_stored >= MAX_FDS)
            break;
        count++;
        char fd_link[128];
        char fd_target[128];

        snprintf(fd_link, sizeof(fd_link), "/proc/%d/fd/%s", pid, fd_entry->d_name);

        ssize_t len = readlink(fd_link, fd_target, sizeof(fd_target) - 1);
        if (len == -1)
        {
            continue;
        }
        fd_target[len] = '\0';

        unsigned long inode;
        if (sscanf(fd_target, "%*[^[][%lu]", &inode) != 1)
        {
            continue;
        }
        info->fds[info->fd_stored].inode = inode;
        info->fd_stored++;
    }

    closedir(dir);
    info->fd_count = count;

    char map_path[64];
    snprintf(map_path, sizeof(map_path), "/proc/%d/maps", pid);
    FILE *mf = fopen(map_path, "r");
    if (!mf)
    {
        perror("fopen maps");
        return 1;
    }

    char fd_line[512];
    while (fgets(fd_line, sizeof(fd_line), mf))
    {
        info->mem.total++;

        char perms[5];
        char pathname[256] = "";

        sscanf(fd_line, "%*s %4s %*s %*s %*s %255[^\n]", perms, pathname);
        // printf("Perms: %s, Pathname: %s\n", perms, pathname);
        if (perms[3] == 's')
        {
            info->mem.shared++;
        }
        else
        {
            info->mem.private++;
        }

        if (strlen(pathname) == 0 || pathname[0] == '[')
        {
            info->mem.anonymous++;
        }
        else
        {
            info->mem.file_backed++;
        }

        if (strstr(pathname, "[heap]"))
        {
            info->mem.has_heap = 1;
        }
        else if (strstr(pathname, "[stack]"))
        {
            info->mem.has_stack = 1;
        }
        else if (strstr(pathname, "[vdso]"))
        {
            info->mem.has_vdso = 1;
        }
        else if (strstr(pathname, "[vvar]"))
        {
            info->mem.has_vvar = 1;
        }
    }

    fclose(mf);

    FILE *f;
    char status_path[64];
    char line[256];

    snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
    f = fopen(status_path, "r");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "Pid:", 4) == 0)
            sscanf(line, "Pid:\t%d", &info->pid);
        else if (strncmp(line, "Tgid:", 5) == 0)
            sscanf(line, "Tgid:\t%d", &info->tgid);
        else if (strncmp(line, "PPid:", 5) == 0)
            sscanf(line, "PPid:\t%d", &info->ppid);
    }

    fclose(f);
    return 0;
}

void compareNamespaces(struct proc_info *info1, struct proc_info *info2)
{
    printf("\t Namespaces Comparision: \n");

    for (int i = 0; i < info1->ns_count; i++)
    {
        for (int j = 0; j < info2->ns_count; j++)
        {
            if (strcmp(info1->namespaces[i].name, info2->namespaces[j].name) == 0)
            {
                printf("%s & %s: %s\n", info1->namespaces[i].name, info2->namespaces[j].name, info1->namespaces[i].inode == info2->namespaces[j].inode ? "Same" : "Different");
            }
        }
    }
}

void compareIdentity(struct proc_info *info1, struct proc_info *info2)
{
    printf("\t Identity Comparision: \n");
    printf("PID: %s\n", info1->pid == info2->pid ? "Same" : "Different");
    printf("TGID: %s\n", info1->tgid == info2->tgid ? "Same" : "Different");
    printf("PPID: %s\n", info1->ppid == info2->ppid ? "Same" : "Different");
}

void compare_fds(struct proc_info *a, struct proc_info *b)
{
    printf("\n== File Descriptors ==\n");
    printf("FD count: %d vs %d\n",
           a->fd_count, b->fd_count);
}

void compare_shared_fds(struct proc_info *a, struct proc_info *b)
{
    printf("\n== Shared FD inodes ==\n");

    int shared = 0;
    for (int i = 0; i < a->fd_stored; i++)
    {
        for (int j = 0; j < b->fd_stored; j++)
        {
            if (a->fds[i].inode == b->fds[j].inode)
            {
                shared++;
                break;
            }
        }
    }

    printf("Shared open files: %d\n", shared);
}

void compare_memory(struct proc_info *a, struct proc_info *b)
{
    printf("\n== Memory Summary ==\n");

    printf("VMAs: %d vs %d\n",
           a->mem.total, b->mem.total);

    printf("Anonymous: %d vs %d\n",
           a->mem.anonymous, b->mem.anonymous);

    printf("File-backed: %d vs %d\n",
           a->mem.file_backed, b->mem.file_backed);

    printf("Shared: %d vs %d\n",
           a->mem.shared, b->mem.shared);

    printf("Private: %d vs %d\n",
           a->mem.private, b->mem.private);

    printf("Heap: %s vs %s\n",
           a->mem.has_heap ? "yes" : "no",
           b->mem.has_heap ? "yes" : "no");

    printf("Stack: %s vs %s\n",
           a->mem.has_stack ? "yes" : "no",
           b->mem.has_stack ? "yes" : "no");
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    struct proc_info info1, info2;

    pid_t pid1 = atoi(argv[1]);
    pid_t pid2 = atoi(argv[2]);

    find_status(pid1, &info1);
    find_status(pid2, &info2);

    printf("Comparing processes %d and %d:\n", info1.pid, info2.pid);

    compareNamespaces(&info1, &info2);
    compareIdentity(&info1, &info2);
    compare_fds(&info1, &info2);
    compare_shared_fds(&info1, &info2);
    compare_memory(&info1, &info2);

    return 0;
}