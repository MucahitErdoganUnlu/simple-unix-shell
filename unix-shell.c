#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include<sys/wait.h>
#include <ctype.h>
#include <time.h>

#define clear() printf("\033[H\033[J")
#define MAX_BUF 512
#define ROWS 100
#define COLS 100

int take_input(char *);
int read_dir(char*, char *);
int find_path(char *, char *);
void execute_cmd(char*, char *[]);
int tokenize(char *);
void freePointerArray(char **);
int allocate_tokens_array();
void deallocate_tokens_array();
void execute_bello(const char *,const  char *,const  char *,const  char *,const  char *, char *);
void inverse_redirection_bello(const char *,const  char *,const  char *,const  char *,const  char *, char *);
void inverse_redirection_cmd(char*, char *[]);
void redirection_execute_bello();
void redirection_execute(char*, char *[]);
int get_number_of_children();
void reset_values();


int tokenCount = 0;
char **tokens = NULL;
int greaterCount = 0;
int redirection_index = 0;
char last_executed_cmd[MAX_BUF];
int isBackground = 0;
int children_offset = 0;
void reset_values(){
    deallocate_tokens_array();
    tokenCount = 0;
    redirection_index = 0;
    greaterCount = 0;
    isBackground = 0;
}
int main() {
    clear();
    printf("------------------------------------------------------------\n");
    printf("                    myshell has started\n");
    printf("------------------------------------------------------------\n");
    children_offset = get_number_of_children();

    char input[MAX_BUF];
    char target[MAX_BUF];
    while(1){

        take_input(input);
        int status;
        waitpid(-1, &status, WNOHANG);
        if(!tokenize(input)){
            printf("wrong input");
            reset_values();
            continue;
        }
        if(strcmp(tokens[0],"") == 0){
            reset_values();
            continue;
        }

        // Open a file in read mode
        FILE *file_read = fopen(".myshell_bash", "r");
        if (file_read == NULL) {
            file_read = fopen(".myshell_bash", "w");
            fclose(file_read);
            file_read = fopen(".myshell_bash", "r");
        }

        char line[MAX_BUF];  // A buffer to store each line read from the file
        char *alias_input[MAX_BUF];
        // Read lines from the file until there are no more lines
        int alias_line = 0;
        int line_count = 0;
        int isAlias = 0;
        while (fgets(line, sizeof(line), file_read) != NULL) {
            int j=0;
            char *tmp = line;
            alias_input[j++] = strsep(&tmp," "); // skip the word "alias"
            alias_input[j++] = strsep(&tmp," ="); // skip the variable
            alias_input[j++] = strsep(&tmp," "); // skip the equal sign
            alias_input[j++] = strsep(&tmp," \n"); // skip the command

            if(!strcmp(alias_input[1], tokens[0])){
                alias_line = line_count;
                isAlias = 1;
            }
            line_count++;
        }
        // Close the file
        fclose(file_read);

        if(isAlias){
            file_read = fopen(".myshell_bash", "r");
            if (file_read == NULL) {
                perror("Error opening file");
                return 1;
            }
            while (alias_line > -1) {
                fgets(line, sizeof(line), file_read);
                alias_line--;
            }
            int j=0;
            char *tmp = line;

            alias_input[j++] = strsep(&tmp," "); // skip the word "alias"
            alias_input[j++] = strsep(&tmp," ="); // skip the variable
            alias_input[j++] = strsep(&tmp," "); // skip the equal sign
            alias_input[j++] = strsep(&tmp," \n"); // skip the command
            strcpy(tokens[0], alias_input[3]);  // replace the alias with the command

            while((alias_input[j] = strsep(&tmp," \n") )!= NULL && alias_input[j][0] != 0){ // get the arguments
                strcpy(tokens[tokenCount++], alias_input[j]);
                j++;
            }
            // Close the file
            fclose(file_read);
        }

        if(strcmp(tokens[0],"bello") == 0){

            redirection_execute_bello();
            strcpy(last_executed_cmd, input);
            reset_values();
            continue;
        }

        if(!strcmp(tokens[0], "alias")){
            // Open a file in append mode
            FILE *file = fopen(".myshell_bash", "a");
            if (file == NULL) {
                perror("Error opening file");
                reset_values();
                continue;
            }

            for(int i=0; i<tokenCount-1; i++){
                fprintf(file, "%s ", tokens[i]);
            }
            fprintf(file, "%s", tokens[tokenCount-1]);
            fprintf(file, "\n");
            // Close the file
            fclose(file);
            reset_values();
            continue;
        }

        if(strcmp(tokens[0],"exit") == 0){
            if(strcmp(tokens[1],"") != 0){
                printf("Invalid argument in exit\n");
                reset_values();
                continue;
            }
            exit(0);
        }


        if(!find_path(tokens[0], target)){
            printf("command not found: %s\n", tokens[0]);
            reset_values();
            continue;
        }
        strcat(target, "/");
        strcat(target, tokens[0]);
        strcpy(tokens[0], target);

        char **cmd = (char **)malloc(MAX_BUF * sizeof(char *));

        for(int i=0; i<tokenCount && (tokens[i][0] != '>' || !greaterCount); i++){
            cmd[i] = (char *)malloc((MAX_BUF + 1) * sizeof(char));
            // Copy the row from the 2D array to the pointer
            strcpy(cmd[i], tokens[i]);
        }


        // Set the last element to a NULL pointer
        cmd[tokenCount] = NULL;

        redirection_execute(tokens[0], cmd);
        strcpy(last_executed_cmd, input);
        freePointerArray(cmd);
        reset_values();
    }

}


void redirection_execute(char* path, char *cmd[]){
    if(!greaterCount){
        execute_cmd(path,cmd);
        return;
    }

    switch (greaterCount) {
        case 1:
            // Redirect stdout to a file named "output.txt" in write mode
            freopen(tokens[redirection_index], "w", stdout);
            // Now, anything that would be printed to stdout will be appended to "output.txt"
            execute_cmd(path, cmd);
            // Close the file stream
            fclose(stdout);
            // Reopen stdout to the console
            freopen(ttyname(0), "a", stdout);
            break;
        case 2:
            // Redirect stdout to a file named "output.txt" in write mode
            freopen(tokens[redirection_index], "a", stdout);
            // Now, anything that would be printed to stdout will be appended to "output.txt"
            execute_cmd(path, cmd);
            // Close the file stream
            fclose(stdout);
            // Reopen stdout to the console
            freopen(ttyname(0), "a", stdout);
            break;
        case 3:
            inverse_redirection_cmd(path, cmd);
            break;
        default:
            printf("wrong case in redirection\n");
            break;
    }

}


int allocate_tokens_array(){
    // Allocate memory for the array of pointers
    tokens = (char **)malloc(ROWS * sizeof(char *));

    if (tokens == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    // Allocate memory for each row
    for (int i = 0; i < ROWS; i++) {
        tokens[i] = (char *)malloc(COLS * sizeof(char));

        if (tokens[i] == NULL) {
            fprintf(stderr, "Memory allocation failed for row %d.\n", i);
            // You might need to free the allocated memory before returning
            return 1;
        }
    }
    return 0;
}
void deallocate_tokens_array(){
    // Deallocate memory when done
    for (int i = 0; i < ROWS; i++) {
        if (tokens[i] != NULL && tokens[i][0] != 0) {
            break;
        }
        free(tokens[i]);
    }
    free(tokens);
}
void freePointerArray(char **ptrArray) {
    for (int i = 0; i < MAX_BUF; i++) {
        if (ptrArray[i] != NULL && ptrArray[i][0] != 0) {
            break;
        }
        free(ptrArray[i]);
    }
    free(ptrArray);
}

int tokenize(char *input){
    allocate_tokens_array();
    int isEcho = 0;
    int count = 0;
    const char special_chars_array[] = "./_=";
    while(input[count] != 10 && input[count] != 0){
        int index = 0;
        if(isspace(input[count]) && input[count] != 10) {
            while (isspace(input[count]) && input[count] != 10) {
                count++;
            }
            continue;
        }
        else if(input[count] == '>' && greaterCount  == 0){
//            int greaterCount = 0;
            while(input[count] == '>' && greaterCount < 3) {
                tokens[tokenCount][index] = '>';
                greaterCount++;
                index++;
                count++;
            }
            redirection_index = tokenCount + 1;
        } else if(input[count] == '"'){
            count++;
            if(!strcmp(tokens[0], "alias")){
                while(input[count] != 10 && input[count] != 0){
                    if(input[count] != '\'' && input[count] != '"'){
                        tokens[tokenCount][index] = input[count];
                        index++;
                    }
                    count++;
                }
            }else{
                while(input[count] != '"'){
                    tokens[tokenCount][index] = input[count];
                    index++;
                    count++;
                }
                count++;
            }

        }else if(input[count] == '\''){
            count++;
            if(!strcmp(tokens[0], "alias")){
                while(input[count] != 10 && input[count] != 0){
                    if(input[count] != '\'' && input[count] != '"'){
                        tokens[tokenCount][index] = input[count];
                        index++;
                    }
                    count++;
                }
            }else{
                while(input[count] != '\''){
                    tokens[tokenCount][index] = input[count];
                    index++;
                    count++;
                }
                count++;
            }
        }
        else if(input[count] == '&'){
            // do not put it in the tokens list
            count++;
            isBackground = 1;
            continue;
        }
        else if(!isspace(input[count]) && input[count] != 0){
            while(!isspace(input[count]) && input[count] != 0 && input[count] != '\'' && input[count] != '"'){
                tokens[tokenCount][index] = input[count];
                index++;
                count++;
            }

        }
        else{
            printf("invalid argument: %c %d\n", input[count], input[count]);
            return 0;
        }
        tokens[tokenCount][index] = '\0';
        tokenCount++;
    }
    tokens[tokenCount][0] = '\0';
    return 1;
}

void execute_cmd(char* path, char *cmd[]){
    pid_t pid = fork();

    if (pid == -1) {
        printf("\nFailed forking child..");
    } else if (pid == 0) {

        if (execvp(path, cmd) < 0) {

            printf("\nCould not execute command..");
        }
        exit(0);
    } else {
        // waiting for child to terminate
        if(!isBackground){
            wait(NULL);
        }else{

            printf("pid: %d\n", (int) pid);
        }

    }
}

void redirection_execute_bello(){
    const char *username = getenv("USER");
    char cwd[MAX_BUF];
    char hostname[MAX_BUF];
    {
        gethostname(hostname, sizeof(hostname));
    }
    const char *tty = ttyname(0);
    const char* shell_name = getenv("SHELL");
    const char* home_directory = getenv("HOME");
    // Get the current time
    time_t currentTime;
    time(&currentTime);

    // Convert the current time to a local time structure
    struct tm* localTime = localtime(&currentTime);
    char *string_time = asctime(localTime);

    if(!greaterCount){
        execute_bello(username, hostname, tty, shell_name, home_directory, string_time);
        return;
    }
    switch (greaterCount) {
        case 1:
            // Redirect stdout to a file named "output.txt" in write mode
            freopen(tokens[redirection_index], "w", stdout);
            // Now, anything that would be printed to stdout will be appended to "output.txt"
            execute_bello(username, hostname, tty, shell_name, home_directory, string_time);
            // Close the file stream
            fclose(stdout);
            // Reopen stdout to the console
            freopen(ttyname(0), "a", stdout);
            break;
        case 2:
            // Redirect stdout to a file named "output.txt" in write mode
            freopen(tokens[redirection_index], "a", stdout);
            // Now, anything that would be printed to stdout will be appended to "output.txt"
            execute_bello(username, hostname, tty, shell_name, home_directory, string_time);
            // Close the file stream
            fclose(stdout);
            // Reopen stdout to the console
            freopen(ttyname(0), "a", stdout);
            break;
        case 3:
            // Redirect stdout to a file named "output.txt" in write mode
            inverse_redirection_bello(username, hostname, tty, shell_name, home_directory, string_time);
            break;
        default:
            printf("wrong input in bello redirection\n");
            break;
    }

}

void execute_bello(const char *username, const char *hostname, const char *tty, const char *shell_name, const char *home_directory, char *string_time){


    printf("Username: %s \n"
           "Hostname: %s \n"
           "Last Executed Command: %s\n"
           "TTY: %s \n"
           "Current Shell Name: %s \n"
           "Home Location: %s \n"
           "Current Time and Date: %s"  // asctime(localTime) has the newline character
           "Current number of proccesses being executed: %d\n",
           username, hostname, last_executed_cmd, tty, shell_name, home_directory, string_time, get_number_of_children() - children_offset + 1);
                                                                                                // add 1 for myshell
}

int get_number_of_children(){
    int value = 0;
    pid_t pid;
    int fd[2];
    int fd_wc[2];

    pipe(fd);
    pipe(fd_wc);
    pid = fork();

    if(pid==0)
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        close(fd_wc[0]);
        close(fd_wc[1]);
        char path[MAX_BUF];
        find_path("ps", path);
        strcat(path, "/ps");
        char *cmd[] = {path, NULL};
        execute_cmd(path, cmd);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        pid=fork();

        if(pid==0)
        {
            dup2(fd[0], STDIN_FILENO);
            dup2(fd_wc[1], STDOUT_FILENO);
            close(fd_wc[0]);
            close(fd_wc[1]);
            close(fd[1]);
            close(fd[0]);
            char temp[] = {'w', 'c', 0};
            char path[MAX_BUF];
            find_path("wc", path);
            strcat(path, "/wc");
            char *cmd[] = {path,"-l", NULL};
            execute_cmd(path, cmd);
            exit(1);
        }
        else
        {
            int old_stdin = dup(STDIN_FILENO);
            dup2(fd_wc[0], STDIN_FILENO);
            close(fd_wc[0]);
            close(fd_wc[1]);
            close(fd[0]);
            close(fd[1]);
            char buf[10];
            read(STDIN_FILENO, buf, 10);
            value = atoi(buf);
            dup2(old_stdin, STDIN_FILENO);
            close(old_stdin);       // Probably correct
            waitpid(pid, &status, 0);
        }
    }
    return value;
}

void inverse_redirection_cmd(char* path, char *cmd[]) {
    pid_t pid;
    int fd[2];

    pipe(fd);
    pid = fork();

    if(pid==0)
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execute_cmd(path, cmd);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        pid=fork();

        if(pid==0)
        {
            dup2(fd[0], STDIN_FILENO);
            close(fd[1]);
            close(fd[0]);
            char buf[1000];
            // fgets(buf, 1000, stdin);
            read(STDIN_FILENO, buf, 1000);

            // Redirect stdout to a file named "output.txt" in write mode
            freopen(tokens[redirection_index], "a", stdout);
            // Now, anything that would be printed to stdout will be appended to "output.txt"
            for(int i= ((int) strlen(buf)) -2; i> -1; i--){
                if(buf[i] != 0)
                    printf("%c", buf[i]);
            }
            printf("\n");
            // Close the file stream
            fclose(stdout);
            // Reopen stdout to the console
            freopen(ttyname(0), "a", stdout);
            exit(0);
        }
        else
        {
            close(fd[0]);
            close(fd[1]);
            waitpid(pid, &status, 0);
        }
    }
}

void inverse_redirection_bello(const char *username, const char *hostname, const char *tty, const char *shell_name, const char *home_directory, char *string_time) {
    pid_t pid;
    int fd[2];

    pipe(fd);
    pid = fork();

    if(pid==0)
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execute_bello(username, hostname, tty, shell_name, home_directory, string_time);
        exit(0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        pid=fork();

        if(pid==0)
        {
            dup2(fd[0], STDIN_FILENO);
            close(fd[1]);
            close(fd[0]);
            char buf[1000];
            // fgets(buf, 1000, stdin);
            read(STDIN_FILENO, buf, 1000);

            // Redirect stdout to a file named "output.txt" in write mode
            freopen(tokens[redirection_index], "a", stdout);
            // Now, anything that would be printed to stdout will be appended to "output.txt"

            for(int i= ((int) strlen(buf)) -2; i> -1; i--){
                if(buf[i] != 0)
                    printf("%c", buf[i]);
            }
             printf("\n");


            // Close the file stream
            fclose(stdout);
            // Reopen stdout to the console
            freopen(ttyname(0), "a", stdout);
            exit(0);
        }
        else
        {
            close(fd[0]);
            close(fd[1]);
            waitpid(pid, &status, 0);
        }
    }
}

int find_path(char * cmd, char * target){
    char path_array[MAX_BUF*8];
    strcpy(path_array, getenv("PATH"));
    char *path = path_array;
    int path_length = (int) strlen(path);
    int count = 0;
    char *parsed[path_length/5] ; // assume path_length/5
    for(int i=0; i<path_length/5; i++){
        parsed[i] = strsep(&path,":");
        if(parsed[i] == NULL){
            parsed[i] = "\0";
            break;
        }
        if(strlen(parsed[i]) == 0){
            i--;
            continue;
        } else{
            count++;
        }
    }
    path_length = count;
    int isFound = 0;
    for(int i=0; i<path_length; i++){
        if(read_dir(parsed[i], cmd)){
            for(int j=0; j<(int)strlen(parsed[i]); j++){
                target[j] = parsed[i][j];
            }
            for(int j=(int)strlen(parsed[i]); j<MAX_BUF; j++){
                target[j] = '\0';
            }

            isFound = 1;
            break;
        }
    }
    return isFound;
}

int read_dir(char *path, char *cmd) {
    DIR* dir = opendir(path);
    struct dirent *ent;
    struct stat states;

    if (dir != NULL) {

        // Read directory entries
        while ((ent = readdir(dir)) != NULL) {

            // Get information about the entry
            stat(ent->d_name, &states);

            // Process the entry or print information
            if(!strcmp(ent->d_name, cmd)){
                return 1;
            }
        }


    } else {
//        perror("Unable to open directory");
    }
    closedir(dir);
    return 0;
}

int take_input(char *input){
    const char *name = getenv("USER");
    char cwd[MAX_BUF];
    char hostname[MAX_BUF];
    {
        gethostname(hostname, sizeof(hostname));
        getcwd(cwd, sizeof(cwd));
        printf("%s@%s %s --- ", name, hostname, cwd);
    }
    char buf[MAX_BUF];
    fgets(buf, MAX_BUF, stdin);

    if(strlen(buf) == 0){
        return 0;
    }
    for(int i=0; i<strlen(buf)-1; i++){
        input[i] = buf[i];
    }
    for(int i= (int) strlen(buf)-1; i<MAX_BUF; i++){
        input[i] = '\0';
    }
    return 1;
}
