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

    //TODO: setup sig alarm handler

    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        std::cout << smash.getPrompt() << ">" << " ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        if (cmd_line.empty() == true)
        {
            continue;
        }
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}


/*chprompt
chprompt a b

chprompt b
chprompt
chprompt Jeffry_Epstein_didn’t_kill_himself
quit
*/