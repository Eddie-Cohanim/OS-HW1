#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    std::cout << "fuck you too" << std::endl;
    //TODO: setup sig alarm handler
    int i = 0;
    SmallShell &smash = SmallShell::getInstance();
    while (i < 10) {
        std::cout << smash.getPrompt() << ">" << " ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        std::cout << "line was recieved again..." << std::endl;
        i++;
        //smash.executeCommand(cmd_line.c_str());
        smash.executeCommand(cmd_line);

    }
    return 0;
}