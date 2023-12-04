#include "fcntl.h"
#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "sys/wait.h"
#include "unistd.h"

pthread_mutex_t count_lock;
pthread_cond_t count_nonzero;
unsigned count;

int main(int argc, char *argv[]) {
  int shared_filename_fd = open("shared_filename", O_CREAT | O_RDWR, S_IRWXU);
  int shared_num_fd = open("shared_num", O_CREAT | O_RDWR, S_IRWXU);
  int shared_state_fd = open("shared_char", O_CREAT | O_RDWR, S_IRWXU);
  posix_fallocate(shared_filename_fd, 0, sizeof(char) * 100);
  posix_fallocate(shared_num_fd, 0, sizeof(int));
  posix_fallocate(shared_state_fd, 0, sizeof(int));

  char *shared_filename = mmap(NULL, sizeof(char) * 100, PROT_READ | PROT_WRITE,
                               MAP_SHARED, shared_filename_fd, 0);

  int *shared_num = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED,
                         shared_num_fd, 0);
  int *shared_state = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                           MAP_SHARED, shared_state_fd, 0);
  *shared_state = 1;

  pthread_mutex_t mutex;
  pthread_cond_t cond;

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);

  pid_t pid = fork();

  if (pid == 0) {
    int output;
    while (*shared_state != 5) {
      sleep(1);
    }
    output = open(shared_filename, O_CREAT | O_RDWR, S_IRWXU);

    *shared_state = 1;
    int sum = 0;
    while (*shared_state != 0) {
      sleep(1);
      pthread_mutex_lock(&mutex);
      // printf("%d\n", *shared_state);
      if (*shared_state == 2) {
        sum += *shared_num;
        *shared_state = 4;
      }
      if (*shared_state == 3) {
        sum += *shared_num;
        printf("%d\n", sum);
        write(output, &sum, sizeof(sum));
        sum = 0;
        *shared_state = 4;
      }

      pthread_mutex_unlock(&mutex);
    }
    printf("%d\n", a);
    read(output, &a, sizeof(a));
    printf("%d\n", a);
    read(output, &a, sizeof(a));
    printf("%d\n", a);
    
  }

  if (pid > 0) {
    pthread_mutex_lock(&mutex);
    char c = 0;
    scanf("%100s", shared_filename);
    *shared_state = 5;
    pthread_mutex_unlock(&mutex);

    while (*shared_state == 5) {
      sleep(1);
    }
    int num;
    c = ' ';
    while (scanf("%d", &num) && (c == ' ' || c == '\n')) {
      c = getchar();
      pthread_mutex_lock(&mutex);
      *shared_num = num;
      if (c == ' ') {
        *shared_state = 2;
      }
      if (c == '\n') {
        *shared_state = 3;
      }
      pthread_mutex_unlock(&mutex);
      while (*shared_state != 4) {
        sleep(1);
        // printf("a\n");
      };
    }
    *shared_state = 0;


    wait(NULL);

  }

  munmap(&shared_num, sizeof(int));
  munmap(&shared_state, sizeof(int));
}

