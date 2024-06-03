#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define NUM_CHILDREN 4
#define SEM_NAME "/sync_semaphore"
pid_t children[NUM_CHILDREN];
sem_t *sync_semaphore;
void sig_handler(int sig, siginfo_t *si, void *unused) {
    if (sig == SIGUSR2) {
        printf("Parent received confirmation from child with PID: %d\n", si->si_pid);
    }
}
void child_process(int id) {
    // Attendre le signal SIGUSR1 du processus père
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    printf("Child %d waiting for SIGUSR1\n", getpid());
    int sig;
    sigwait(&mask, &sig);

    // Simuler une tâche
    sleep(1 + id); // Simuler une tâche différente pour chaque processus

    // Envoyer un signal de confirmation au processus père
    kill(getppid(), SIGUSR2);

    // Fin du processus fils
    exit(EXIT_SUCCESS);
}
void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sig_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}
int main() {
    // Initialiser le sémaphore
    sync_semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    if (sync_semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Configurer les gestionnaires de signaux
    setup_signal_handlers();

    // Créer des processus fils
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Processus fils
            child_process(i);
        } else {
            // Processus père
            children[i] = pid;
        }
    }

    // Attendre un moment pour que tous les processus fils soient prêts
    sleep(1);

    // Envoyer un signal SIGUSR1 à tous les processus fils pour qu'ils commencent leurs tâches
    for (int i = 0; i < NUM_CHILDREN; i++) {
        kill(children[i], SIGUSR1);
    }

    // Attendre que tous les processus fils terminent
    for (int i = 0; i < NUM_CHILDREN; i++) {
        waitpid(children[i], NULL, 0);
    }

    // Nettoyer le sémaphore
    sem_close(sync_semaphore);
    sem_unlink(SEM_NAME);

    return EXIT_SUCCESS;
}

