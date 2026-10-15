#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>


int parse_string_to_int(const char *str, int bytes, int *result_array) {
    int count = 0;
    int i = 0;

    while (i < bytes) {
        while (i < bytes && (str[i] < '0' || str[i] > '9') && str[i] != '-') {
            i++;
        }
        if (i >= bytes) {
            break;
        }

        int is_negative = 0;
        if (str[i] == '-') {
            if (i + 1 < bytes && str[i + 1] >= '0' && str[i + 1] <= '9') {
                is_negative = 1;
                i++;
            } else {
                i++;
                continue;
            }
        }
        int current_number = 0;
        while (i < bytes && str[i] >= '0' && str[i] <= '9') {
            current_number = current_number * 10 + (str[i] - '0');
            i++;
        }
        if (is_negative) {
            current_number = -current_number;
        }
        result_array[count] = current_number;
        count++;
    }
    return count;
}


void reverse(char *str, int len) {
    int start = 0;
    int end = len - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}


int int_to_str(char *dest, int n) {
    int i = 0;

    if (n == 0) {
        dest[i++] = '0';
        dest[i] = '\0';
        return i;
    }

    long long nbr = n; 
    int is_negative = 0;

    if (nbr < 0) {
        is_negative = 1; 
        nbr = nbr * -1; 
    }

    while (nbr > 0) {
        dest[i++] = (nbr % 10) + '0'; 
        nbr /= 10;                    
    }

    if (is_negative) {
        dest[i++] = '-';
    }

    dest[i] = '\0';

    reverse(dest, i);

    return i;
}


int main(int argc, char *argv[]){
    if (argc < 2){exit(5);}

    char buffer[1024];

    ssize_t bytes = read(0, buffer, sizeof(buffer)-1);

    close(0);
    close(4);

    if (bytes == -1){exit(1);}

    if (bytes>0){buffer[bytes] = '\0';}

    int int_massive[512];
    int len_int_massive = parse_string_to_int(buffer, bytes, int_massive);
    int result=0;

    for (int i = 0; i < len_int_massive; i++){
        int temp = int_massive[i];

        if (result != (result + temp) - temp){exit(2);}
        result += temp;
    }

    int file = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0644);

    if (file == -1){
        exit(3);
    }

    char str_result[64];
    int len = int_to_str(str_result, result);
    
    write(file, str_result, len);
    close(file);

    exit(0);
}