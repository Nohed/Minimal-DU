// Included header
// files._____________________________________________________________________________________|
#include "file_queue.h"
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct ThreadData {
  file_info *files;
  queue *queue;
  pthread_mutex_t *queue_mutex;
  sem_t *sem_subdirs; // Semaphore for tracking subdirectories
} ThreadData;

//___________________________________________________________________________________________________________|
// Functions reading input from argument list.
int find_number_of_threads(int argc, char *argv[]);
char **read_filenames(int argc, char *argv[]);
int find_number_of_files(char **files);
void init_files(char **files, file_info **file_info_array);
void print_size(file_info *file_info_array, int length);

// Functions for calculating disk usage.
int is_directory(const char *path);
void *start_thread(void *arg);
size_t file_size(char *file_name);
void search_directory(struct ThreadData *data, file_info file);

// Cleanup functions
void dealloc_file_names(char **files);

//-----------------------------------------------------------------------------------------------------------|
//						Readme
/**----------------------------------------------------------------------------------------------------------|
 *
 *
 *
 *	DOCUMENTATION HERE
 *
 *	Error handling
 *	Functions
 *	version
 *	brief
 * 	Usage
 *	 WILL ALWAYS RUN WITH A THREAD
 *
 */
//-----------------------------------------------------------------------------------------------------------|

int main(int argc, char *argv[]) {
  // Read input, initialize variables.
  int number_of_threads = find_number_of_threads(argc, argv);
  char **files = read_filenames(argc, argv);
  int number_of_files = find_number_of_files(files);
  if (number_of_files == 0) {
    fprintf(stderr, "USAGE: No files \n"); //! PRINT CORRECT USAGE
    exit(EXIT_FAILURE);
  }

  file_info file_info_array[number_of_files];
  for (int i = 0; files[i] != NULL; ++i) {
    file_info_array[i].file_name = files[i];
    file_info_array[i].file_size = 0;
    file_info_array[i].array_number = i;
    printf("checking: %s\n", file_info_array[i].file_name);
  }
  // Initialize the semaphore
  sem_t sem_subdirs;
  if (sem_init(&sem_subdirs, 0, 0) != 0) {
    perror("Semaphore initialization failed");
    exit(EXIT_FAILURE);
  }

  // Initialize the queue
  queue file_queue;
  initializeQueue(&file_queue);

  // Populate the file queue with initial files
  for (int i = 0; i < number_of_files; ++i) {
    enqueue(&file_queue, file_info_array[i]);
  }
  // Create thread data
  ThreadData thread_data[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i) {
    thread_data[i].files = file_info_array;
    thread_data[i].queue = &file_queue;
    thread_data[i].queue_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(thread_data[i].queue_mutex, NULL);
    thread_data[i].sem_subdirs = &sem_subdirs;
  }

  // Create worker threads
  pthread_t th[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i) {
    pthread_create(&th[i], NULL, start_thread, &thread_data[i]);
  }

  // Wait for all worker threads to finish
  for (int i = 0; i < number_of_threads; ++i) {
    pthread_join(th[i], NULL);
  }

  print_size(file_info_array, number_of_files);
  // Cleanup and exit.
  dealloc_file_names(files);
  destroyQueue(&file_queue);

  // Destroy the semaphore
  sem_destroy(&sem_subdirs);

  return EXIT_SUCCESS;
}

void *start_thread(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  printf("starting thread\n");
  while (1) {
    sem_wait(data->sem_subdirs);
    printf("while loop\n");
    pthread_mutex_lock(data->queue_mutex);

    if (isEmpty(data->queue)) {
      pthread_mutex_unlock(data->queue_mutex);
      break; // Queue is empty, exit thread
    }

    file_info file = dequeue(data->queue);
    pthread_mutex_unlock(data->queue_mutex);

    // Process the file (calculate size, print, etc.)
    if (is_directory(file.file_name) != 0) {
      // Process the directory and add subdirectories to the queue
      search_directory(data, file);
    } else {
      // Process the file (calculate size, print, etc.)
      pthread_mutex_lock(data->queue_mutex);
      data->files[file.array_number].file_size += file_size(
          file.file_name); // add filesize to the corresponding directory.
      pthread_mutex_unlock(data->queue_mutex);
    }
    sem_post(data->sem_subdirs);
  }

  pthread_exit(NULL);
}

void search_directory(struct ThreadData *data, file_info file) {
  DIR *dir = opendir(file.file_name);
  if (!dir) {
    printf("Can't open directory %s\n", file.file_name);
    return; // Return 0 to indicate an error
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) // Is a directory
    {
      if (strcmp(entry->d_name, ".") == 0 ||
          strcmp(entry->d_name, "..") == 0) //! Does code search .
      {
        continue; // Skip . and .. to prevent loop.
      }

      char subpath[PATH_MAX];
      snprintf(subpath, sizeof(subpath), "%s/%s", file.file_name,
               entry->d_name);

      file_info subdir; //! DO i need to allocate memory for this?
      subdir.file_name = subpath;
      subdir.array_number = file.array_number;

      // Recursively calculate size of subdirectories
      pthread_mutex_lock(data->queue_mutex);
      enqueue(data->queue, subdir); // add value to queue
      pthread_mutex_unlock(data->queue_mutex);
    } else if (entry->d_type == DT_REG) // is a file
    {
      char filepath[PATH_MAX];
      snprintf(filepath, sizeof(filepath), "%s/%s", file.file_name,
               entry->d_name);

      // Calculate and add size of regular files
      pthread_mutex_lock(data->queue_mutex);
      data->files[file.array_number].file_size +=
          file_size(filepath); // add filesize to according dirr.
      printf("filesize of %s is %lu", filepath, file_size(file.file_name));
      pthread_mutex_unlock(data->queue_mutex);
    }
  }

  closedir(dir);
}

int find_number_of_threads(int argc, char *argv[]) {
  optind = 1; // Reset optind,
  int opt;
  int number = 1;

  while ((opt = getopt(argc, argv, "j:")) != -1) {
    if (opt == 'j') {
      number = atoi(optarg);
    }
  }
  return number;
}

char **read_filenames(int argc, char *argv[]) {
  int count = 0; // Number of filenames
  char **files = malloc((argc - optind + 1) *
                        sizeof(char *)); //! Memmory deallocated on row XXXXXXXX
                                         //! in dealloc_file_names

  for (int i = optind; i < argc; ++i) {
    // Check if the current argument is not an integer (after -j)
    if (i > optind && argv[i - 1][0] == '-' && argv[i - 1][1] == 'j') {
      continue; // Skip this argument
    }

    // Allocate memory for the filename
    files[count] = malloc(strlen(argv[i]) + 1);
    strcpy(files[count], argv[i]);

    ++count;
  }

  // Add a NULL pointer at the end of the array to indicate the end
  files[count] = NULL;

  return files;
}

int find_number_of_files(char **files) {
  int count = 0;
  for (int i = 0; files[i] != NULL; ++i) {
    ++count;
  }
  return count;
}

int is_directory(const char *path) {
  struct stat path_stat;

  if (lstat(path, &path_stat) != 0) {
    perror("Error in lstat");
    return -1; // Return -1 to indicate an error
  }

  return S_ISDIR(path_stat.st_mode);
}

size_t file_size(char *file_name) {
  struct stat st;
  size_t size;
  if (stat(file_name, &st) == 0) {
    size = st.st_size;
    return size;
  } else {
    printf("File: %s not found.\n", file_name);
    return 0;
  }
}

void dealloc_file_names(char **files) {
  for (int i = 0; files[i] != NULL; ++i) {
    free(files[i]);
  }
  free(files);
}

void print_size(file_info *file_info_array, int length) {
  for (int i = 0; i < length; i++) {
    printf("%s \t %lu\n", file_info_array[i].file_name,
           file_info_array[i].file_size);
  }
}
