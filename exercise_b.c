#define _GNU_SOURCE
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    // if (argc != 2)
    // {
    //     fprintf(stderr, "Usage %s <pid>\n", argv[0]);
    //     return -1;
    // }

    // pid_t pid = atoi(argv[1]);
    // int status;

    // ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    // waitpid(pid, &status, 0);

    // // waitpid(pid, &status, 0);

    // // if (WIFSTOPPED(status))
    // // {
    // //     printf("[+] Process %d stopped by signal %d\n", pid, WSTOPSIG(status));
    // // }
    // // else
    // // {
    // //     printf("[!] Unexpected stop\n");
    // //     return -1;
    // // }

    // struct user_regs_struct regs;
    // ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    // unsigned long long rip_before = regs.rip;

    // printf("RaX  before = 0x%llx\n", regs.rax);
    // printf("RsP  before = 0x%llx\n", regs.rsp);

    // ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    // waitpid(pid, &status, 0);
    // // if (!WIFSTOPPED(status)) {
    // //     fprintf(stderr, "Unexpected stop after step\n");
    // //     return 1;
    // // }

    // ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    // unsigned long long rip_after = regs.rip;
    // printf("RIP  before = 0x%llx\n", rip_before);

    // printf("RIP after = 0x%llx\n", rip_after);
    // printf("RaX  after = 0x%llx\n", regs.rax);
    // printf("RsP  after = 0x%llx\n", regs.rsp);

    // ptrace(PTRACE_DETACH, pid, NULL, NULL);
    // return 0;

    pid_t pid = atoi(argv[1]);
    int status;

    while(1){
        waitpid(pid, &status, 0);

        if(WIFEXITED(status)){
            printf("Process exited \n");
            break;
        }
        if(WIFSTOPPED(status)){
            int sig = WSTOPSIG(status);
            printf("Stopped by signal %d\n", sig);

            ptrace(PTRACE_CONT, pid, NULL, NULL);
        }

    }
}
