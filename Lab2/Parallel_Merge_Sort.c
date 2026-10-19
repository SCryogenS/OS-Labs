#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

typedef struct Massive{
    int *data;
    int size;
} Massive;

typedef struct ThreadInventory{
    int *buffer;
    int thread_id;
    struct Massive *m;
    struct ThreadInventory **allinv;
    pthread_barrier_t *barrier;
    int total_threads;
    int left_border;
    int right_border;
    int size;
} ThreadInventory;

void create_massive(struct Massive *m, const int size){
    m->size = size;
    m->data = (int*)malloc(size*sizeof(int));
}

void set_thread(struct ThreadInventory *td, struct Massive *m, int left_border, int right_border, int total_threads, int thread_id, struct ThreadInventory **allinv, pthread_barrier_t *barrier){
    td->left_border = left_border;
    td->right_border = right_border;
    td->size = right_border-left_border;
    td->buffer = (int*)malloc((td->size)*sizeof(int));
    td->m = m;
    td->allinv = allinv;
    td->barrier = barrier;
    td->thread_id = thread_id;
    td->total_threads = total_threads;
}

int string_to_int(const char *str) {
    int result = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            break; 
        }
        result = (result * 10) + (str[i] - '0');
    }
    return result;
}

int read_int_directly(void) {
    int result = 0;
    char ch;
    int got_digit = 0;

    while (read(0, &ch, 1) > 0) {
        if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') {
            if (got_digit) break;
            continue;                
        }
        if (ch < '0' || ch > '9') {
            if (got_digit) break;
            continue;               
        }
        got_digit = 1;
        result = (result * 10) + (ch - '0');
    }
    return result;
}

void write_int_directly(int num) {
    if (num == 0) {
        write(1, "0", 1);
        return;
    }
    char buf[32];
    int i = 0;
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    for (int j = 0; j < i / 2; j++) {
        char t = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = t;
    }
    write(1, buf, i);
}

static void merge_sort(int *array, int *buffer, int left, int right){
    if(right - left < 2){return;}

    int mid = left + (right - left) / 2;
    merge_sort(array, buffer, left, mid);
    merge_sort(array, buffer, mid, right);

    int left_idx = left;
    int right_idx = mid;
    int buffer_idx = left;

    while(left_idx < mid && right_idx < right){
        if (array[left_idx] <= array[right_idx]){
            buffer[buffer_idx++] = array[left_idx++];
        } else {
            buffer[buffer_idx++] = array[right_idx++];
        }
    }

    while(left_idx < mid){buffer[buffer_idx++] = array[left_idx++];}
    while(right_idx < right){buffer[buffer_idx++] = array[right_idx++];}

    for (int copy = left; copy < right; copy++){
        array[copy] = buffer[copy];
    }
}

static void* parallel_merge_sort(void* arg){
    struct ThreadInventory* td =(struct ThreadInventory*) arg;

    int idx = 0;
    for (int i = td->left_border; i < td->right_border; i++){
        td->buffer[idx++] = td->m->data[i];
    }

    int *temp = (int*)malloc(td->size*sizeof(int));

    merge_sort(td->buffer, temp, 0, td->size);
    free(temp);

    pthread_barrier_wait(td->barrier);

    int step = 1;
    while (step < td->total_threads) {
        if (td->thread_id % (step * 2) == 0){   
            int left_id = td->thread_id;
            int right_id = td->thread_id + step;

            if (right_id < td->total_threads) {
                int left_size = td->allinv[left_id]->size;
                int right_size = td->allinv[right_id]->size;
                int fsize = left_size + right_size;
                
                int *temp_massive = (int*)malloc(fsize * sizeof(int));

                int left_idx = 0;  
                int right_idx = 0; 
                int temp_idx = 0;

                while (left_idx < left_size && right_idx < right_size) {
                    if (td->allinv[left_id]->buffer[left_idx] < td->allinv[right_id]->buffer[right_idx]) {
                        temp_massive[temp_idx++] = td->allinv[left_id]->buffer[left_idx++];
                    } else {
                        temp_massive[temp_idx++] = td->allinv[right_id]->buffer[right_idx++];
                    }
                }

                while (left_idx < left_size) {
                    temp_massive[temp_idx++] = td->allinv[left_id]->buffer[left_idx++];
                }

                while (right_idx < right_size) {
                    temp_massive[temp_idx++] = td->allinv[right_id]->buffer[right_idx++];
                }

                free(td->allinv[left_id]->buffer);
                td->allinv[left_id]->buffer = temp_massive;
                td->allinv[left_id]->size = fsize;
                td->allinv[left_id]->left_border = 0;
                td->allinv[left_id]->right_border = 0;

                free(td->allinv[right_id]->buffer);
                td->allinv[right_id]->size = 0;
                td->allinv[right_id]->buffer = NULL;
                td->allinv[right_id]->left_border = 0;
                td->allinv[right_id]->right_border = 0;
                }
            }
        pthread_barrier_wait(td->barrier);    
        step*=2;
    }

    return NULL;
}


int main(int argc, char *argv[]){
    
    char mess1[] = "The amount of given threads shoudnt be more then 1\n";
    char mess2[] = "Not corrected given number, it shouldnt be lower then 0 and be 0 too\n";
    char mess3[] = "Write size of your upcoming massive: ";
    char mess4[] = "Write numbers for your upcoming massive: ";
    char mess5[] = "The amount of threads is more then massive's size\n";  
    char mess6[] = "The programm couldnt create a thread\n";
    char mess7[] = "The size of massive should be at least 1\n";

    if (argc != 2){write(2, mess1, sizeof(mess1)-1); return 1;}
    
    char *temp = argv[1];
    if (temp[0] == '-' || temp[0] == '0'){write(2, mess2, sizeof(mess2)-1); return 1;}

    write(1, mess3, sizeof(mess3)-1);

    char temp_size[1024];
    ssize_t bytes_read = read(0, temp_size, sizeof(temp_size)-1);

    if (bytes_read > 0){
        if (temp_size[bytes_read - 1] == '\n'){temp_size[bytes_read - 1] = '\0';} else {temp_size[bytes_read] = '\0';}
    }else{write(2, mess7, sizeof(mess7)-1); return 1;}
    
    struct Massive m;

    const int size = string_to_int(temp_size);
    create_massive(&m, size);

    write(1, mess4, sizeof(mess4)-1);

    for (int i = 0; i < size; i++){
        m.data[i] = read_int_directly();
    }

    const int THREADS_AMOUNT = string_to_int(temp);
    if (THREADS_AMOUNT == 0){write(2, mess2, sizeof(mess2)-1); return 1;}

    pthread_t threads[THREADS_AMOUNT];
    struct ThreadInventory *inventory[THREADS_AMOUNT];

    int step = m.size / THREADS_AMOUNT; 
    if (step == 0){write(2, mess5, sizeof(mess5)-1); free(m.data); return 1;}

    struct timespec start_time, end_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, THREADS_AMOUNT);

    for (int i = 0; i<THREADS_AMOUNT; i++){
        inventory[i] = (ThreadInventory*)malloc(sizeof(ThreadInventory));
     
        int left = step*i;
        int right = (i == THREADS_AMOUNT - 1) ? m.size : (i+1)*step;

        set_thread(inventory[i], &m, left, right, THREADS_AMOUNT, i, inventory, &barrier);

        int check = pthread_create(&threads[i], NULL, parallel_merge_sort, inventory[i]);

        if (check != 0){write(2, mess6, sizeof(mess6)-1); free(m.data); free(inventory[i]); return 1;}
    }
    
    for (int i = 0; i<THREADS_AMOUNT; i++){
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    long long elapsed_ms = (end_time.tv_sec - start_time.tv_sec) * 1000LL + (end_time.tv_nsec - start_time.tv_nsec) / 1000000LL;

    char out_time_mess[] = "Execution time: ";
    write(1, out_time_mess, sizeof(out_time_mess)-1);
    write_int_directly((int)elapsed_ms);
    write(1, " ms\n", 4);

    for (int i = 0; i < inventory[0]->size; i++){write_int_directly(inventory[0]->buffer[i]); write(1, " ", 1);}
    write(1, "\n", 1);

    if (inventory[0]->buffer != NULL) {
        free(inventory[0]->buffer);
    }
    
    for (int k = 0; k < THREADS_AMOUNT; k++) {
        free(inventory[k]);
    }

    free(m.data);

    pthread_barrier_destroy(&barrier);

    return 0;
}