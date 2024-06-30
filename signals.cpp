#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {

std::cout << "smash: got ctrl-C" << std::endl;
SmallShell& smash = SmallShell::getInstance();
int foregroundId= smash.getForeground();
if (foregroundId != -1) {
    int result = kill(jobPid, signal);
    checkSysCall("kill", result);
    std::cout << "smash: process " << foregroundId << " was killed"<< std::endl;
    smash.setForeground(-1);
}


}
