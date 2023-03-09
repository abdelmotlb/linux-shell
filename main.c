#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

char input[100];
int log_file;
typedef struct Command Command;
typedef struct Var Var;


struct Command {
    char* input_type;   // built-in and non-built-in.
    int length;         // top pointer index.
    char* args[10];     // max arguments for a command line.
}cmd;                   // create global struct (obj) instance.


struct Var {
    char identifier[15];
    char value[15];
    Var* nxt;
};

Var* var = NULL;

void on_child_exit() {
    write(log_file, "Child terminated\n", strlen("Child terminated\n"));
    int status;
    waitpid(-1, &status, WNOHANG);
}

void setup_environment()
{

    char path[100];
    getcwd(path, sizeof(path));
    chdir(path);
    log_file = open("log_file.txt", O_CREAT  | O_RDWR,0666);

}

int isEqual(const char* s1, const char* s2){
    return ( !strcmp(s1, s2) );
}

void read_input() {
    scanf("%[^\n]%*c", input);
}

int parse_input() {

    // reset command struct.
    cmd.input_type = NULL;
    for(int i = 0; i < 10; i++) cmd.args[i] = NULL;
    cmd.length = 0;

    // get actual command.
    char* ptr = strtok(input, " "); // char pointer to the first string before " "
    cmd.args[0] = ptr;
    cmd.length++;                           // increment the top pointer

    // if export take only two arguments
    if(strcmp(cmd.args[0], "export") == 0) {

        ptr = strtok(NULL, "\n");
        cmd.args[1] = ptr;                  // remain string.
        cmd.input_type = "execute_shell_bultin";
        // printf("cmd.args[1] > %s", cmd.args[1]);
        return 0;

    }
    else if(strcmp(cmd.args[0], "echo") == 0) {

        while(ptr != NULL) {

            // single token that includes the entire input string if " isn't in the string.
            ptr = strtok(NULL, "\"");
            cmd.args[cmd.length++] = ptr;

        }

    } else {

        while (ptr != NULL) {
            ptr = strtok(NULL, " ");
            cmd.args[cmd.length++] = ptr;
        }

    }

    // specify the type of command >> built-in & non built-in.
    if (strcmp(cmd.args[0], "cd") == 0 || strcmp(cmd.args[0], "echo") == 0 || strcmp(cmd.args[0], "export") == 0) {
        cmd.input_type = "execute_shell_bultin";
    } else {
        cmd.input_type = "executable_or_error";
    }

    // specify the type of running programs >> background & leave.
    if (cmd.args[1] != NULL && strcmp(cmd.args[1], "&") == 0) {
        return 1;
    } else {
        return 0;
    }



}

int command_is_not_exit() {
    return !isEqual(cmd.args[0], "exit");
}

Var* find_var(Var* ins, const char* idn)
{
    if(ins == NULL) {
        return  NULL;
        printf("in null");
    } else if(strcmp(ins->identifier, idn) == 0){
        return ins;
    }else if (ins->nxt == NULL) {
        return ins;
    }
    return find_var(ins->nxt, idn);
}

void substitute(){
    for (int i = 1; i < 10; i++) {
//                printf("i >> %c\n", *(cmd.args[i])); *string means print the first character
        if (cmd.args[i] != NULL && *(cmd.args[i]) == '$') {

            // search for the variable in its struct;
            char *ptr = cmd.args[i] + 1;
            Var *ins = find_var(var, ptr);

            // if the variable not found.
            if (ins == NULL) {
                printf("Invalid expression : No variable '%s'\n", ptr);
            }

                // if the variable found.
            else {

                char* ptr = strtok(ins->value, " ");
                int j = i;
                while (ptr != NULL) {
                    cmd.args[j++] = ptr;
                    ptr = strtok(NULL, " ");
                }
            }
        }
    }
    // end loop.
}

void evaluate_expression(char* exp)
{

    if(exp == NULL) {
        return;
    }
    char* ptr = strtok(exp, "$");
    if(*exp != '$')
        printf("%s", ptr);
    while (ptr != NULL) {
        if((ptr-1 != NULL) && *(ptr-1) == '$') {
            char* ins_ptr = strtok(ptr, " ");
            if(ins_ptr != NULL) {
                ptr = ins_ptr;
            }
        }else {

            char* ins_ptr = strtok(NULL, "$");
            if(ins_ptr != NULL) {
                ptr = ins_ptr;
                ins_ptr = strtok(ins_ptr, " ");
                if(ins_ptr != NULL){
                    ptr = ins_ptr;
                }
            }else {
                if(exp != ptr)
                    printf("%s\n", ptr);
                else
                    printf("\n");

                return;
            }
        }

        Var *ins = find_var(var, ptr);
        if (ins == NULL || strcmp(ins->identifier, ptr) != 0) {
            // no variable
            printf("Invalid expression : No variable '%s'\n", ptr);
            return;
        } else {
            // variable exist
            printf("%s ",ins->value);
            if(ptr != NULL)
                ptr = strtok(NULL, "$");
        }
    }
    printf("\n");
}

void parse_export() {

    // define pointers
    char* var_name = strtok(cmd.args[1], "="); // ended.
    char* value;
    
    if(var_name != NULL){
        value = strtok(NULL, "="); // ended except the quotes.   
    }

    Var* ins = find_var(var, var_name);
    
    // create a new variable.
    
    // first variable:
    if(ins == NULL) {
        
        var = (Var*) malloc(sizeof (Var)); // create struct obj. 
        strcpy(var->identifier, var_name);
        
        if(*value == '\"'){
            
            char* ins_value = strtok(value+1, "\"");
            strcpy(var->value, ins_value);

        }else {
        
            strcpy(var->value, value);

        }

        
        var->nxt = NULL;
    } 
    // existing variable.
    else if(strcmp(ins->identifier, var_name) == 0) {
        strcpy(ins->value, value);
    }
    // add another variable.
    else {
        
        ins->nxt = (Var*) malloc(sizeof (Var));
        ins = ins->nxt;
        strcpy(ins->identifier, var_name);
        
        if(*value == '\"'){
            
            char* ins_value = strtok(value+1, "\"");
            strcpy(ins->value, ins_value);

        }else {
            
            strcpy(ins->value, value);

        }
        ins->nxt = NULL;
    }
    
}

void execute_shell_bultin() {
    if(strcmp(cmd.args[0], "cd") == 0) {

        if( cmd.args[1] == NULL || isEqual( cmd.args[1], "~" ) ){
            chdir( getenv("HOME") );
        }
        else {
            int success = !chdir(cmd.args[1]);
            if( !success ){
                // generate an error due to no success
                printf("error message: the entered path is not valid\n");
            }
        }
        
    }
    else if (strcmp(cmd.args[0], "echo") == 0) {

        // echo command
        evaluate_expression(cmd.args[1]);

    } 
    else if (strcmp(cmd.args[0], "export") == 0) {

        // export command
        parse_export();
        
    }
}

int execute_command(int in_background)
{
    int child_id = fork();
    int isParent = child_id;
    if ( !isParent ) {

        if( cmd.args[1] != NULL && isEqual(cmd.args[1], "&") ){
            cmd.args[1] = NULL;
        }
        int res = execvp(cmd.args[0], cmd.args);
        if(res < 0 && strcmp(cmd.args[0], "exit") != 0){
            printf("Error Message not found\n");
        }
        exit(0);

    }
    // wait until the user exit the opened app.
    else if ( !in_background ) {
        int status;
        waitpid(child_id, &status, 0);
    }
    return 0;
}

void printCurrentPath() {
    char newPath[100];
    getcwd(newPath, sizeof newPath);
    printf("shell: %s >> ", newPath);
}

void shell() {
    do {
        // print for show
        printCurrentPath();

        // read and parse
        read_input();
        int in_background = parse_input();

        // substitute variable by its value in non echo command only at the end of command.
        if(strcmp(cmd.args[0], "echo") != 0) {
            substitute();
        }

        // directing by type created in parsing.
        if (strcmp(cmd.input_type, "execute_shell_bultin") == 0) {
            execute_shell_bultin();
        } else if (strcmp(cmd.input_type, "executable_or_error") == 0){
            execute_command(in_background);
        }

    }while(command_is_not_exit());
}

int main(int argc, char* argv[]) {
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();
    return 0;
}

// important notes:
/*
 * the shell must be at the end because of fork function in which the child execute from fork the end of program.
 */