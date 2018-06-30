#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define FILEPATH "/mmapped.bin"
#define NUMINTS  (1)
#define FILESIZE (NUMINTS * sizeof(int))

void setSharedMemory(int value){
  int fd = shm_open(FILEPATH, O_CREAT|O_RDWR, (mode_t)0666);
  ftruncate(fd, FILESIZE);
  if (fd == -1) {
      perror("Error opening file for writing");
      exit(EXIT_FAILURE);
  }
  int *map;  /* mmapped array of int's */
  map = (int*)mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    close(fd);
    perror("Error mmapping the file");
    exit(EXIT_FAILURE);
  }
  map[0] = value;
  if (munmap(map, FILESIZE) == -1) {
    perror("Error un-mmapping the file");
  }
  close(fd);
  printf("set shared memory as : %d\n", value);
}

int getSharedMemory(){
  int fd = shm_open(FILEPATH, O_RDWR, (mode_t)0666 );
  if (fd == -1) {
    perror("Error opening file for reading");
    exit(EXIT_FAILURE);
  }

  int *map = (int*)mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  int result = map[0];

  if (munmap(map, FILESIZE) == -1) {
    perror("Error un-mmapping the file");
  }
  close(fd);

  return result;
}
