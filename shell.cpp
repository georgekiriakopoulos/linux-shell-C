#include <iostream>
#include <stdio.h>
#include "shell.hpp"


vector<string> normalizeInput(string inputString) {                   //function to create a vector out of each word that was typed as a command by the user 
    vector<string> inputVector; 
    istringstream ss(inputString);                  //splitting inputString around the spaces
    string word, s;
    while (ss >> word)
    {   
        if (word != "<" && word != ">" && word != "|" && word != ">>" && word != "&") {
            s += word + " ";  
            continue;
        } else {
            inputVector.push_back(s);
            inputVector.push_back(word);
            s.clear();
        }
    }
    inputVector.push_back(s); 
    return inputVector; 
}



void traverseCommand(vector<string> inputVector) {
    bool hasPipedCommand = false; 
    bool hasBackgroundCommand = false; 
    for (int i = 0; i < inputVector.size(); i++) {   
        if (inputVector[i] == "<") {                                    //redirect input
            string inputFileStr = inputVector[i+1]; 
            redirectInput(inputFileStr); 
        }
        if (inputVector[i] == ">") {                                    //redirect output
            string outputFileStr = inputVector[i+1];
            redirectOutput(outputFileStr);
        }
        if (inputVector[i] == ">>") {                                   //append output
            string appendFile = inputVector[i+1];           
            appendOutput(appendFile);
        }
        if (inputVector[i] == "|") {                                      //implement a pipe 
            string outputFileStr = inputVector[i-1];
            string inputFileStr = inputVector[i+1];
            hasPipedCommand = true; 
            implementPipe(outputFileStr, inputFileStr);
        }
        if (inputVector[i] == "&") {                                    //background process detected
            hasBackgroundCommand = true; 
            string command = inputVector[i-1];
            executeCommand(inputVector[i-1], true); 
        }
    }
    if (hasPipedCommand == false && hasBackgroundCommand == false) {
        executeCommand(inputVector[0], false);
    }
  }

// char *StringToCharArray(string commandArgs, int &size) {
//     istringstream ss(commandArgs);                  //splitting inputString around the spaces
//     string word; 
//     int total_words = 0;
//     for (int i = 0; i < commandArgs.size(); i++) {
//         if (commandArgs[i] == ' ') {                    //if we encounter a space in the command given then it means we have a new word
//             total_words++;
//         }
//     }
  
//     char *inputArgs[total_words + 1]; 
//     int j = 0;
//     while (ss >> word) {
//         inputArgs[j] = strdup(word.c_str());
//         j++; 
//     }

//     inputArgs[total_words] = NULL;    
//     size = total_words + 1; 
//     return inputArgs[total_words+1];
// }

void executeCommand(string commandArgs, bool background_process) {
    string word; 
    bool foundSingleQuote = false; 
    glob_t gstruct; 
    string terminal = "/dev/tty";

    int total_words = countWordsInCommand(commandArgs); 

    istringstream ss(commandArgs);                  //splitting inputString around the spaces

    char *inputArgs[total_words + 1]; 
    int j = 0;
    while (ss >> word) {
        size_t foundWildcard = word.find_first_of("*?[]");          //searching for a wildcard inside of string
        if (foundWildcard != string::npos) {
            glob(word.c_str(), 0, NULL, &gstruct);                  
            for (int i = 0; i < gstruct.gl_pathc; i++) {            //passing the wildcard's pathname to my input arguments array so execvp() can recognize it
                inputArgs[j] = gstruct.gl_pathv[i];
            }

        } else {
            inputArgs[j] = strdup(word.c_str());
        }
        j++; 
    }

    inputArgs[total_words] = NULL;              
    const char *command = inputArgs[0];
    //char *inputArgTest = StringToCharArray(commandArgs, size); 


    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        exit(1);
    }else if (pid == 0) {
        //child process so exec
        int status_code = execvp(command, inputArgs);
        if (status_code == -1) {
            perror("Exec on child did not work!");    
        }
        exit(1);
    } else {
        //parent process so wait 
        if (background_process == false) {
            int status; 
            waitpid(pid, &status, 0);
            redirectInput("/dev/tty");              //when child process is done, redirect input and output back to the terminal 
            redirectOutput("/dev/tty");  
        }
    }


}

int redirectInput(string inputFileStr) {
    char* char_array = new char[inputFileStr.length() + 1];        //converting the string to a char * array so it can be recognized by the open()
    if (inputFileStr == "/dev/tty") {                              //redirect back to terminal 
        strcpy(char_array, inputFileStr.c_str()); 
    }else {                                                     //converting the string to a char * array so it can be recognized by the open()
        for (int j = 0; j < inputFileStr.size() - 1; j++){
            char_array[j] = inputFileStr[j];
        }
    }

    int input_fd = open(char_array, O_CREAT | O_RDONLY);
    if (input_fd == -1) {
      perror("Error at input\n");
    }
    int dup2_status = dup2(input_fd, STDIN_FILENO);
    if (dup2_status == -1 ){
        perror("error at dup2 input");
    }
    close(input_fd); 
    return input_fd;
}

int redirectOutput(string outputFileStr) { 
    char* char_array = new char[outputFileStr.length() + 1];        //converting the string to a char * array so it can be recognized by the open()
    if (outputFileStr == "/dev/tty") {                              //redirect back to terminal 
        strcpy(char_array, outputFileStr.c_str()); 
    }else {                                                     //converting the string to a char * array so it can be recognized by the open()
        for (int j = 0; j < outputFileStr.size() - 1; j++){
            char_array[j] = outputFileStr[j];
        }
    }

    int output_fd = open(char_array, O_CREAT | O_TRUNC | O_WRONLY , 0666); 
    if (output_fd == -1) {
        perror("Error at output\n");
    }
    int dup2_status = dup2(output_fd, STDOUT_FILENO);
    if (dup2_status == -1) {
        perror("error at dup2 output");
    }
    close(output_fd); 
    delete[] char_array;
    return output_fd;
}

int appendOutput(string appendFileStr) {
    char* char_array = new char[appendFileStr.length() + 1];        //converting the string to a char * array so it can be recognized by the open()
    if (appendFileStr == "/dev/tty") {                              //redirect back to terminal 
        strcpy(char_array, appendFileStr.c_str()); 
    }else {                                                     //converting the string to a char * array so it can be recognized by the open()
        for (int j = 0; j < appendFileStr.size() - 1; j++){
            char_array[j] = appendFileStr[j];
        }
    }

    int output_fd = open(char_array, O_CREAT | O_WRONLY | O_APPEND, 0666); 
    if (output_fd == -1) {
        perror("Error at output\n");
    }
    int dup2_status = dup2(output_fd, STDOUT_FILENO);
    if (dup2_status == -1 ) {
        perror("error at dup2 output");
    }
    close(output_fd); 
    return output_fd;
}

void implementPipe(string outputCommand, string inputCommand) {
    int fd[2];
    pipe(fd); 
    dup2(fd[WRITE], STDOUT_FILENO);
    close(fd[WRITE]);
    
    executeCommand(outputCommand, false);

    dup2(fd[READ], STDIN_FILENO);
    close(fd[READ]);

    executeCommand(inputCommand, false); 
}

int countWordsInCommand(string commandArgs) {           //counting the words in the command that was typed by the user so that we can create an array of argvs and call exec
    int total_words = 0; 
    for (int i = 0; i < commandArgs.size(); i++) {
        if (commandArgs[i] == ' ') {                    //if we encounter a space in the command given then it means we have a new word
            total_words++;
        }
    }
    return total_words;
}