#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <assert.h>

#define PIPE_NAME "/tmp/cyh_fuzz_pipe"
#define FUZZ1 "cyhfuzz1"
#define FUZZ2 "cyhfuzz2"

int main(int argc, char *argv[]) {
    int fd;      // 父子进程用于打开命名管道的文件描述符
    long long nrbb = 512; // 子进程传递的 nrbb
    sigset_t set;
    int sig;

    int executeType = 0;

    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], FUZZ1) == 0)
            executeType = 1;
        if(strcmp(argv[i], FUZZ2) == 0)
            executeType = 2;
    }
    assert(executeType == 1 || executeType == 2);

    printf("executeType = %d\n", executeType);

    if(executeType == 1) {
        // 打开命名管道以供写入
        fd = open(PIPE_NAME, O_WRONLY);
        if (fd == -1) {
            fprintf(stderr, "子进程无法打开命名管道\n");
            return 1;
        }
        // 从标准输入读取用户输入，并写入命名管道
        write(fd, &nrbb, sizeof(nrbb));
        // 关闭命名管道和文件描述符
        close(fd);
    }
    else {
        // 打开命名管道以供读取
        int shmid = 0;
        fd = open(PIPE_NAME, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "子进程无法打开命名管道以供读取\n");
            return 1;
        }
        // 从命名管道读取父进程写入的消息，并输出到标准输出
        printf("before read, shmid = %d\n", shmid);
        read(fd, &shmid, sizeof(int));
        printf("after read, shmid = %d\n", shmid);
        // 关闭命名管道和文件描述符
        close(fd);

        // 到这里说明父进程已经分配好了共享内存
        // 将共享内存段连接到当前进程的地址空间
        char *shm;
        printf("child process: shmid = %d\n", shmid);
        shm = shmat(shmid, NULL, 0);
        if (shm == (char *) -1) {
            fprintf(stderr, "子进程：无法连接共享内存段\n");
            return 1;
        }

        // 打印共享内存段里的内容
        for(int i = 0; i < nrbb; i++)
            printf("%c ", shm[i]);
        printf("\n");


        // 给共享内存的 512 个字节都赋值为 'a'
        for(int i = 0; i < nrbb; i++)
            shm[i] = 'x';

        // 断开与共享内存段的连接
        shmdt(shm);

        // 关闭命名管道和文件描述符
        close(fd);
    }

    return 0;

    // 等待从父进程发过来的 ACK，表示父进程已经申请好了共享内存，子进程可以开始往共享内存写入数据了
    // 设置要等待的信号
    // sigemptyset(&set);
    // sigaddset(&set, SIGUSR1); // 要等待信号 SIGUSR1

    // // printf("child process: waiting for SIGUSR1 from parent\n");
    // // 等待父进程发送信号
    // sigwait(&set, &sig);
}