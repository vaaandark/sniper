#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

/*
 * Usage:
 *   sniper {PID}
 * then you will get the output of the program
 */

void help(void) {
    fprintf(stderr, "Print the output of a running process\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    sniper [OPTIONS] PID\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -v    Print verbose debug output.\n");
    fprintf(stderr, "    -h    Print this help message and exit.\n");
}

ssize_t vm_read(pid_t pid, void *ptr, size_t len, char *buff) {
    bzero(buff, 1024);
    struct iovec local, remote;
    remote.iov_base = ptr;
    remote.iov_len = len;
    local.iov_base = buff;
    local.iov_len = len;
    return process_vm_readv(pid, &local, 1, &remote, 1, 0);
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    int opt;

    struct user_regs_struct regs;
    pid_t pid;
    int status;
    char buff[1024];
    bool in_syscall = false;

    while ((opt = getopt(argc, argv, "vh")) != -1) {
        switch (opt) {
        case 'v':
            verbose = true;
            break;
        case 'h':
            help();
            return 0;
        default:
            help();
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        help();
        exit(EXIT_FAILURE);
    }

    pid = atoi(argv[optind]);

    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    wait(&status);
    ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD);
    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    while (wait(&status) && !WIFEXITED(status)) {
        if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            int additional = status >> 16;
            if (sig == (SIGTRAP | 0x80)) {
                if (in_syscall) {
                    in_syscall = false;
                } else {
                    void *ptr;
                    size_t len;
                    size_t ret;
                    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
                    ptr = (void *)regs.rsi;
                    len = regs.rdx;
                    if ((ret = vm_read(pid, ptr, len, buff)) > 0) {
                        if (verbose) {
                            printf("%d | %p | %zu\n", pid, ptr, len);
                        }
                        printf("%s", buff);
                        fflush(stdout);
                    }
                    in_syscall = true;
                }
                ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            } else if (additional == 0) {
                ptrace(PTRACE_SYSCALL, pid, NULL, &sig);
                break;
            }
        } else {
            break;
        }
    }

    return 0;
}
