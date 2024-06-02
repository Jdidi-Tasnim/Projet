#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h> 

#define NUM_CHILDREN 4

sem_t *semaphore;

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        printf("Received SIGUSR1\n");
    } else if (signal == SIGUSR2) {
        printf("Received SIGUSR2\n");
    } else if (signal == SIGTERM) {
        printf("Received SIGTERM\n");
    }
}

void child_process() {
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGTERM, signal_handler);

    sem_wait(semaphore);

    // Simulate complex task
    sleep(5);

    // Send confirmation signal to parent
    kill(getppid(), SIGUSR1);

    exit(0);
}

int main() {
    pid_t pid[NUM_CHILDREN];

    semaphore = sem_open("/semaphore", O_CREAT, 0644, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid[i] = fork();
        if (pid[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid[i] == 0) {
            child_process();
        }
    }

    // Send start signal to children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        kill(pid[i], SIGUSR1);
    }

    // Release semaphore
    for (int i = 0; i < NUM_CHILDREN; i++) {
        sem_post(semaphore);
    }

    // Wait for children to finish
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    sem_close(semaphore);
    sem_unlink("/semaphore");

    return 0;
}
