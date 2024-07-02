#include <unistd.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <pwd.h>
#include <regex>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "Commands.h"


using namespace std;


const std::string WHITESPACE = " \n\r\t\f\v";
static const std::vector<std::string> reservedKeywords = {
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

#define ALLPERMISSIONS 0666
#define FAIL -1
#define BUF_SIZE 1000


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


///..................HELPER FUNCTION IMPELEMENTATIONS..................///


void checkSysCallPtr(const char* sysCall, char* currDirPtr)
{
  if (currDirPtr == nullptr)
  {
    std::string tmp = "smash error: "; 
    tmp += sysCall;                  
    tmp += " failed";                  
    perror(tmp.c_str()); //???????????
  }
}

void checkSysCall(const char* sysCall, int currDir)
{
  if (currDir == -1)
  {
    std::string tmp = "smash error: "; 
    tmp += sysCall;                  
    tmp += " failed";                  
    perror(tmp.c_str()); //???????????
  }
}

void basicExternalCommand(char* firstArgValue, char** argValueArray)
{
  if(execvp(firstArgValue, argValueArray) == FAIL)
  {
    checkSysCall("execvp", FAIL);
  }
}

void complexExternalCommand(const char* cmdLine)
{
  const char* arguments [4];
  arguments[0] = "/bin/bash";
  arguments[1] = "-c";
  //tmpArray = cmdLine;
  arguments[2] = cmdLine;
  arguments[3] = nullptr;
  checkSysCall("execv", execv("/bin/bash", (char* const*)arguments));
}

void parsePipeCommand(const string& commandString, std::string* parsedPipeCommand)
{
  int pipeLocation = commandString.find('|');
  parsedPipeCommand[0] = _trim(commandString.substr(0, pipeLocation)).c_str();
  
  if(commandString.find("|&", pipeLocation) == std::string::npos)
  {
    parsedPipeCommand[1] = "|";
    parsedPipeCommand[2] = _trim(commandString.substr(pipeLocation + 1)).c_str(); 
  }
  else
  {
    parsedPipeCommand[1] = "|&";
    parsedPipeCommand[2] = _trim(commandString.substr(pipeLocation + 2)).c_str() ;
  }
}

void ForegroundHelper(int jobId)
{
  //SmallShell &smash = SmallShell::getInstance();

  if(SmallShell::getInstance().m_jobs.getJobById(jobId) == nullptr)
  {
    std::cerr << "smash error: fg: job-id " << jobId << " does not exist" << std::endl;
    return;
  }
  SmallShell::getInstance().setForeground(jobId);
  std::cout << SmallShell::getInstance().m_jobs.getJobById(jobId)->m_jobName << " " << std::to_string(jobId) << std::endl; 
  std::cerr << "this is where the waitpid failed" << std::endl;
  checkSysCall("waitpid", waitpid(SmallShell::getInstance().m_jobs.getJobById(jobId)->m_jobPid, nullptr, 0));
  SmallShell::getInstance().m_jobs.removeJobById(jobId);
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

Command *SmallShell::CreateCommand(const char *cmd_line) 
{
  string command = _trim(string(cmd_line));
  string firstWord = command.substr(0, command.find_first_of(" \n"));
  if (firstWord.empty()) 
  {
        return nullptr;
  }
  if (command.find_first_of("|") != std::string::npos)// pipe command
  {
    std::string parsedPipeCommand[3];
    std::string commandString (cmd_line);
    parsePipeCommand(commandString, parsedPipeCommand);
    return new PipeCommand(cmd_line, parsedPipeCommand[0], parsedPipeCommand[2], parsedPipeCommand[1] == "|&");
  }
  else if (command.find_first_of(">") != std::string::npos)
  {
      return new RedirectionCommand(cmd_line);
  }
  else if (command.find_first_of(">>") != std::string::npos)
  {
      return new RedirectionCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0)
  {
    return new ChpromptCommand(cmd_line);
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
  if (command == nullptr) 
  {
    return;
  }
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
    id = m_listOfJobs.back().m_jobId +1;
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
      it = m_listOfJobs.erase(it); //Erase returns the next iterator
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
  for (JobEntry job: m_listOfJobs)
  {
    std::cout << std::to_string(job.m_jobPid) << ": " << job.m_jobName << std::endl;//ditto
    checkSysCall("kill", kill(job.m_jobPid, SIGKILL));
  }
}

void JobsList::printJobsList()
{
  removeFinishedJobs();
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end(); ++it)
  {
    std::cout << "[" << it->m_jobId << "] " << it->m_jobName << std::endl;  
  }
}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  removeFinishedJobs();
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end(); ++it)
  {
    if(it->m_jobId == jobId)
    {
      JobsList::JobEntry* job = &(*it); ///////////////////////////////////////might cause mem leek
      return job;
    }
  }
  return nullptr;
}

JobsList::JobEntry* JobsList::getMaxJobId()
{
  removeFinishedJobs();
  int currentMax = 0;
  JobsList::JobEntry* job;
  for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end(); ++it)
  {
    if(it->m_jobId >= currentMax)
    {
      job = &(*it); 
    }
  }
  return job;
}

void JobsList::removeJobById(int jobId)
{
  removeFinishedJobs();
    for (auto it = m_listOfJobs.begin(); it != m_listOfJobs.end(); )
  {
      if (it->m_jobId == jobId)
      {
        it = m_listOfJobs.erase(it); //Erase returns the next iterator
        break;
      }
      else
      {
        ++it; // Only increment if not erasing
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

void JobsList::JobEntry::killJob(){
    if (kill(m_jobPid, SIGKILL) == -1) {
        perror("smash error: kill failed");
    }
}

JobsList::~JobsList()
{
  for( auto &job : m_listOfJobs)
  {
    job.killJob();
  }
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


BuiltInCommand::BuiltInCommand (const char *cmd_line) : Command(cmd_line) {}

ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute()
{
  if (m_arg_count == 1)
  {
    SmallShell::getInstance().setPrompt("smash");
  }
  else
  {
    SmallShell::getInstance().setPrompt((m_arg_values[1]));
  }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand:: execute()
{
  pid_t pid = getpid();
  std::string pidStr = std::to_string(pid);
  cout << "smash pid is " << pidStr << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

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

  if(strcmp(m_arg_values[1], "-") == false)
  {
    if(SmallShell::getInstance().m_lastPwd.empty())
    {
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      return;
    }
    char* lastDir = new char[ADRRESS_MAX_LENGTH];
    checkSysCallPtr("cwd", getcwd(lastDir, sizeof(char) * ADRRESS_MAX_LENGTH));
    int result = chdir(SmallShell::getInstance().m_lastPwd.c_str());
    checkSysCall("chdir", result);
    if(result != FAIL)
    {
      SmallShell::getInstance().setLastPwd(lastDir);
    }
  }
  else{
     char* lastDir = new char[ADRRESS_MAX_LENGTH];
  checkSysCallPtr("cwd", getcwd(lastDir, sizeof(char) * ADRRESS_MAX_LENGTH));
  int result = chdir(m_arg_values[1]);
  checkSysCall("chdir", result);
  if(result != FAIL)
  {
    SmallShell::getInstance().setLastPwd(lastDir);
  }
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

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , m_jobs(jobs)
{
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
      int jobId = SmallShell::getInstance().m_jobs.getMaxJobId()->m_jobId;
      ForegroundHelper(jobId);
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

    int jobId = std::stoi(m_arg_values[1]);
    
    ForegroundHelper(jobId);
    return;
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
  if(m_arg_count != 3)
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
    if(myJobId < 0)
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
    if(signal == SIGKILL)
    {
      m_jobs->removeJobById(myJobId);
    }
    std::cout << "signal number " << std::to_string(signal) << " was sent to pid " << std::to_string(jobPid) << std::endl;

  } 
  catch (const std::exception& e)
  {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
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

aliasCommand::aliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

bool aliasCommand::checkValidName(std::string name)
{

  std::regex validNamePattern("^[a-zA-Z0-9_]+$");
  if (std::regex_match(name, validNamePattern)) {
    return true;
  }
  return false;
}

void aliasCommand::insertAlias(std::string name, std::string Command)
{
  SmallShell &smash = SmallShell::getInstance();//think this is wrong
  smash.m_aliases_new.push_back({name, Command});
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
        for (const auto &alias : SmallShell::getInstance().m_aliases_new) {
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
  insertAlias(m_arg_values[1], m_arg_values[2]);  

}

unaliasCommand::unaliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void unaliasCommand::execute()
{
  if(m_arg_count < 2)
  {
    std::cerr << "smash error: unalias: not enough arguments" << std::endl;
    return;
  }
  SmallShell &smash = SmallShell::getInstance();
  for(int i = 1; i < m_arg_count; i++){
    bool found = false;
    for(auto it = smash.m_aliases_new.begin(); it != smash.m_aliases_new.end(); ++it) {
        if(it->second == m_arg_values[i]) {
            smash.m_aliases_new.erase(it);
            found = true;
            break;  // Exit the loop after erasing the element
        }
    }
    if(!found) {
        std::cerr << "smash error: unalias: " << m_arg_values[i] << " alias does not exist" << std::endl;
    }
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


RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line)
{
  std::string trimmedCmdLine = _trim(cmd_line);
  int arrowLocation = trimmedCmdLine.find_first_of('>');
  m_append = false;
  m_commandLine = trimmedCmdLine.substr(0, arrowLocation);

  if('>' == trimmedCmdLine[arrowLocation + 1])
  {
    //arrowLocation++; 
    m_append = true;
  }
  int i;
  for(i = 0; i < COMMAND_MAX_ARGS; i++)
  {
    std::string commandString = _trim(m_arg_values[i]);
    if(commandString.front() == '>')
    {
      i++;
      break;
    }
  }
  m_outPath =  m_arg_values[i]; 

}

void RedirectionCommand::execute()
{
  int newPathFileDiscriptor;
  if(m_append == true)
  {
    newPathFileDiscriptor = open(m_outPath, O_RDWR | O_CREAT | O_APPEND, ALLPERMISSIONS);
    checkSysCall("open", newPathFileDiscriptor);
  }
  else
  {
    newPathFileDiscriptor = open(m_outPath, O_WRONLY | O_CREAT | O_TRUNC , ALLPERMISSIONS);
    checkSysCall("open", newPathFileDiscriptor);
  }

  int oldPathFileDiscriptor = dup(1); // 1 --> output stream of the process
  dup2(newPathFileDiscriptor, 1);
  SmallShell::getInstance().executeCommand(m_commandLine.c_str());

  dup2(oldPathFileDiscriptor, 1);
  int closeFileResult = close(newPathFileDiscriptor);
  checkSysCall("close", closeFileResult);
}

GetUserCommand::GetUserCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetUserCommand::execute()
{
  if(m_arg_count < 2)
  {
    std::cerr << "smash error: getuser: not enough arguments" << std::endl;
    return;
  }
  string procpath = "/proc/" + string(m_arg_values[2]) + "/status";
  struct stat procPathStat;
  int result = stat(procpath.c_str(), &procPathStat);
  checkSysCall("stat", result);

  if(result == FAIL)
  {
    std::cerr << "smash error: getuser: " << m_arg_values[2] << " does not exist" << std::endl; //may be diffrent need to check!!
    return;
  }

  std::cout << "User: " << procPathStat.st_uid << std::endl;
  std::cout << "Group: " << procPathStat.st_gid << std::endl;
  
}

PipeCommand::PipeCommand(const char* cmd_line, std::string firstCommand,
                         std::string secondCommand, bool printToError):
  Command(cmd_line),
  m_printToError(printToError)
{
  m_firstCommand = SmallShell::getInstance().CreateCommand(firstCommand.c_str());
  m_secondCommand = SmallShell::getInstance().CreateCommand(secondCommand.c_str());
}

PipeCommand::~PipeCommand()
{
  delete m_firstCommand;
  delete m_secondCommand;
}

void PipeCommand::execute()
{
  int pipeFileDescriptor[2];
  int writeEnd;
  if(m_printToError == true)
  {
    writeEnd = STDERR_FILENO;
  }
  else
  {
    writeEnd = STDOUT_FILENO;
  }
  
  if (pipe(pipeFileDescriptor) == FAIL) 
  {
    perror("smash error: pipe failed");
    return;
  }

  pid_t firstPid = fork();
  if (firstPid == FAIL) 
  {
    perror("smash error: fork failed");
    return;
  }
  if (firstPid == 0) 
  {
    close(pipeFileDescriptor[0]);
    if (dup2(pipeFileDescriptor[1], writeEnd) == FAIL)
    {
      perror("smash error: dup2 failed");
      return;
    }
    close(pipeFileDescriptor[1]);
    m_firstCommand->execute();
    exit(0);
  }

  pid_t secondPid = fork();
  if (secondPid == FAIL) 
  {
    perror("smash error: fork failed");
    return;
  }
  if (secondPid == 0) 
  {
  close(pipeFileDescriptor[1]);
  if (dup2(pipeFileDescriptor[0], STDIN_FILENO) == FAIL) 
  {
    perror("smash error: dup2 failed");
    return;
  }
  close(pipeFileDescriptor[0]);
  m_secondCommand->execute();
  exit(0);
  }

  close(pipeFileDescriptor[0]);
  close(pipeFileDescriptor[1]);
  int status;
  if (waitpid(firstPid, &status, 0) == FAIL) 
  {
    perror("smash error: waitpid failed");
    return;
  }
  if (waitpid(secondPid, &status, 0) == FAIL) 
  {
    perror("smash error: waitpid failed");
    return;
  }
}

bool WatchCommand::isNegativeInterval(std::string interval)
{
  if(interval[0] == '-' && interval.find_first_not_of("123456789") == std::string::npos)
  {
    return true;
  }
  return false;
}

bool WatchCommand::isIntervalOrCommand(std::string interval)
{
  if(interval.find_first_not_of("123456789") == std::string::npos)
  {
    return true;
  }
  return false;
}

WatchCommand::WatchCommand(const char *cmd_line): Command(cmd_line) {}

void WatchCommand::execute()
{
  if(m_arg_count < 2)
  {
    std::cerr << "smash error: watch: not enough arguments" << std::endl;
    return;
  }
  if(m_arg_count > 3)
  {
    std::cerr << "smash error: watch: too many arguments" << std::endl;
    return;
  }
  if(isNegativeInterval(m_arg_values[2]))
  {
    std::cerr << "smash error: watch: : invalid interval" << std::endl;
    return;
  }
  SmallShell &smash = SmallShell::getInstance();
  if(!isIntervalOrCommand(m_arg_values[2])) // this is a command
  {
    while(true)
    {
      smash.executeCommand(m_arg_values[2]);
      sleep(2);
    }
  
  }
  else // this is an interval
  { 
    while(true)
    {
    smash.executeCommand(m_arg_values[3]);
    sleep(stoi(m_arg_values[2]));
    }
  }
  
}

ListDirCommand::ListDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ListDirCommand::execute()
{
  if(m_arg_count > 2)
  {
    std::cerr << "smash error: listdir: too many arguments" << std::endl;
    return;
  }
  int fileDescriptor;
  char buf[BUF_SIZE];
  struct linux_dirent *directoryEntry;
  int bytesPositioned;
  char d_type;

  
  fileDescriptor = open(m_arg_count > 1 ? m_arg_values[1] : ".", O_RDONLY | O_DIRECTORY);
    if (fileDescriptor == -1) 
    {
      perror("smash error: open failed");
      return;
    }
    int numBytesRead;
    std::vector<std::string> file_vector;
    std::vector<std::string> dir_vector;
    while(true) 
    {
      numBytesRead = syscall(SYS_getdents, fileDescriptor, buf, BUF_SIZE);
      if (numBytesRead == -1) 
      {
        perror("smash error: getdents failed");
        close(fileDescriptor);
        return;
      }
      if (numBytesRead == 0)
      {
        break;
      }

      for (bytesPositioned = 0; bytesPositioned < numBytesRead;) 
      {
        directoryEntry = (struct linux_dirent *) (buf + bytesPositioned);
        d_type = *(buf + bytesPositioned + directoryEntry->d_reclen - 1);

        if (d_type == DT_REG){
          file_vector.emplace_back(directoryEntry->d_name);
        } else if(d_type == DT_DIR){
          dir_vector.emplace_back(directoryEntry->d_name);
        }

        bytesPositioned += directoryEntry->d_reclen;
      }
    }

    std::sort(file_vector.begin(), file_vector.end());
    std::sort(dir_vector.begin(), dir_vector.end());
    for(auto &_file : file_vector){
        std::cout << "file: "<< _file << std::endl;
    }
    for(auto &_dir : dir_vector){
        std::cout << "directory: "<< _dir << std::endl;
    }
}

   