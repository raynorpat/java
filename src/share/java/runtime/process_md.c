/* Process support reconstructed from JDK 1.1.5 javai_g.dll.
 * Linux maps the shipped Win32Process Java class to POSIX spawn/wait APIs.
 * See tests/native/process115-evidence.md for evidence and port deviations.
 */
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#ifdef XP_PC
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#endif
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "sys_api.h"
#include "java_io_FileDescriptor.h"
#include "java_lang_Win32Process.h"
#include "java_lang_Process.h"
#include "java_lang_Runtime.h"

#ifdef XP_PC

void java_lang_Win32Process_create(Hjava_lang_Win32Process *self,
                                  HString *command, HString *environment)
{
    HANDLE pipes[6];
    SECURITY_ATTRIBUTES attributes;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char *line, *env;
    char message[1024];
    int i, descriptors[3];
    DWORD error;
    if (command == NULL) {
        SignalError(0, "java/lang/NullPointerException", NULL);
        return;
    }
    line = allocCString(command);
    env = NULL;
    if (environment != NULL) {
        env = (char *)sysCalloc(javaStringLength(environment) + 2, 1);
        if (env != NULL)
            javaString2CString(environment, env, javaStringLength(environment) + 1);
    }
    if (line == NULL || (environment != NULL && env == NULL)) {
        sysFree(line);
        sysFree(env);
        SignalError(0, "java/lang/OutOfMemoryError", NULL);
        return;
    }
    memset(pipes, 0, sizeof(pipes));
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = NULL;
    attributes.bInheritHandle = TRUE;
    for (i = 0; i < 6; i += 2) {
        if (!CreatePipe(&pipes[i], &pipes[i + 1], &attributes, 512)) goto failed;
    }
    /* Parent-only endpoints must not keep child pipes alive after close. */
    if (!SetHandleInformation(pipes[1], HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(pipes[2], HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(pipes[4], HANDLE_FLAG_INHERIT, 0)) goto failed;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = pipes[0];
    startup.hStdOutput = pipes[3];
    startup.hStdError = pipes[5];
    if (!CreateProcessA(NULL, line, NULL, NULL, TRUE, DETACHED_PROCESS,
                        env, NULL, &startup, &process)) goto failed;
    CloseHandle(process.hThread);
    CloseHandle(pipes[0]); pipes[0] = NULL;
    CloseHandle(pipes[3]); pipes[3] = NULL;
    CloseHandle(pipes[5]); pipes[5] = NULL;
    descriptors[0] = _open_osfhandle((intptr_t)pipes[1], O_WRONLY | O_BINARY);
    descriptors[1] = _open_osfhandle((intptr_t)pipes[2], O_RDONLY | O_BINARY);
    descriptors[2] = _open_osfhandle((intptr_t)pipes[4], O_RDONLY | O_BINARY);
    if (descriptors[0] == -1 || descriptors[1] == -1 || descriptors[2] == -1) {
        for (i = 0; i < 3; ++i) {
            if (descriptors[i] != -1) _close(descriptors[i]);
            else CloseHandle(pipes[i == 0 ? 1 : i * 2]);
        }
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hProcess);
        sysFree(line); sysFree(env);
        SignalError(0, "java/io/IOException", "open_osfhandle");
        return;
    }
    unhand(self)->handle = (long)process.hProcess;
    unhand(unhand(self)->stdin_fd)->fd = descriptors[0] + 1;
    unhand(unhand(self)->stdout_fd)->fd = descriptors[1] + 1;
    unhand(unhand(self)->stderr_fd)->fd = descriptors[2] + 1;
    sysFree(line); sysFree(env);
    return;
failed:
    error = GetLastError();
    for (i = 0; i < 6; ++i) if (pipes[i] != NULL) CloseHandle(pipes[i]);
    jio_snprintf(message, sizeof(message), "CreateProcess: %s error=%lu", line, error);
    sysFree(line); sysFree(env);
    SignalError(0, "java/io/IOException", message);
}

long java_lang_Win32Process_exitValue(Hjava_lang_Win32Process *self)
{
    DWORD status;
    if (!GetExitCodeProcess((HANDLE)unhand(self)->handle, &status)) return -1;
    if (status == STILL_ACTIVE) {
        SignalError(0, "java/lang/IllegalThreadStateException", "process has not exited");
        return -1;
    }
    return status;
}

long java_lang_Win32Process_waitFor(Hjava_lang_Win32Process *self)
{
    DWORD status;
    WaitForSingleObject((HANDLE)unhand(self)->handle, INFINITE);
    if (!GetExitCodeProcess((HANDLE)unhand(self)->handle, &status)) return -1;
    return status;
}

void java_lang_Win32Process_destroy(Hjava_lang_Win32Process *self)
{
    TerminateProcess((HANDLE)unhand(self)->handle, 1);
}

void java_lang_Win32Process_close(Hjava_lang_Win32Process *self)
{
    if (unhand(self)->handle != 0) CloseHandle((HANDLE)unhand(self)->handle);
    unhand(self)->handle = 0;
}

Hjava_lang_Process *java_lang_Runtime_execInternal(Hjava_lang_Runtime *self,
                                      HArrayOfString *args, HArrayOfString *env)
{
    if (args == NULL) {
        SignalError(0, "java/lang/NullPointerException", NULL);
        return NULL;
    }
    return (Hjava_lang_Process *)execute_java_constructor(EE(),
        "java/lang/Win32Process", NULL,
        "([Ljava/lang/String;[Ljava/lang/String;)", args, env);
}

#else /* Explicit POSIX port of the shipped Win32Process Java interface. */

typedef struct {
    pid_t pid;
    int exited, status;
    pthread_mutex_t lock;
} process_state;

static int process_status(process_state *state)
{
    int status;
    pid_t result;
    if (state->exited) return 1;
    do { result = waitpid(state->pid, &status, WNOHANG); }
    while (result < 0 && errno == EINTR);
    if (result != state->pid) return 0;
    state->status = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    state->exited = 1;
    return 1;
}

long java_lang_Win32Process_exitValue(Hjava_lang_Win32Process *self)
{
    process_state *state;
    int exited, status;
    state = (process_state *)unhand(self)->handle;
    if (state == NULL) return -1;
    pthread_mutex_lock(&state->lock);
    exited = process_status(state);
    status = state->status;
    pthread_mutex_unlock(&state->lock);
    if (!exited) {
        SignalError(0, "java/lang/IllegalThreadStateException", "process has not exited");
        return -1;
    }
    return status;
}

long java_lang_Win32Process_waitFor(Hjava_lang_Win32Process *self)
{
    process_state *state;
    int exited, status;
    state = (process_state *)unhand(self)->handle;
    if (state == NULL) return -1;
    do {
        pthread_mutex_lock(&state->lock);
        exited = process_status(state);
        status = state->status;
        pthread_mutex_unlock(&state->lock);
        if (!exited) usleep(10000);
    } while (!exited);
    return status;
}

void java_lang_Win32Process_destroy(Hjava_lang_Win32Process *self)
{
    process_state *state;
    state = (process_state *)unhand(self)->handle;
    if (state == NULL) return;
    pthread_mutex_lock(&state->lock);
    if (!process_status(state)) kill(state->pid, SIGTERM);
    pthread_mutex_unlock(&state->lock);
}

static void *reap_process(void *arg)
{
    process_state *state;
    int status;
    state = (process_state *)arg;
    while (waitpid(state->pid, &status, 0) < 0 && errno == EINTR) {}
    pthread_mutex_destroy(&state->lock);
    sysFree(state);
    return NULL;
}

void java_lang_Win32Process_close(Hjava_lang_Win32Process *self)
{
    process_state *state;
    pthread_t reaper;
    state = (process_state *)unhand(self)->handle;
    if (state == NULL) return;
    unhand(self)->handle = 0;
    if (process_status(state)) {
        pthread_mutex_destroy(&state->lock);
        sysFree(state);
    } else if (pthread_create(&reaper, NULL, reap_process, state) == 0) {
        pthread_detach(reaper);
    }
}

/* Linux bypasses the Windows command-string constructor, retaining argv. */
void java_lang_Win32Process_create(Hjava_lang_Win32Process *self,
                                  HString *command, HString *environment)
{
    SignalError(0, "java/io/IOException", "Windows command line on POSIX");
}

static void free_strings(char **strings)
{
    int i;
    if (strings == NULL) return;
    for (i = 0; strings[i] != NULL; ++i) sysFree(strings[i]);
    sysFree(strings);
}

static char **process_strings(HArrayOfString *array)
{
    char **strings;
    int i, count;
    count = obj_length(array);
    strings = (char **)sysCalloc(count + 1, sizeof(char *));
    if (strings == NULL) {
        SignalError(0, "java/lang/OutOfMemoryError", NULL);
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        if (unhand(array)->body[i] == NULL) {
            SignalError(0, "java/lang/NullPointerException", NULL);
            free_strings(strings);
            return NULL;
        }
        strings[i] = allocCString(unhand(array)->body[i]);
        if (strings[i] == NULL) { free_strings(strings); return NULL; }
    }
    return strings;
}

static HObject *process_stream(char *class_name, Hjava_io_FileDescriptor *fd)
{
    return execute_java_constructor(EE(), class_name, NULL,
                                    "(Ljava/io/FileDescriptor;)", fd);
}

Hjava_lang_Process *java_lang_Runtime_execInternal(Hjava_lang_Runtime *unused,
                                      HArrayOfString *args, HArrayOfString *env)
{
    extern char **environ;
    Hjava_lang_Win32Process *process;
    HObject *stream;
    ClassClass *clazz;
    process_state *state;
    posix_spawn_file_actions_t actions;
    int pipes[6], i, error;
    char **argv, **envp;
    if (args == NULL) {
        SignalError(0, "java/lang/NullPointerException", NULL);
        return NULL;
    }
    if (obj_length(args) == 0) {
        SignalError(0, "java/lang/IllegalArgumentException", NULL);
        return NULL;
    }
    argv = process_strings(args);
    if (argv == NULL) return NULL;
    envp = env == NULL ? NULL : process_strings(env);
    if (env != NULL && envp == NULL) { free_strings(argv); return NULL; }
    clazz = FindClass(EE(), "java/lang/Win32Process", TRUE);
    process = clazz == NULL ? NULL : (Hjava_lang_Win32Process *)ObjAlloc(clazz, 0);
    if (process == NULL) { free_strings(argv); free_strings(envp); return NULL; }
    stream = execute_java_constructor(EE(), "java/io/FileDescriptor", NULL, "()");
    unhand(process)->stdin_fd = (Hjava_io_FileDescriptor *)stream;
    if (!exceptionOccurred(EE())) {
        stream = execute_java_constructor(EE(), "java/io/FileDescriptor", NULL, "()");
        unhand(process)->stdout_fd = (Hjava_io_FileDescriptor *)stream;
    }
    if (!exceptionOccurred(EE())) {
        stream = execute_java_constructor(EE(), "java/io/FileDescriptor", NULL, "()");
        unhand(process)->stderr_fd = (Hjava_io_FileDescriptor *)stream;
    }
    if (exceptionOccurred(EE())) { free_strings(argv); free_strings(envp); return NULL; }
    state = (process_state *)sysCalloc(1, sizeof(*state));
    if (state == NULL) {
        free_strings(argv); free_strings(envp);
        SignalError(0, "java/lang/OutOfMemoryError", NULL);
        return NULL;
    }
    pthread_mutex_init(&state->lock, NULL);
    for (i = 0; i < 6; ++i) pipes[i] = -1;
    for (i = 0; i < 6; i += 2) {
        if (pipe(pipes + i) != 0) goto failed;
        if (fcntl(pipes[i], F_SETFD, FD_CLOEXEC) < 0 ||
            fcntl(pipes[i + 1], F_SETFD, FD_CLOEXEC) < 0) goto failed;
    }
    error = posix_spawn_file_actions_init(&actions);
    if (error != 0) { errno = error; goto failed; }
    error = posix_spawn_file_actions_adddup2(&actions, pipes[0], 0);
    if (error == 0) error = posix_spawn_file_actions_adddup2(&actions, pipes[3], 1);
    if (error == 0) error = posix_spawn_file_actions_adddup2(&actions, pipes[5], 2);
    for (i = 0; i < 6 && error == 0; ++i)
        error = posix_spawn_file_actions_addclose(&actions, pipes[i]);
    if (error == 0)
        error = posix_spawnp(&state->pid, argv[0], &actions, NULL, argv,
                            envp == NULL ? environ : envp);
    posix_spawn_file_actions_destroy(&actions);
    if (error != 0) { errno = error; goto failed; }
    close(pipes[0]); close(pipes[3]); close(pipes[5]);
    free_strings(argv); free_strings(envp);
    unhand(process)->handle = (long)state;
    unhand(unhand(process)->stdin_fd)->fd = pipes[1] + 1;
    unhand(unhand(process)->stdout_fd)->fd = pipes[2] + 1;
    unhand(unhand(process)->stderr_fd)->fd = pipes[4] + 1;
    stream = process_stream("java/io/FileOutputStream", unhand(process)->stdin_fd);
    if (!exceptionOccurred(EE())) {
        stream = execute_java_constructor(
            EE(), "java/io/BufferedOutputStream", NULL, "(Ljava/io/OutputStream;)", stream);
        unhand(process)->stdin_stream = (struct Hjava_io_OutputStream *)stream;
    }
    if (!exceptionOccurred(EE())) {
        stream = process_stream("java/io/FileInputStream", unhand(process)->stdout_fd);
        if (!exceptionOccurred(EE())) {
            stream = execute_java_constructor(
                EE(), "java/io/BufferedInputStream", NULL, "(Ljava/io/InputStream;)", stream);
            unhand(process)->stdout_stream = (struct Hjava_io_InputStream *)stream;
        }
    }
    if (!exceptionOccurred(EE())) {
        stream = process_stream(
            "java/io/FileInputStream", unhand(process)->stderr_fd);
        unhand(process)->stderr_stream = (struct Hjava_io_InputStream *)stream;
    }
    if (exceptionOccurred(EE())) {
        java_lang_Win32Process_destroy(process);
        java_lang_Win32Process_close(process);
        close(pipes[1]); close(pipes[2]); close(pipes[4]);
        unhand(unhand(process)->stdin_fd)->fd = 0;
        unhand(unhand(process)->stdout_fd)->fd = 0;
        unhand(unhand(process)->stderr_fd)->fd = 0;
        return NULL;
    }
    return (Hjava_lang_Process *)process;
failed:
    error = errno;
    for (i = 0; i < 6; ++i) if (pipes[i] != -1) close(pipes[i]);
    pthread_mutex_destroy(&state->lock);
    sysFree(state);
    free_strings(argv); free_strings(envp);
    SignalError(0, "java/io/IOException", strerror(error));
    return NULL;
}
#endif
