#include "sys/wait.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {

  pthread_cond_t *condition;
  pthread_mutex_t *mutex;
  char *message;
  int des_cond, des_msg, des_mutex;

  des_mutex =
      shm_open("mutex_lock", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXG);

  if (des_mutex < 0) {
    perror("failure on shm_open on des_mutex");
    exit(1);
  }

  if (ftruncate(des_mutex, sizeof(pthread_mutex_t)) == -1) {
    perror("Error on ftruncate to sizeof pthread_cond_t\n");
    exit(-1);
  }

  mutex =
      (pthread_mutex_t *)mmap(NULL, sizeof(pthread_mutex_t),
                              PROT_READ | PROT_WRITE, MAP_SHARED, des_mutex, 0);

  if (mutex == MAP_FAILED) {
    perror("Error on mmap on mutex\n");
    exit(1);
  }

  des_cond =
      shm_open("condwrite", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXG);

  if (des_cond < 0) {
    perror("failure on shm_open on des_cond");
    exit(1);
  }

  if (ftruncate(des_cond, sizeof(pthread_cond_t)) == -1) {
    perror("Error on ftruncate to sizeof pthread_cond_t\n");
    exit(-1);
  }

  condition =
      (pthread_cond_t *)mmap(NULL, sizeof(pthread_cond_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, des_cond, 0);

  if (condition == MAP_FAILED) {
    perror("Error on mmap on condition\n");
    exit(1);
  }

  pthread_mutexattr_t mutexAttr;
  pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(mutex, &mutexAttr);

  pthread_condattr_t condAttr;
  pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(condition, &condAttr);

  int shared_filename_fd =
      shm_open("shared_filename", O_CREAT | O_RDWR, S_IRWXU);
  int shared_num_fd = shm_open("shared_num", O_CREAT | O_RDWR, S_IRWXU);
  int shared_state_fd = shm_open("shared_char", O_CREAT | O_RDWR, S_IRWXU);
  if (ftruncate(shared_filename_fd, sizeof(int)) == -1) {
    perror("Error on ftruncate to sizeof shared_filename_fd\n");
    exit(-1);
  }
  if (ftruncate(shared_num_fd, sizeof(int)) == -1) {
    perror("Error on ftruncate to sizeof shared_num_fd\n");
    exit(-1);
  };
  if (ftruncate(shared_state_fd, sizeof(int)) == -1) {
    perror("Error on ftruncate to sizeof shared_state_fd\n");
    exit(-1);
  };

  char *shared_filename = mmap(NULL, sizeof(char) * 100, PROT_READ | PROT_WRITE,
                               MAP_SHARED, shared_filename_fd, 0);
  int *shared_num = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED,
                         shared_num_fd, 0);
  int *shared_state = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                           MAP_SHARED, shared_state_fd, 0);
  *shared_state = 1;
  shared_filename[0] = 0;

  pid_t pid = fork();

  if (pid == -1) {
    perror("Error on fork\n");
    exit(0);
  }

  if (pid == 0) {
    int sum = 0;

    while (shared_filename[0] == 0) {
      pthread_mutex_lock(mutex);
      pthread_cond_wait(condition, mutex);
      pthread_mutex_unlock(mutex);
    }

    int output = open(shared_filename, O_CREAT | O_WRONLY, S_IRWXU);
    while (*shared_state != 0) {
      pthread_mutex_lock(mutex);
      if (*shared_state == 2) {
        sum += *shared_num;
        *shared_state = 1;
        pthread_cond_signal(condition);
      }
      if (*shared_state == 3) {
        sum += *shared_num;
        write(output, &sum, sizeof(sum));
        printf("%d\n", sum);
        sum = 0;
        *shared_state = 1;
        pthread_cond_signal(condition);
      }
      pthread_mutex_unlock(mutex);
    }
    close(output);

    int a = open(shared_filename, O_RDONLY, S_IRWXU);

    read(output, &sum, sizeof(sum));
    printf("%d\n", sum);
    close(output);
  }

  else {

    scanf("%100s", shared_filename);
    pthread_cond_signal(condition);
    char c = ' ';
    while (scanf("%d", shared_num) && (c == ' ' || c == '\n')) {
      c = getchar();
      pthread_mutex_lock(mutex);
      if (c == ' ') {
        *shared_state = 2;
      }
      if (c == '\n') {
        *shared_state = 3;
      }
      pthread_cond_wait(condition, mutex);
      pthread_mutex_unlock(mutex);
    }
    *shared_state = 0;
    pthread_condattr_destroy(&condAttr);
    pthread_mutexattr_destroy(&mutexAttr);
    pthread_mutex_destroy(mutex);
    pthread_cond_destroy(condition);

    wait(NULL);
  }

  return 0;
}
