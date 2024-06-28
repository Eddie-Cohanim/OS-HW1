#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";


#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) 
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) 
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) 
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) 
{
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) 
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) 
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

///........................SHELL IMPLEMENTATION........................///

void SmallShell::setForground(int forground_id)
{
  m_forground_id = fg_id;
}

int SmallShell::getForground()
{
  return m_forground_id;
}

void SmallShell::setPrompt(const Char* new_prompt)
{
  m_prompt = std::string(new_prompt);
}

std::string SmallShell::getPrompt()
{
  return m_prompt;
}

void SmallShell::setLastPwd(const Char* lastPwd)
{
  m_last_pwd = lastPwd;
}

std::string SmallShell::getLastPwd()
{
  return m_last_pwd;
}

SmallShell::SmallShell() 
{
  m_prompt = "smash";
  m_last_pwd = std::string(); 
  m_forground_id = -1;
}

SmallShell::~SmallShell()
{
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) 
{
  string command = _trim(string(cmd_line));
  string firstWord = command.substr(0, cmd_s.find_first_of(" \n"));

  if (cmd_s.find_first_of(">") != std::string::npos)
  {
      return new RedirectionCommand(cmd_line);
  }
  else if (cmd_s.find_first_of(">>") != std::string::npos)
  {
      return new RedirectionCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line,  &this->m_jobs);
  }
  else if (firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(cmd_line, &this->m_jobs);
  }
  else if (firstWord.compare("quit") == 0)
  {
    return new QuitCommand(cmd_line, &this->m_jobs);
  }
  else if (firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line, &this->m_jobs);
  }
  else if (firstWord.compare("alias") == 0)
  {
    return new aliasCommand(cmd_line);
  }
  else if (firstWord.compare("unalias") == 0)
  {
    return new unaliasCommand(cmd_line);
  }  
  else
  {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line)
{
  Command* command = CreateCommand(cmd_line);
  command->execute();
  delete command.
}


///.......................JOBS LIST IMPLEMENTATION.....................///

JobsList::JobEntry::JobEntry(int id, int pid, char* name) : m_jobId(id), m_jobPid(pid)
{
  m_jobName = std::string(name);
}

void JobsList::addJob(int pid, char* name)
{
  removeFinishedJobs();
  int id = 1;
  if(m_listOfJobs.empty == false)
  {
    id = m_listOfJobs.back().m_jobId;
  }

  JobEntry newJob = JobEntry(id, pid, name);
  m_listOfJobs.push_back(newJob);
}

void JobsList::removeFinishedJobs()
{
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end();)
  {
    int status;
    int result = waitpid((*it).m_listOfJobs, &status, WNOHANG);
    checkSysCall("waitpid", result);
    if (result != 0)
    {
      it = m_job_list.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void JobsList::killAllJobs()
{
  removeFinishedJobs();
  std::cout << "smash: sending SIGKILL signal to " << std::to_string(m_job_list.size()) << " jobs:" << std::endl;//not sure this is neccesary
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end();)
  {
    std::cout << std::to_string(job.m_jobPid) << ": " << job.m_job_name << std::endl;//ditto
    checkSysCall("kill", kill(*it->m_jobPid, SIGKILL));
  }
}

void JobsList::








///.....................COMMAND IMPLEMENTATION.........................///


Command:: Command(const char* cmd_line) 
{
  m_background = _isBackgroundComamnd(cmd_line);
  m_cmd_line = new char[COMMAND_ARGS_MAX_LENGTH + 1];
  m_cmd_line_with_background = new char[COMMAND_ARGS_MAX_LENGTH + 1];
  strcpy(m_cmd_line, cmd_line);
  strcpy(m_cmd_line_with_background, cmd_line);
  _removeBackgroundSign(m_cmd_line);
  m_arg_count = _parseCommandLine(m_cmd_line, m_arg_values);
}

Command::~Command()
{
    for (int i = 0; i < m_arg_count; i++)
    {
        delete[] m_arg_values[i];
    }

    delete[] m_cmd_line_with_background;
    delete[] m_cmd_line;
}


///.................BUILT IN COMMAND IMPELEMENTATIONS..................///


void ChpromptCommand::execute()
{
  if (m_arg_count == 1)
  {
    SmallShell::getInstance().setPrompt("smash");
  }
  else{
    SmallShell::getInstance().setPrompt(m_arg_values[1]);
  }
}

void ShowPidCommand:: execute()
{
  pid_t pid = getpid();
  std::string pidStr = std::to_string(pid);
  cout << "smash pid is " << pidStr << endl;
}

void GetCurrDirCommand:: execute()
{
  char* currDir = new char[ADRRESS_MAX_LENGTH];
  checkSysCall("pwd:", getcwd(currDir, sizeof(char) * ADRRESS_MAX_LENGTH)); //?????????
  std::cout << currDir << std::endl;
  delete currDir[];

}

void ChangeDirCommand:: execute()
{
  if(m_arg_count > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }

  if(m_arg_values[1] == "-")
  {
    if(SmallShell::getInstance().getLastPwd() == nullptr)
    {
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      return;
    }
    char* lastDir = new char[ADRRESS_MAX_LENGTH];
    checkSysCallPtr("cd:", getcwd(lastDir, sizeof(char) * ADRRESS_MAX_LENGTH));
    int result = chdir(SmallShell::getInstance().getLastPwd());
    checkSysCall("cd:", result);
    if(result != -1)
    {
      SmallShell::getInstance().setLastPwd(lastDir);
    }
  }
  
  char* lastDir = new char[ADRRESS_MAX_LENGTH];
  checkSysCallPtr("cd:", getcwd(lastDir, sizeof(char) * ADRRESS_MAX_LENGTH));
  int result = chdir(m_arg_values[1]);
  checkSysCall("cd:", result);
  if(result != -1)
  {
    SmallShell::getInstance().setLastPwd(lastDir);
  }
}

void JobsCommand:: execute()
{
  SmallShell::getInstance().m_jobs.removeFinishedJobs();
  SmallShell::getInstance().m_jobs.printJobsList();
}




void ForegroundCommand :: execute()
{
  if(m_arg_values[2] )
}





///................HELPER FUNCTION IMPELEMENTATIONS..................///


void checkSysCallPtr(char* sysCall, char* currDirPtr)
{
  if (currDirPtr == nullptr)
  {
    std::string tmp = "smash error: "; 
    tmp += sysCall;                  
    tmp += " failed";                  
    perror(tmp.c_str()); //???????????
  }
}

void checkSysCall(char* sysCall, int currDir)
{
  if (currDir == -1)
  {
    std::string tmp = "smash error: "; 
    tmp += sysCall;                  
    tmp += " failed";                  
    perror(tmp.c_str()); //???????????
  }
}




