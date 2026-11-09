#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>

void cleanup_child(pid_t pid, int fd, sem_t *sem, const char *shm_name, const char *sem_name)
{
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);

    close(fd);
    sem_close(sem);
    shm_unlink(shm_name);
    sem_unlink(sem_name);
}

int from_str_to_int(int *result, char *word){
    *result = 0;
    int ptr = 0;

    while (word[ptr]!='\0'){
        if (!(word[ptr] >= '0' && word[ptr] <= '9')){return 1;}

        int num = word[ptr] - '0';
        if (*result > (INT_MAX - num)/10){return 2;}
        *result = *result * 10 + num;

        ptr++;
    }

    return 0;
}

int main(){
    char mess1[] = "Got a word in potential number\n";
    char mess2[] = "Faced with overflow\n";
    char mess3[] = "Shm_open problem\n";
    char mess4[] = "MAP_FAILED\n";
    char mess5[] = "Memory Alocation problem\n";
    char mess6[] = "ftruncate Error\n";
    char mess7[] = "Not enought size\n";
    char mess8[] = "Sem troubles\n";
    char mess9[] = "Child is dead\n";
    char mess11[] = "Child faced with overflow\n";
    char mess12[] = "Child couldn't open or create a file\n";
    char mess13[] = "Execv didn't work\n";
    char mess14[] = "Child didn't get enought elements\n";
    char mess15[] = "Child is succsefully ended\n";

    char shm_name[64];
    char sem_name[64];

    snprintf(shm_name, sizeof(shm_name),"/SharedMemory_%ld", (long)getpid());

    snprintf(sem_name, sizeof(sem_name),"/SemFile_%ld", (long)getpid());

    int fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1){write(2, mess3, sizeof(mess3)-1); return 1;}

    sem_t *sem = sem_open(sem_name, O_CREAT | O_EXCL, 0666, 0);
    pid_t pid;

    if (sem == SEM_FAILED){write(2, mess8, sizeof(mess8)-1); return 1;}

    char filename[256];
    write(1, "Enter filename: ", 16);

    ssize_t fn_bytes = read(0, filename, sizeof(filename)-1);

    if (fn_bytes <= 0){return 1;}
    if (filename[fn_bytes-1] == '\n'){
        filename[fn_bytes-1] = '\0';
    }else{filename[fn_bytes] = '\0';}

    pid = fork();

    if (pid<0){write(2, "Error with fork\n", 16); return 1;}

    if (pid == 0){

        char *argv[] = {"./child", filename, shm_name, NULL};

        sem_wait(sem);
        execv(argv[0], argv);

        exit(1);

    }else{
        char *buffer = (char*)malloc(sizeof(char));
        int *numbers = NULL;
        int ptr = 0;
        int counter = 0;
        int end = false;

        while (read(0, &buffer[ptr], 1) == 1){
            if (buffer[ptr] == '\n'){end = true;}

            ptr++;
            buffer = (char*)realloc(buffer, ptr+1);
            if (buffer == NULL){
                write(2, mess5, sizeof(mess5)-1);
                free(numbers);
                cleanup_child(pid, fd, sem, shm_name, sem_name);
                return 1;
            }

            if (buffer[ptr-1] == ' ' || buffer[ptr-1] == '\n'){
                buffer[ptr-1] = '\0';
                counter++;

                numbers = (int*)realloc(numbers, counter*sizeof(int));
                if (numbers == NULL){
                    write(2, mess5, sizeof(mess5)-1);
                    free(buffer);
                    cleanup_child(pid, fd, sem, shm_name, sem_name);
                    return 1;
                }

                int status = from_str_to_int(&numbers[counter-1], buffer);
                if (status == 1){
                    write(2, mess1, sizeof(mess1)-1);
                    free(buffer);
                    free(numbers);
                    cleanup_child(pid, fd, sem, shm_name, sem_name);
                    return 1;
                }
                if (status == 2){
                    write(2, mess2, sizeof(mess2)-1);
                    free(buffer);
                    free(numbers);
                    cleanup_child(pid, fd, sem, shm_name, sem_name);
                    return 1;
                }

                ptr = 0;

                if (end){break;}
            }
        }

        size_t size = counter * sizeof(int);

        if (size == 0){
            write(2, mess7, sizeof(mess7)-1);
            free(buffer);
            free(numbers);
            cleanup_child(pid, fd, sem, shm_name, sem_name);
            return 1;
        }
        
        if (ftruncate(fd, size) == -1){
            write(2, mess6, sizeof(mess6)-1);
            free(buffer);
            free(numbers);
            cleanup_child(pid, fd, sem, shm_name, sem_name);
            return 1;
        }
        
        int *nums = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
        if (nums == MAP_FAILED){
            write(2, mess4, sizeof(mess4)-1);
            free(buffer);
            free(numbers);
            cleanup_child(pid, fd, sem, shm_name, sem_name);
            return 1;
        }

        for (int i = 0; i<counter; i++){nums[i]=numbers[i];}

        close(fd);

        munmap(nums, size);
        free(buffer);
        free(numbers);

        sem_post(sem);

        int status;
        pid_t result = waitpid(pid, &status, 0);

        if (result<0){write(2, mess9, sizeof(mess9)-1); return 1;}
        if (result>0){
            if (WIFEXITED(status)){
                shm_unlink(shm_name);
                sem_unlink(sem_name);
                sem_close(sem);
                int signal = WEXITSTATUS(status);
                if (signal == 0){write(1, mess15, sizeof(mess15)-1); return 0;}
                if (signal == 1){write(2, mess13, sizeof(mess13)-1); return 1;}
                if (signal == 2){write(2, mess14, sizeof(mess14)-1); return 1;}
                if (signal == 3){write(2, mess3, sizeof(mess3)-1); return 1;}
                if (signal == 4){write(2, mess4, sizeof(mess4)-1); return 1;}
                if (signal == 5){write(2, mess11, sizeof(mess11)-1); return 1;}
                if (signal == 6){write(2, mess12, sizeof(mess12)-1); return 1;}
            }
        }
    }
}





