/* Bound jdb and its automatically launched java_g to one owned Windows job. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    HANDLE job;
    DWORD result = 1;
    char *command = GetCommandLineA();
    int quoted = 0;
    while (*command) {
        if (*command == '"') quoted = !quoted;
        else if (!quoted && (*command == ' ' || *command == '\t')) break;
        ++command;
    }
    while (*command == ' ' || *command == '\t') ++command;
    if (!*command) return 1;
    job = CreateJobObjectA(NULL, NULL);
    if (!job) return 1;
    memset(&limits, 0, sizeof(limits));
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        CloseHandle(job); return 1;
    }
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, CREATE_SUSPENDED,
                       NULL, NULL, &startup, &process)) {
        fprintf(stderr, "Cannot start debugger: %lu\n", GetLastError());
        CloseHandle(job); return 1;
    }
    if (!AssignProcessToJobObject(job, process.hProcess) || ResumeThread(process.hThread) == (DWORD)-1) {
        TerminateProcess(process.hProcess, 1);
    } else {
        WaitForSingleObject(process.hProcess, INFINITE);
        GetExitCodeProcess(process.hProcess, &result);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(job);
    return (int)result;
}
