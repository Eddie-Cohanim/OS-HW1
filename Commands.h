#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>
#include <iostream>


#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define ADRRESS_MAX_LENGTH (1000)

class Command 
{
    protected:
        char* m_cmd_line;
        char *m_arg_values[COMMAND_MAX_ARGS + 1];
        int m_arg_count;
        bool m_background;//?????????????????????????????????????????????????????????????????????????

    public:
        char* m_cmd_line_with_background;
        Command(const char *cmd_line, bool is_background);
        Command(const char *cmd_line);

        virtual ~Command();

        virtual void execute() = 0;
        //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command 
{
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command 
{
    private:
        bool m_complex;
        
    public:
        ExternalCommand(const char *cmd_line);

        virtual ~ExternalCommand() {}

        void execute() override;
};


///..........................JOBS DECLARATIONS.........................///

class JobsList 
{
    public:
        class JobEntry 
        {
            friend class JobsList;

            public:
                int m_jobId;
                pid_t m_jobPid;
                std::string m_jobName;
                JobEntry(int id, int pid, char* name, bool isStopped);
                bool m_isStopped;
                void killJob();
        };
        
        std::vector<JobEntry> m_listOfJobs;
    
        JobsList() = default;

        ~JobsList();

        void addJob(int pid, char* name, bool isStopped = false);

        void printJobsList();

        void killAllJobs();

        void removeFinishedJobs();

        JobEntry *getJobById(int jobId);

        JobEntry* getMaxJobId();

        void removeJobById(int jobId);

        JobEntry *getLastJob(int *lastJobId);

        JobEntry *getLastStoppedJob(int *jobId);
        // TODO: Add extra methods or modify exisitng ones as needed
};


///..........................SPECIAL IN COMMANDS.........................///


class PipeCommand : public Command 
{
    private:
        bool m_printToError;
        Command* m_firstCommand;
        Command* m_secondCommand;

    public:
        PipeCommand(const char* cmd_line, std::string firstCommand,
                    std::string secondCommand, bool printToError = false);

        virtual ~PipeCommand();

        void execute() override;
};

class WatchCommand : public Command 
{
    // TODO: Add your data members
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;

    bool isIntervalOrCommand(std::string interval);
    bool isNegativeInterval(std::string interval);
};

class RedirectionCommand : public Command 
{
    private:
        bool m_append;
        std::string m_commandLine;
        //std::string m_outPath;
        const char* m_outPath;
        std::string m_command;
        Command* m_commandToExecute;

    public:
        explicit RedirectionCommand(const char *cmd_line);

        virtual ~RedirectionCommand() {}

        void execute() override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {}

    void execute() override;
};


///..........................BUILT IN COMMANDS.........................///


class JobsCommand : public BuiltInCommand {
    private:
        JobsList* m_jobs;
    public:
        JobsCommand(const char *cmd_line, JobsList *jobs);

        virtual ~JobsCommand() {}

        void execute() override;
};

class ChangeDirCommand : public BuiltInCommand 
{
    private:
        std::string m_lastPwd;
        char* m_cmdLine;

    public:
        ChangeDirCommand(const char *cmd_line, std::string pLastPwd);

        virtual ~ChangeDirCommand() {}

        void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class ChpromptCommand : public BuiltInCommand 
{
public:
    ChpromptCommand(const char* cmd_line);

    virtual ~ChpromptCommand() {}

    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    private:
        JobsList* m_jobs;

    public:
        QuitCommand(const char *cmd_line, JobsList *jobs);

        virtual ~QuitCommand() {}

        void execute() override;
};

class KillCommand : public BuiltInCommand {
    private:
        JobsList* m_jobs;
    public:
        KillCommand(const char *cmd_line, JobsList *jobs);

        virtual ~KillCommand() {}

        void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    private:
        JobsList* m_jobs;

    public:
        ForegroundCommand(const char *cmd_line, JobsList *jobs);

        virtual ~ForegroundCommand() {}

        void execute() override;
};

class GetUserCommand : public BuiltInCommand {
public:
    GetUserCommand(const char *cmd_line);

    virtual ~GetUserCommand() {}

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() {}

    void execute() override;
    
    bool checkValidName(std::string name);

    void insertAlias(std::string name, std::string command);
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {}

    void execute() override;
};


///......................SMALL SHELL DECLERATION.......................///


class SmallShell {
private:
    std::string m_prompt;
    int m_foregroundId;
    SmallShell();

public:
    std::string m_lastPwd;
    JobsList m_jobs;
    std::vector<std::pair<std::string, std::string>> m_aliases_new;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);
    void setPrompt(const char *newPrompt);
    std::string getPrompt() const; 
    void setForeground(int foregroundId);
    int getForeground();
    void setLastPwd(const char* lastPwd);
    std::string getLastPwd();
};

void checkSysCallPtr(const char* sysCall, char* currDirPtr);
void checkSysCall(const char* sysCall, int currDir);


#ifndef DT_DIR
#define DT_DIR 4
#endif

#ifndef DT_REG
#define DT_REG 8
#endif

#ifndef O_DIRECTORY
#define O_DIRECTORY 0200000
#endif

struct linux_dirent {
    long           d_ino;    // Inode number
    off_t          d_off;    // Offset to next dirent
    unsigned short d_reclen; // Length of this record
    char           d_name[]; // Filename (null-terminated)
};

#endif //SMASH_COMMAND_H_