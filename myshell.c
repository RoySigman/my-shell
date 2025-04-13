#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define BUFFER_SIZE 100
#define LOG 0

//TASKS: only send my-shell> after command finishes output (in & flow)
//does not work because not waiting for cmd to finish. solution:
int is_history(char* command){ //prefix check
    return (strncmp(command, "history", 7) == 0);
}

void history(char** command_log, int commands_used){ //create behaviour of numbering
    for(int i = commands_used - 1; i >= 0; i--){
        fprintf(stdout, "%d ", i+1);
        fprintf(stdout, "%s", command_log[i]);
    }
}

int copy_and_trim_char(char* dest, const char* src, char c) {
    if (!dest || !src) return -1;
    size_t len = strlen(src);
    //remove ' '
    while (len > 0 && (src[len - 1] == ' ')) {
        if (src[len - 1] == c) break; // stop if we found c
        len--;
    }
    if (len > 0 && src[len - 1] == c) len--; //remove c if needed
    while (len > 0 && (src[len - 1] == ' ')) len--; //remove more ' '
    
    strncpy(dest, src, len);
    dest[len] = '\0'; 
    return 0;
}

void move_history(char** dest, char** src, int prevSize){
    for(int i = 0; i < prevSize; i++)
        strncpy(dest[i], src[i], BUFFER_SIZE);
    if(LOG) printf("\nmove history called. prevSize was %d\n", prevSize);
}

int main(void)
{
    close(2);
    dup(1);
    
    char old_command[BUFFER_SIZE];
    char command[BUFFER_SIZE] = {0}; 
    int history_size = 100; //realistically, shouldn't change and move_history shouldn't be called
    
    char** command_log = (char**)(malloc(sizeof(char*) * history_size));
    if(command_log == NULL) perror("error"); //malloc failed, unlikely
    
    int commands_used = 0; //actual size of commands log
    int background = 0; //background = 1 <=> cmd ends with &
    
    for(int i = 0; i < history_size; i++){ // init command log
        command_log[i] = (char*)(malloc(BUFFER_SIZE));
        if(command_log[i] == NULL) perror("error"); //malloc failed, unlikely
    }
       
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        fflush(stdout);
        memset(old_command, 0, BUFFER_SIZE);
        fgets(old_command, BUFFER_SIZE, stdin);
        
        if(strncmp(old_command, "exit", 4) == 0) break; //check for exit
        
        if(commands_used < history_size) strncpy(command_log[commands_used++], old_command, BUFFER_SIZE); //log command if possible
        if(commands_used == history_size){
            //reallocate log, didn't use realloc on purpose
            history_size *= 2;
            char** ptr = command_log;
            command_log = (char**)(malloc(sizeof(char*) * history_size));
            if(command_log == NULL) perror("error"); //malloc failed, unlikely
            
            for(int i = 0; i < history_size; i++){ // init command log
                command_log[i] = (char*)(malloc(BUFFER_SIZE));
                if(command_log[i] == NULL) perror("error"); //malloc failed, unlikely
            }
            
            move_history(command_log, ptr, history_size / 2);
            
            for(int i = 0; i < history_size / 2; i++) free(ptr[i]);
            free(ptr);
            
        }
        copy_and_trim_char(command, old_command, '\n');
        background = (command[strlen(command) - 1] == '&');
        copy_and_trim_char(command, command, '&');
        
        if(strlen(command) == 0) continue;
        if(LOG) printf("trimmed command is: \n%s\nand background = %d\n", command, background);
        if(is_history(command)) { //history behaviour
            history(command_log, commands_used); 
            continue;
        }
        
        // *** parse command
        char orig_cmd[BUFFER_SIZE] = {0}; //copy of cmd
        strncpy(orig_cmd, command, BUFFER_SIZE);
        int len = 0;
        
        char* pch;
        while(pch = strtok((len == 0) ? command : NULL, " ")) len++; //amount of args wo null
        
        // *** create argv for execvp
        int argv_size_wo_null = len;
        int argv_size = argv_size_wo_null + 1; //+1 for NULL
        
        char* argv[argv_size];
        
        len = 0;
        strcpy(command, orig_cmd); //restore command from copy
        
        while((pch = strtok((len == 0) ? command : NULL, " ")) && len < argv_size_wo_null){
            argv[len] = (char*)(malloc(strlen(pch) + 1));
            if(argv[len] == NULL) perror("error"); //malloc failed, unlikely
            strcpy(argv[len], pch);
            len++;
        }
        
        
        argv[argv_size - 1] = NULL;
        if(LOG){
            printf("Arguments are:\n");
            for(int i = 0; i < argv_size; i++) printf("%s\n", argv[i]?argv[i]:"null");
        }
        
        // *** execute command
        pid_t pid = fork();
        if(pid < 0) perror("error");
        if(pid > 0) {
            int status = 0;
            if(!background) waitpid(pid, &status, 0);
        }
        if(pid == 0){
            int ret = execvp(argv[0], argv);
            if(ret == -1){
                perror("error");
                exit(EXIT_FAILURE);
            }
        }
        
        for(int i = 0; i < argv_size; i++) free(argv[i]);
        
    }
    
    for(int i = 0; i < history_size; i++) free(command_log[i]);
    free(command_log);
    
    return 0;
}
