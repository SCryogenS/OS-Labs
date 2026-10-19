#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

typedef struct Massive{
    int *data;
    int size;
} Massive;

void create_massive(struct Massive *m, const int size){
    m->size = size;
    m->data = (int*)malloc(size*sizeof(int));
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

static void sequential_merge_sort(struct Massive *m){
    int *temp = (int*)malloc(m->size*sizeof(int));
    merge_sort(m->data, temp, 0, m->size);
    free(temp);
}


int main(){

    char mess1[] = "Write the size of the upcoming massive: ";
    char mess2[] = "Write the elements for the upcoming massive: ";
    char mess3[] = "The size of massive should be atleast 1\n";

    write(1, mess1, sizeof(mess1)-1);
    
    char temp[1024];
    ssize_t bytes_read = read(0, temp, sizeof(temp)-1);
    
    if (bytes_read <= 0){write(2, mess3, sizeof(mess3)-1); return 1;}

    if (temp[bytes_read-1] == '\n'){
        temp[bytes_read-1] = '\0';
    }else{temp[bytes_read] = '\0';}

    const int size_m = string_to_int(temp);
    struct Massive m;

    create_massive(&m, size_m);

    write(1, mess2, sizeof(mess2)-1);
    for (int i = 0; i < size_m; i++){m.data[i] = read_int_directly();}


    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    sequential_merge_sort(&m);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long long elapsed_ms = (end_time.tv_sec - start_time.tv_sec) * 1000LL + (end_time.tv_nsec - start_time.tv_nsec) / 1000000LL;

    write(1, "Sequential time: ", 17);
    write_int_directly((int)elapsed_ms);
    write(1, " ms\n", 4);

    for (int i = 0; i< m.size; i++){write_int_directly(m.data[i]); write(1, ' ', 1);}

    free(m.data);

    return 0;
}