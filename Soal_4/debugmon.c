#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

#define LOG_FILE "debugmon.log"
#define PID_FILE "debugmon.pid"

void debugmon_log(const char *operation, const char *proc, const char *status){
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(log, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s_%s\n",
            tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            operation, proc, status);
    fclose(log);
}

void list_user(const char *user) {
    struct passwd *pw = getpwnam(user);
    if (!pw) { perror("user tidak ditemukan"); return; }
    uid_t uid = pw->pw_uid;

    DIR *dir = opendir("/proc");
    if (!dir) { perror("tidak dapat dijangkau /proc"); return; }

    printf("%-8s %-20s %-10s %-10s\n", "PID", "COMMAND", "CPU", "MEM");

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!isdigit(ent->d_name[0])) continue;

        char path[512];
        snprintf(path, sizeof(path), "/proc/%s/status", ent->d_name);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char line[256], name[256] = "?", mem[256] = "0";
        uid_t pid_uid = -1;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Uid:\t%u", &pid_uid)) continue;
            if (sscanf(line, "Name:\t%255s", name)) continue;
            if (sscanf(line, "VmRSS: %255[^\n]", mem)) break;
        }
        fclose(f);

        if (pid_uid != uid) continue;

        snprintf(path, sizeof(path), "/proc/%s/stat", ent->d_name);
        FILE *sf = fopen(path, "r");
        if (!sf) continue;

        char skip[256];
        unsigned long utime, stime;
        for (int i = 0; i < 13; i++) fscanf(sf, "%s", skip);
        fscanf(sf, "%lu %lu", &utime, &stime);
        fclose(sf);

        double cpu = (utime + stime) / (double) sysconf(_SC_CLK_TCK);

        printf("%-8s %-20s %-10.2fs %-10s\n", ent->d_name, name, cpu, mem);
    }
    closedir(dir);
}

void run_daemon(const char *user) {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) {
        FILE *pf = fopen(PID_FILE, "w");
        if (pf) { fprintf(pf, "%d", pid); fclose(pf); }
        exit(0);
    }
    setsid();
    chdir("/");

    debugmon_log("daemon", "debugmon", "STARTED");

    struct passwd *pw = getpwnam(user);
    if (!pw) exit(1);
    uid_t uid = pw->pw_uid;

    while (1) {
        DIR *proc = opendir("/proc");
        if (!proc) exit(1);

        struct dirent *ent;
        char seen[100][256];
        int seen_count = 0;

        while ((ent = readdir(proc)) != NULL) {
            if (!isdigit(ent->d_name[0])) continue;

            char status_path[512];
            snprintf(status_path, sizeof(status_path), "/proc/%s/status", ent->d_name);
            FILE *f = fopen(status_path, "r");
            if (!f) continue;

            char name[256] = "unknown", line[256];
            uid_t pid_uid = -1;
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "Uid:", 4) == 0)
                    sscanf(line, "Uid:\t%u", &pid_uid);
                if (strncmp(line, "Name:", 5) == 0)
                    sscanf(line, "Name:\t%255s", name);
            }
            fclose(f);
            if (pid_uid == uid) {
                int already_logged = 0;
                for (int i = 0; i < seen_count; i++) {
                    if (strcmp(seen[i], name) == 0) {
                        already_logged = 1;
                        break;
                    }
                }
                if (!already_logged) {
                    strcpy(seen[seen_count++], name);
                    debugmon_log("daemon", name, "RUNNING");
                }
            }
        }
        closedir(proc);
        sleep(5);
    }
}

void stop_daemon() {
    FILE *pf = fopen(PID_FILE, "r");
    if (!pf) { printf("Daemon tidak berjalan.\n"); return; }
    int pid;
    fscanf(pf, "%d", &pid);
    fclose(pf);
    if (kill(pid, SIGTERM) == 0) {
        remove(PID_FILE);
        debugmon_log("stop", "debugmon", "STOPPED");
        printf("Daemon berhenti.\n");
    } else {
        perror("Gagal menghentikan daemon");
    }
}

void fail_user(const char *username) {
    const char *target_procs[] = {"node", "sh", "bash", "debugmon", NULL};

    struct passwd *pw = getpwnam(username);
    if (!pw) {
        fprintf(stderr, "User '%s' tidak ditemukan\n", username);
        return;
    }
    uid_t target_uid = pw->pw_uid;
    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("Gagal buka /proc");
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!isdigit(ent->d_name[0])) continue;

        char path[512];
        snprintf(path, sizeof(path), "/proc/%s/comm", ent->d_name);

        FILE *comm = fopen(path, "r");
        if (!comm) continue;

        char proc_name[256];
        fgets(proc_name, sizeof(proc_name), comm);
        fclose(comm);
        proc_name[strcspn(proc_name, "\n")] = '\0';

        int should_kill = 0;
        for (int i = 0; target_procs[i] != NULL; i++) {
            if (strcmp(proc_name, target_procs[i]) == 0) {
                should_kill = 1;
                break;
            }
        }
        if (should_kill) {
            pid_t pid = atoi(ent->d_name);
            if (pid == getpid()) continue;
            if (kill(pid, SIGTERM) == 0) {
                debugmon_log("fail", proc_name, "FAILED");
            } else {
                debugmon_log("fail", proc_name, "KILL_FAILED");
            }
        }
    }
    closedir(proc);
}

void revert_user(const char *user) {
    struct passwd *pw = getpwnam(user);
    if (!pw) {
        fprintf(stderr, "User '%s' tidak ditemukan\n", user);
        return;
    }
    uid_t uid = pw->pw_uid;
    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("Gagal membuka user/proc");
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!isdigit(ent->d_name[0])) continue;

        char path[512];
        snprintf(path, sizeof(path), "/proc/%s/status", ent->d_name);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char line[256], name[256] = "?", mem[256] = "0";
        uid_t pid_uid = -1;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Uid:\t%u", &pid_uid)) continue;
            if (sscanf(line, "Name:\t%255s", name)) continue;
            if (sscanf(line, "VmRSS: %255[^\n]", mem)) break;
        }
        fclose(f);
        if (pid_uid == uid) {
            debugmon_log("revert", name, "RUNNING");
        }
    }
    closedir(proc);
    printf("Akses user %s kembali seperti semula.\n", user);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./debugmon [list|daemon|stop|fail|revert] <user>\n");
        return 1;
    }
    if (!strcmp(argv[1], "list") && argc == 3) list_user(argv[2]);
    else if (!strcmp(argv[1], "daemon") && argc == 3) run_daemon(argv[2]);
    else if (!strcmp(argv[1], "stop")) stop_daemon();
    else if (!strcmp(argv[1], "fail") && argc == 3) fail_user(argv[2]);
    else if (!strcmp(argv[1], "revert") && argc == 3) revert_user(argv[2]);
    else printf("Perintah tidak valid.\n");
    return 0;
}
