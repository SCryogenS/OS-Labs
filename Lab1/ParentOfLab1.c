#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>


int main(){

    int pipe1[2]; 
    int pipe2[2]; 

    pid_t pid;

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0){write(2, "Pipe trouble creating\n", 22); return 1;}

    char filename[256];
    write(1, "Enter filename: ", 16);

    ssize_t fn_bytes = read(0, filename, sizeof(filename)-1);

    if (fn_bytes <= 0){return 1;}
    if (filename[fn_bytes-1] == '\n'){
        filename[fn_bytes-1] = '\0';
    }else{filename[fn_bytes] = '\0';}

    pid = fork();

    if (pid<0){write(2, "Error with fork\n", 16); return 1;}

    if (pid==0){

        dup2(pipe1[0], 0);
        dup2(pipe2[1], 4);

        close(pipe1[0]);
        close(pipe2[1]);
        close(pipe2[0]);
        close(pipe1[1]);
 
        char *argv[] = {"./child", filename, NULL};
        execv(argv[0], argv);

        exit(4);
    }else{
        close(pipe1[0]);
        close(pipe2[1]);

        char buffer[1024];
        ssize_t numbers = read(0, buffer, sizeof(buffer));

        close(pipe2[0]);
        write(pipe1[1], buffer, numbers);
        close(pipe1[1]);

        int status;
        pid_t result = waitpid(pid, &status, 0);

        char mess1[] = "Child is counting\n";
        char mess2[] = "Child is succsefully ended\n";
        char mess3[] = "Child is dead\n";
        char mess4[] = "Child couldn't read the recived text\n";
        char mess5[] = "Child faced with overflow\n";
        char mess6[] = "Child couldn't open or create a file\n";
        char mess7[] = "Execv didn't work\n";
        char mess8[] = "Child didn't get a file name\n";
        
        if (result == 0){write(1, mess1, sizeof(mess1)-1);}
        if (result > 0){
            if (WIFEXITED(status)){
                int ChildStatusWork = WEXITSTATUS(status);
                if (ChildStatusWork == 0){write(1, mess2, sizeof(mess2)-1); return 0;}
                if (ChildStatusWork == 1){write(2, mess4, sizeof(mess4)-1); return 1;}
                if (ChildStatusWork == 2){write(2, mess5, sizeof(mess5)-1); return 1;}
                if (ChildStatusWork == 3){write(2, mess6, sizeof(mess6)-1); return 1;}
                if (ChildStatusWork == 4){write(2, mess7, sizeof(mess7)-1); return 1;}
                if (ChildStatusWork == 5){write(2, mess8, sizeof(mess8)-1); return 1;}
            }
        }
        if (result < 0){write(2, mess3, sizeof(mess3)-1); return 0;}

        return 0;
    }
}