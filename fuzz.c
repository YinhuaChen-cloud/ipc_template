#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PIPE_NAME "/tmp/cyh_fuzz_pipe"
#define FUZZ1 "cyhfuzz1"
#define FUZZ2 "cyhfuzz2"

int main() {
    // 用于打开命名管道的文件描述符
    int fd;      
    // 用来存放子进程的 pid
    pid_t pid;
    // 用来存放子进程发送的 nrbb，也是用于决定共享内存大小的依据
    long long nrbb = 0;
    // 用来分配和使用共享内存的变量
    int shmid;
    key_t key;
    char *shm;

    // 创建命名管道
    mkfifo(PIPE_NAME, 0777);

    // Fork a child process
    pid = fork();

    if (pid < 0) {
        // Forking failed
        fprintf(stderr, "Forking failed.\n");
        return 1;
    } else if (pid == 0) {
        // Child process
        printf("Child process PID: %d\n", getpid());

        // 使用 execvp 启动另一个程序
        char *args[] = {"./instrument", FUZZ1, NULL};
        execvp(args[0], args);

        // execvp 函数只在出错时返回，所以如果执行到这里，说明启动失败
        fprintf(stderr, "execvp error\n");
        return 1;

    } else {
        // Parent process
        printf("Parent process PID: %d\n", getpid());

        // 打开命名管道以供读取
        fd = open(PIPE_NAME, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "父进程无法打开命名管道以供读取\n");
            return 1;
        }
        // 从命名管道读取父进程写入的消息，并输出到标准输出
        printf("before read, nrbb = %lld\n", nrbb);
        read(fd, &nrbb, sizeof(nrbb));
        printf("after read, nrbb = %lld\n", nrbb);

        // 等待子进程结束
        wait(NULL);
        // 关闭命名管道和文件描述符
        close(fd);
    }

    // 读取到了子进程发过来的 nrbb，需要根据 nrbb 分配共享内存
    int shmsize = nrbb * sizeof(char); // 分配 nrbb 字节的共享内存
    // 生成一个唯一的键值
    key = ftok(".", 'R');

    // 创建或打开共享内存段
    shmid = shmget(key, shmsize, IPC_CREAT | 0777);
    printf("parent process: shmid = %d\n", shmid);
    if (shmid == -1) {
        fprintf(stderr, "无法创建共享内存段\n");
        return 1;
    }

    // 将共享内存段连接到当前进程的地址空间
    shm = shmat(shmid, NULL, 0);
    if (shm == (char *) -1) {
        fprintf(stderr, "无法连接共享内存段\n");
        return 1;
    }

    // TODO: 设置信号处理：接收到 ctrl+c 和 ctrl + d 时，要删除共享内存和命名管道，随后退出程序

    int count = 0;
    while(count < 10) {
        count++;
        printf("count = %d\n", count);

        // Fork a child process
        pid = fork();

        if (pid < 0) {
            // Forking failed
            fprintf(stderr, "Forking failed.\n");
            return 1;
        } else if (pid == 0) {
            // Child process
            printf("Child process PID: %d\n", getpid());

            // 使用 execvp 启动另一个程序
            char *args[] = {"./instrument", FUZZ2, NULL};
            execvp(args[0], args);

            // execvp 函数只在出错时返回，所以如果执行到这里，说明启动失败
            fprintf(stderr, "execvp error\n");
            return 1;

        } else {
            // Parent process
            printf("Parent process PID: %d\n", getpid());

            // 打开命名管道以供写入
            fd = open(PIPE_NAME, O_WRONLY);
            if (fd == -1) {
                fprintf(stderr, "父进程无法打开命名管道以供写入\n");
                return 1;
            }
            write(fd, &shmid, sizeof(int));
            close(fd);

            int status;

            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                printf("子进程退出状态：%d\n", WEXITSTATUS(status));
            } else {
                printf("子进程异常退出\n");
            }

            // 打印共享内存段里的内容
            for(int i = 0; i < nrbb; i++)
                printf("%c ", shm[i]);
            printf("\n");

            // 给共享内存的 512 个字节都赋值为 '0'
            for(int i = 0; i < nrbb; i++)
                shm[i] = '0';
        }
    }

    // 断开与共享内存段的连接
    shmdt(shm);

    // 删除共享内存段
    shmctl(shmid, IPC_RMID, NULL);

    // 删除命名管道
    unlink(PIPE_NAME);

    // 发送信号告知子进程已经分配好了共享内存，可以开始对共享内存写入数据了
    // printf("parent process: waiting for sending SIGUSR1 to child\n");
    // sleep(5);
    // kill(pid, SIGUSR1);

    return 0;
}

