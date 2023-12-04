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
#include "sys/wait.h"


#define OKTOWRITE "condwrite"
#define MESSAGE "msg"
#define MUTEX "mutex_lock"

int main(int argc, char **argv) {

  pthread_cond_t *condition;
  pthread_mutex_t *mutex;
  char *message;
  int des_cond, des_msg, des_mutex;
  int mode = S_IRWXU | S_IRWXG;

  des_mutex = shm_open(MUTEX, O_CREAT | O_RDWR | O_TRUNC, mode);

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

  des_cond = shm_open(OKTOWRITE, O_CREAT | O_RDWR | O_TRUNC, mode);

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

  /* HERE WE GO */
  /**************************************/

  /* set mutex shared between processes */
  pthread_mutexattr_t mutexAttr;
  pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(mutex, &mutexAttr);

  /* set condition shared between processes */
  pthread_condattr_t condAttr;
  pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(condition, &condAttr);

  /*************************************/

  int shared_num_fd = shm_open("shared_num", O_CREAT | O_RDWR, S_IRWXU);
  int shared_state_fd = shm_open("shared_char", O_CREAT | O_RDWR, S_IRWXU);
  posix_fallocate(shared_num_fd, 0, sizeof(int));
  posix_fallocate(shared_state_fd, 0, sizeof(int));

  int *shared_num = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED,
                         shared_num_fd, 0);
  int *shared_state = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                           MAP_SHARED, shared_state_fd, 0);
  *shared_state = 1;

  pid_t pid = fork();
  if (pid == 0) {

    int sum = 0;

    int output = open("fff", O_CREAT | O_WRONLY, S_IRWXU);

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

    
  }

  else {
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

    shm_unlink(OKTOWRITE);
    shm_unlink(MESSAGE);
    shm_unlink(MUTEX);

     wait(NULL);
}

  return 0;
}
