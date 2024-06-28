#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

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
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;
};

class PipeCommand : public Command 
{
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class WatchCommand : public Command 
{
    // TODO: Add your data members
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command 
{
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};


///..........................BUILT IN COMMANDS.........................///


class ChangeDirCommand : public BuiltInCommand 
{
    private:

// TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

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



// class PwdCommand : public BuiltInCommand 
// {
// public:
//     PwdCommand(const char* cmd_line);

//     virtual ~PwdCommand() {}

//     void execute() override;
// };



class CdCommand : public BuiltInCommand 
{
public:
    CdCommand(const char* cmd_line);

    virtual ~CdCommand() {}

    void execute() override;
};












class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};

class JobsList 
{
    public:
        class JobEntry 
        {
            public:
                int m_jobId;
                pid_t m_jobPid;
                std::string m_jobName;
                JobEntry(int id, int pid, char* name);
        };
        
        std::vector<JobEntry> m_listOfJobs;
    
        JobsList() = default;

        ~JobsList() = default;

        void addJob(Command *cmd, bool isStopped = false);

        void printJobsList();

        void killAllJobs();

        void removeFinishedJobs();

        JobEntry *getJobById(int jobId);

        void removeJobById(int jobId);

        JobEntry *getLastJob(int *lastJobId);

        JobEntry *getLastStoppedJob(int *jobId);
        // TODO: Add extra methods or modify exisitng ones as needed
    };

    class JobsCommand : public BuiltInCommand {
        // TODO: Add your data members
    public:
        JobsCommand(const char *cmd_line, JobsList *jobs);

        virtual ~JobsCommand() {}

        void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {}

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
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {}

    void execute() override;
};


class SmallShell {
private:
    char* m_prompt;
    int m_forground_id;
    SmallShell();
    std::string m_last_pwd;

public:
    std::string m_last_pwd;
    JobsList m_jobs;

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
    void setForground(int forground_id);
    int getForground();
    void setLastPwd(char* lastPwd);
    std::string getLastPwd();
};

#endif //SMASH_COMMAND_H_
