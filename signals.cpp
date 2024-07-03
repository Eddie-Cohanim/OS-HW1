#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    std::cout << "smash: got ctrl-C" << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    int foregroundPid = smash.getForeground();
    if (foregroundPid != -1) {

        int result = kill(foregroundPid, sig_num);
        checkSysCall("kill", result);
        std::cout << "smash: process " << foregroundPid << " was killed"<< std::endl;
        smash.setForeground(-1);

    }
}