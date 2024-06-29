#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <pwd.h>
#include <regex>
#include <sys/stat.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
static const     std::vector<std::string> reservedKeywords = {
        "quit", "lisdir", "chprompt", "showpid", "cd", "jobs", "fg", "kill", "pwd", "alias","unalias","kill",">",">>","|","getuser","watch"
    };

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

void SmallShell::setForeground(int foregroundId)
{
  m_foregroundId = foregroundId;
}

int SmallShell::getForeground()
{
  return m_foregroundId;
}

void SmallShell::setPrompt(const char *new_prompt)
{
  m_prompt = std::string(new_prompt);
}

std::string SmallShell::getPrompt() const 
{
  return m_prompt;
}

void SmallShell::setLastPwd(const char *lastPwd)
{
  m_lastPwd = std::string(lastPwd);
}

std::string SmallShell::getLastPwd()
{
  return m_lastPwd;
}

SmallShell::SmallShell() 
{
  m_prompt = "smash";
  m_lastPwd = std::string(); 
  m_foregroundId = -1;
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
  string firstWord = command.substr(0, command.find_first_of(" \n"));

  if (command.find_first_of(">") != std::string::npos)
  {
      return new RedirectionCommand(cmd_line);
  }
  else if (command.find_first_of(">>") != std::string::npos)
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
    return new ChangeDirCommand(cmd_line, m_lastPwd);
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
  delete command;
}


///.......................JOBS LIST IMPLEMENTATION.....................///


JobsList::JobEntry::JobEntry(int id, int pid, char* name, bool isStopped = false)
 : m_jobId(id), m_jobPid(pid), m_isStopped(isStopped)
{
  m_jobName = std::string(name);
}

void JobsList::addJob(int pid, char* name, bool isStopped)
{
  removeFinishedJobs();
  int id = 1;
  if(m_listOfJobs.empty() == false)
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
    int result = waitpid(it->m_jobPid, &status, WNOHANG);
    checkSysCall("waitpid", result);
    if (result != 0)
    {
      it = m_listOfJobs.erase(it);
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
  std::cout << "smash: sending SIGKILL signal to " << std::to_string(m_listOfJobs.size()) << " jobs:" << std::endl;//not sure this is neccesary
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end();)
  {
    std::cout << std::to_string(it->m_jobPid) << ": " << it->m_jobName << std::endl;//ditto
    checkSysCall("kill", kill(it->m_jobPid, SIGKILL));
  }
}

void JobsList::printJobsList()
{
  removeFinishedJobs();
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end();)
  {
    std::cout << "[" << it->m_jobId << "]" << it->m_jobName << std::endl;
  }
}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  removeFinishedJobs();
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end();)
  {
    if(it->m_jobId == jobId)
    {
      JobsList::JobEntry job = *it; ///////////////////////////////////////might cause mem leek
      return &job;
    }
  }
  return nullptr;
}

void JobsList::removeJobById(int jobId)
{
  removeFinishedJobs();
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end();)
  {
    if(it->m_jobId == jobId)
    {
      m_listOfJobs.erase(it);
    }
  }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId)
{
  removeFinishedJobs();
  if(m_listOfJobs.empty())
  {
      return nullptr;
  } 
  *lastJobId = m_listOfJobs.back().m_jobId;
  return &m_listOfJobs.back();
}


///.....................COMMAND IMPLEMENTATION.........................///


Command:: Command(const char* cmd_line) 
{
  m_background = _isBackgroundComamnd(cmd_line);
  m_cmd_line = new char[COMMAND_MAX_LENGTH + 1];
  m_cmd_line_with_background = new char[COMMAND_MAX_LENGTH + 1];
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
  char *currDir = new char[ADRRESS_MAX_LENGTH];
  checkSysCallPtr("pwd:", getcwd(currDir, sizeof(char) * ADRRESS_MAX_LENGTH)); //?????????
  std::cout << currDir << std::endl;
  delete [] currDir;

}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, std::string pLastPwd)
 : BuiltInCommand(cmd_line), m_lastPwd(pLastPwd)
  {}

void ChangeDirCommand:: execute()
{
  if(m_arg_count > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }

  if(m_arg_values[1] == "-")
  {
    if(SmallShell::getInstance().m_lastPwd.empty())
    {
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      return;
    }
    char* lastDir = new char[ADRRESS_MAX_LENGTH];
    checkSysCallPtr("cd:", getcwd(lastDir, sizeof(char) * ADRRESS_MAX_LENGTH));
    int result = chdir(SmallShell::getInstance().m_lastPwd.c_str());
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

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line)
{
  m_jobs = jobs;
}

void JobsCommand:: execute()
{
  SmallShell::getInstance().m_jobs.removeFinishedJobs();
  SmallShell::getInstance().m_jobs.printJobsList();
}

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line)
{
  m_jobs = jobs;
}

void ForegroundCommand:: execute()
{
  if(m_arg_count == 1)
  {
    if(SmallShell::getInstance().m_jobs.m_listOfJobs.empty())
    {
      std::cerr << "smash error: fg: job list is empty" << std::endl;
      return;
    }
    else
    {
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
      return;
    }
  }
  
  if(m_arg_count > 2)
  {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }
    
  try
  {
    int myJob = std::stoi(m_arg_values[2]);
    pid_t jobPid = SmallShell::getInstance().m_jobs.getJobById(myJob)->m_jobPid;
    if(SmallShell::getInstance().m_jobs.getJobById(myJob) == nullptr)
    {
      std::cerr << "smash error: fg: job-id " << myJob << " does not exist" << std::endl;
      return;
    }
    SmallShell::getInstance().setForeground(myJob);
    std::cout << myJob << " " << std::to_string(jobPid) << std::endl; 
    checkSysCall("waitpid", waitpid(jobPid, nullptr, 0));
    SmallShell::getInstance().m_jobs.removeJobById(myJob);
  }
  catch(std::exception &e)
  {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }
}

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line)
{
  m_jobs = jobs;
}

void KillCommand::execute()
{ 
  if(m_arg_count > 3)
  {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  
  int myJobId;
  int signal;

  try
  {
    signal = - std::stoi(m_arg_values[1]);
  }
  catch(std::exception &e)
  {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  if(signal < 0) // may have upper limit
  {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  
  try
  {
    myJobId = std::stoi(m_arg_values[2]);
  } 
  catch (const std::exception& e)
  {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }

  if(m_jobs->getJobById(myJobId) == nullptr)
  {
   std::cerr << "smash error: kill: job-id " << std::to_string(myJobId) << " does not exist" << std::endl;
   return;
  }
  pid_t jobPid = m_jobs->getJobById(myJobId)->m_jobPid;
  
  int result = kill(jobPid, signal);
  checkSysCall("kill", result);
  if(result != -1)
  {
    std::cout << "signal number " << std::to_string(signal) << " was sent to pid " << std::to_string(jobPid) << std::endl;
  }
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line)
{
  m_jobs = jobs;
}

void QuitCommand::execute()
{
  if(m_arg_count != 1)
  {
    std::string signal = std::string(m_arg_values[1]);
    if(signal == "kill")
    {
      m_jobs->killAllJobs();
    }
  }
  exit(0);
}

bool aliasCommand::checkValidName(std::string name)
{

  std::regex validNamePattern("^[a-zA-Z0-9_]+$");
  if (std::regex_match(name, validNamePattern)) {
    return true;
  }
  return false;
}

void aliasCommand::execute()
{
  if(m_arg_count != 3 || m_arg_count != 1)
  {
    std::cerr << "smash error: alias: invalid arguments" << std::endl;
    return;
  }
  if( m_arg_count == 1)
  {
        for (const auto &alias : SmallShell::getInstance().m_aliases) {
        cout << alias.first << "='" << alias.second << "'" << endl;
    }
    return;
  }
  if(checkValidName(m_arg_values[1]) == false)
  {
    std::cerr << "smash error: alias: invalid alias format" << std::endl;
    return;
  }
  for (const auto& keyword : reservedKeywords) 
  {
      if (m_arg_values[1] == keyword)
        {
          std::cerr << "smash error: alias:" << m_arg_values[1] << "already exists or is a reserved command"
          << std::endl;
          return;
        }
  }
  SmallShell::getInstance().m_aliases.insert({m_arg_values[1], m_arg_values[2]});  

}

void unaliasCommand::execute()
{
  if(m_arg_count < 2)
  {
    std::cerr << "smash error: unalias: not enough arguments" << std::endl;
    return;
  }
  
}

///..................EXTERNAL COMMAND IMPELEMENTATIONS.................///


ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line)
{
  m_complex = false;
  for(int i = 0; i < m_arg_count; i++)
  {
    std::string argument = std::string(m_arg_values[i]);
    if(argument.find('?') == std::string::npos || argument.find('*') == std::string::npos)
    {
      m_complex = true;
      break;
    }
  }
}

void ExternalCommand::execute()
{
  pid_t sonPid = fork();
  checkSysCall("fork", sonPid);
  if(sonPid == 0)//we are the son
  {
    checkSysCall("setpgrp", setpgrp());//maybe?
    if(m_complex == false)//basic command
    {
      basicExternalCommand(m_arg_values[0], m_arg_values);
    }

    else//complex command
    {
      complexExternalCommand(m_cmd_line);
    }
    exit(0);
  }

  else//we are the father
  {
    if (m_background == false)
    {
      int status; 
      SmallShell::getInstance().setForeground(sonPid);
      checkSysCall("waitpid", waitpid(sonPid, &status, 0)); 
      SmallShell::getInstance().setForeground(-1);
    }
    
    else
    {
      SmallShell::getInstance().m_jobs.addJob(sonPid, m_cmd_line_with_background);
    }
  }
}


///..................SPECIAL COMMANDS IMPLEMENTATIONS..................///





///..................HELPER FUNCTION IMPELEMENTATIONS..................///

void basicExternalCommand(char* firstArgValue, char** argValueArray)
{
  if(execvp(firstArgValue, argValueArray) == -1)
  {
    checkSysCall("execvp", -1);
  }
}

void complexExternalCommand(char* cmdLine)
{
  char* arguments [4];
  arguments[0] = "/bin/bash";
  arguments[1] = "-c";
  arguments[2] = cmdLine;
  arguments[3] = nullptr;
  checkSysCall("execv", execv("/bin/bash", arguments));
}

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




