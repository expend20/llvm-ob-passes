// TODO: very basic wrapper, better wrapper would need mixing bitcode and enforcing seccomp directly in the process
#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <seccomp.h>
#include <signal.h>

void setup_seccomp() {
    scmp_filter_ctx ctx;

    ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (ctx == NULL) {
        std::cerr << "Failed to initialize seccomp" << std::endl;
        exit(1);
    }

    // Block network-related syscalls
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(socket), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(connect), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(bind), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(accept), 0);

    // Block file-related syscalls (exceptions are needed to make actual run work)
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(open), 0);
    //seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(openat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(creat), 0);
    //seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(read), 0);
    //seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(write), 0);

    // Block process creation
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(fork), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(vfork), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(clone), 0);
    //seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(execve), 0);

    if (seccomp_load(ctx) < 0) {
        std::cerr << "Failed to load seccomp rules" << std::endl;
        seccomp_release(ctx);
        exit(1);
    }

    seccomp_release(ctx);
}

void timeout_handler(int signum) {
    std::cerr << "Time limit exceeded (3 seconds)" << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [args...]" << std::endl;
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        std::cerr << "Fork failed" << std::endl;
        return 1;
    } else if (pid == 0) {
        // Child process
        setup_seccomp();

        // Set up signal handler for timeout
        signal(SIGALRM, timeout_handler);
        alarm(1);  // Set 1-second timeout

        // Prepare arguments for execvp
        std::vector<char*> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        args.push_back(nullptr);

        // Execute the subsequent binary
        // clean environment
        clearenv();
        // close stdin
        close(STDIN_FILENO);
        execvp(args[0], args.data());

        // If execvp fails
        std::cerr << "Failed to execute " << args[0] << std::endl;
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            std::cerr << "Child process terminated by signal " << WTERMSIG(status) << std::endl;
            return 1;
        }
    }

    return 0;
}
