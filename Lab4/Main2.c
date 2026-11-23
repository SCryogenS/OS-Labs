#define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <dlfcn.h>

int from_str_to_int(int *result, char *word){
    *result = 0;
    int ptr = 0;
    if (word[ptr]=='+'){ptr++;}
    if (word[ptr]=='\0'){return 1;}

    while (word[ptr]!='\0'){
        if (!(word[ptr] >= '0' && word[ptr] <= '9')){return 1;}

        int num = word[ptr] - '0';
        if (*result > (INT_MAX - num)/10){return 2;}
        *result = *result * 10 + num;

        ptr++;
    }

    return 0;
}

int read_word(char *word, int size){
    int ptr = 0;
    unsigned char sym;
    ssize_t bytes;

    while ((bytes = read(0, &sym, 1)) == 1){
        if (isspace(sym)){
            if (ptr > 0){break;}
        }else{
            if (ptr == size-1 || sym == '\0'){return -1;}
            word[ptr++] = sym;
        }
    }

    if (bytes < 0){return -1;}
    word[ptr] = '\0';
    return ptr;
}

int int_to_str(char *dest, int n){
    int len = 0;
    do {
        dest[len++] = (n % 10) + '0';
        n /= 10;
    } while (n > 0);

    for (int i = 0; i < len/2; i++){
        char temp = dest[i];
        dest[i] = dest[len-i-1];
        dest[len-i-1] = temp;
    }

    return len;
}

int main(){
    char mess1[] = "Command: ";
    char mess2[] = "A for dividers: ";
    char mess3[] = "B for dividers: ";
    char mess4[] = "X: ";
    char mess5[] = "Invalid integer input.\n";
    char mess6[] = "Use command 0, 1 or 2.\n";
    char mess7[] = "Function has no address.\n";
    char mess8[] = "Library 1\n";
    char mess9[] = "Library 2\n";

    const char *paths[] = {"./libCryoLib.so", "./libCryoLib2.so"};
    const char *gcd_names[] = {"Evklid_gcd", "fool_gcd"};
    const char *e_names[] = {"lim_e", "row_e"};
    int current = 0;
    void *handle = dlopen(paths[0], RTLD_NOW);
    if (handle == NULL){
        char *error = dlerror();
        if (write(2, error, strlen(error)) < 0){return 1;}
        return write(2, "\n", 1) < 0 ? 2 : 1;
    }

    int (*gcd)(int, int) = (int (*)(int, int))dlsym(handle, gcd_names[0]);
    float (*e)(int) = (float (*)(int))dlsym(handle, e_names[0]);
    if (gcd == NULL || e == NULL){
        dlclose(handle);
        return write(2, mess7, sizeof(mess7)-1) < 0 ? 2 : 1;
    }

    char command[2];
    int status = 1;
    while (1){
        if (write(1, mess1, sizeof(mess1)-1) < 0){break;}
        int len = read_word(command, sizeof(command));
        if (len == 0){status = 0; break;}
        if (len < 0){status = 2; break;}

        char result[32];
        if (command[0]=='0'){
            void *new_handle = dlopen(paths[1-current], RTLD_NOW);
            if (new_handle == NULL){
                char *error = dlerror();
                if (write(2, error, strlen(error)) < 0 || write(2, "\n", 1) < 0){break;}
                continue;
            }

            int (*new_gcd)(int, int) = (int (*)(int, int))dlsym(new_handle, gcd_names[1-current]);
            float (*new_e)(int) = (float (*)(int))dlsym(new_handle, e_names[1-current]);
            if (new_gcd == NULL || new_e == NULL){
                dlclose(new_handle);
                if (write(2, mess7, sizeof(mess7)-1) < 0){break;}
                continue;
            }

            dlclose(handle);
            handle = new_handle;
            gcd = new_gcd;
            e = new_e;
            current = 1-current;

            if (write(1, current == 0 ? mess8 : mess9, sizeof(mess8)-1) < 0){break;}
            continue;
        }else if (command[0]=='1'){
            char buffer1[32];
            char buffer2[32];
            int a;
            int b;

            if (write(1, mess2, sizeof(mess2)-1) < 0){break;}
            if (read_word(buffer1, sizeof(buffer1)) <= 0 || from_str_to_int(&a, buffer1) != 0 || a <= 0){status = 2; break;}
            if (write(1, mess3, sizeof(mess3)-1) < 0){break;}
            if (read_word(buffer2, sizeof(buffer2)) <= 0 || from_str_to_int(&b, buffer2) != 0 || b <= 0){status = 2; break;}

            len = int_to_str(result, gcd(a, b));
        }else if (command[0]=='2'){
            char buffer1[32];
            int x;

            if (write(1, mess4, sizeof(mess4)-1) < 0){break;}
            if (read_word(buffer1, sizeof(buffer1)) <= 0 || from_str_to_int(&x, buffer1) != 0 || x <= 0){status = 2; break;}

            len = strfromf(result, sizeof(result), "%.9g", e(x));
        }else{
            if (write(2, mess6, sizeof(mess6)-1) < 0){break;}
            continue;
        }

        if (len < 0 || (size_t)len >= sizeof(result)){break;}
        result[len++] = '\n';
        if (write(1, result, len) != len){break;}
    }

    dlclose(handle);
    if (status == 2){return write(2, mess5, sizeof(mess5)-1) < 0 ? 2 : 1;}
    return status;
}
