#include "logger.hpp"
#include "consts.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>
#include <fcntl.h>

using namespace std;

static Logger logger("main.cpp");

void remove_fifo_file(string fifo_file){
    if (remove(fifo_file.c_str()) == -1) {
        logger.log_error("Failed to remove fifo file");
        exit(EXIT_FAILURE);
    }
}

string make_fifo_file() {
    if (mkfifo(FIFO_FILE, 0666) == -1) {
        logger.log_error("Failed to make fifo file");
        exit(EXIT_FAILURE);
    }
    return FIFO_FILE;
}

pid_t run_linear_process(const char *executable, int& write_pipe, int& read_pipe, Logger& logger) {
    int pipe1[2];
    if (pipe(pipe1) == -1) {
        logger.log_error("Failed to make pipe");
        exit(EXIT_FAILURE);
    }
    int pipe2[2];
    if (pipe(pipe2) == -1) {
        logger.log_error("Failed to make pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if (pid < 0) {
        logger.log_error("Failed to fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe2[1]);
        execl(executable, executable, nullptr);
        logger.log_error("Failed to execute");
        exit(EXIT_FAILURE);
    }
    else {
        close(pipe1[0]);
        close(pipe2[1]);
        write_pipe = pipe1[1];
        read_pipe = pipe2[0];
    }
    return pid;
}

pid_t run_voter_process(const char *executable, int& read_pipe, Logger& logger) {
    pid_t pid = fork();
    if (pid < 0) {
        logger.log_error("Failed to fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        dup2(read_pipe, STDIN_FILENO);
        close(read_pipe);
        execl(executable, executable, nullptr);
        logger.log_error("Failed to execute");
        exit(EXIT_FAILURE);
    }
    else {
        close(read_pipe);
    }
    return pid;
}

void wait_for_child(pid_t pid, Logger& logger){
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)){
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0){
            logger.log_error("Child exited with non-zero status");
            exit(EXIT_FAILURE);
        }
    }
    else{
        logger.log_error("Child did not exit normally");
        exit(EXIT_FAILURE);
    }
}

int open_fifo_file(int o_flag) {
    int fd = open(FIFO_FILE, o_flag);
    if (fd == -1){
        logger.log_error("Failed to open fifo file");
        exit(EXIT_FAILURE);
    }
    return fd;
}

string run_ensemble_process(const string validation_path, const string weights_path) {
    int write_pipe, reda_pipe;
    string fifo_file = make_fifo_file();
    pid_t linear_pid = run_linear_process(LINEAR_EXECUTABLE, write_pipe, reda_pipe, logger);
    pid_t voter_pid = run_voter_process(VOTER_EXECUTABLE, reda_pipe, logger);
    string pipe_data = validation_path + " " + weights_path + "\n";
    write(write_pipe, pipe_data.c_str(), pipe_data.size());
    int fifo_fd = open_fifo_file(O_WRONLY);
    write(fifo_fd, validation_path.c_str(), validation_path.size());
    close(fifo_fd);
    
    for (int indx = 0; indx < NUM_OF_CLASSIFIERS; ++indx) {
        pipe_data = to_string(indx) + " ";
        write(write_pipe, pipe_data.c_str(), pipe_data.size());
        fifo_fd = open_fifo_file(O_RDONLY);
        char next[512] = {0};
        read(fifo_fd, next, sizeof(next));
        close(fifo_fd);
        if (string(next) == "next") {
            string done;
            if (indx == NUM_OF_CLASSIFIERS - 1) done = "done";
            else done = "not done";
            fifo_fd = open_fifo_file(O_WRONLY);
            write(fifo_fd, done.c_str(), done.size());
            close(fifo_fd);
        }
    }
    char result[32] = {0};
    fifo_fd = open_fifo_file(O_RDONLY);
    read(fifo_fd, result, sizeof(result));
    close(fifo_fd);
    close(write_pipe);
    wait_for_child(linear_pid, logger);
    wait_for_child(voter_pid, logger);
    remove_fifo_file(fifo_file);
    return string(result);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        logger.log_error("Usage: %s <validation_path> <weights_path>", argv[0]);
        return EXIT_FAILURE;
    }
    string validation_path = argv[1];
    string weights_path = argv[2];
    string result = run_ensemble_process(validation_path, weights_path);
    cout << result << endl;
    return EXIT_SUCCESS;
}