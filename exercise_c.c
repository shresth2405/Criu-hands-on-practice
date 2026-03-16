#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#define LINE_LEN 512

int main(int argc, char *argv[])
{
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp)
    {
        perror("fopen");
        return -1;
    }

    char line[LINE_LEN];

    unsigned long start, end, offset;
    char perms[5];

    while (fgets(line, sizeof(line), fp))
    {
        if (sscanf(line, "%lx-%lx %4s %lx", &start, &end, perms, &offset) == 4)
        {
            printf("Region: 0x%lx - 0x%lx\n", start, end);
            printf("  Permissions: %s\n", perms);
            printf("  Offset: 0x%lx\n\n", offset);
        }

        // printf("%s", line);
    }

    fclose(fp);

    return 0;
}