#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define DIR_KIT "starter_kit"
#define DIR_ISOLATION "quarantine"
#define FILE_LOG "activity.log"
#define FILE_PID "daemon.pid"
#define FILE_ZIP "starterkit.zip"
#define URL_DOWNLOAD "drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download"

void log_activity(const char *fmt, ...) {
    FILE *fp = fopen(FILE_LOG, "a");
    if (!fp) return;

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    fprintf(fp, "[%02d-%02d-%04d][%02d:%02d:%02d] - ",
            lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
            lt->tm_hour, lt->tm_min, lt->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);

    fprintf(fp, "\n");
    fclose(fp);
}

void run_process(char *const args[]) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp error");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork error");
    }
}

void fetch_and_extract() {
    char *get[] = {"wget", "-O", FILE_ZIP, URL_DOWNLOAD, NULL};
    run_process(get);

    mkdir(DIR_KIT, 0755);
    char *unzip[] = {"unzip", "-q", FILE_ZIP, "-d", DIR_KIT, NULL};
    run_process(unzip);

    remove(FILE_ZIP);
}

int valid_base64(const char *str) {
    while (*str) {
        if (!(isalnum(*str) || *str == '+' || *str == '/' || *str == '='))
            return 0;
        str++;
    }
    return 1;
}

char *decode_b64(const char *input) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "echo '%s' | base64 -d", input);
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return NULL;

    char *output = malloc(256);
    if (!output) return NULL;

    if (!fgets(output, 256, pipe)) {
        free(output);
        pclose(pipe);
        return NULL;
    }
    pclose(pipe);

    output[strcspn(output, "\n")] = 0;
    return output;
}

void quarantine_files() {
    struct stat s;
    if (stat(DIR_ISOLATION, &s) == -1) {
        mkdir(DIR_ISOLATION, 0700);
    }

    DIR *src = opendir(DIR_KIT);
    if (!src) return;

    struct dirent *entry;
    while ((entry = readdir(src)) != NULL) {
        if (entry->d_type == DT_REG) {
            char from[512], to[512];
            snprintf(from, sizeof(from), "%s/%s", DIR_KIT, entry->d_name);
            snprintf(to, sizeof(to), "%s/%s", DIR_ISOLATION, entry->d_name);
            if (rename(from, to) == 0) {
                log_activity("%s - Moved to quarantine.", entry->d_name);
            }
        }
    }
    closedir(src);
}

void decrypt_daemon() {
    mkdir(DIR_ISOLATION, 0700);

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        FILE *fp = fopen(FILE_PID, "w");
        if (fp) {
            fprintf(fp, "%d\n", pid);
            fclose(fp);
            log_activity("Started decrypt daemon with PID %d.", pid);
        }
        exit(EXIT_SUCCESS);
    }

    umask(0);
    setsid();
    chdir(".");

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    while (1) {
        DIR *qdir = opendir(DIR_ISOLATION);
        if (!qdir) {
            sleep(5);
            continue;
        }

        struct dirent *file;
        while ((file = readdir(qdir)) != NULL) {
            if (file->d_type == DT_REG && valid_base64(file->d_name)) {
                char old[512], new[512];
                snprintf(old, sizeof(old), "%s/%s", DIR_ISOLATION, file->d_name);

                char *decoded = decode_b64(file->d_name);
                if (decoded && strlen(decoded) && !strchr(decoded, '/')) {
                    snprintf(new, sizeof(new), "%s/%s", DIR_ISOLATION, decoded);
                    if (rename(old, new) == 0) {
                        log_activity("%s - Decrypted to %s.", file->d_name, decoded);
                    }
                }
                free(decoded);
            }
        }

        closedir(qdir);
        sleep(5);
    }
}

void restore_files() {
    DIR *qdir = opendir(DIR_ISOLATION);
    if (!qdir) return;

    struct dirent *file;
    while ((file = readdir(qdir)) != NULL) {
        if (file->d_type == DT_REG) {
            char src[512], dst[512];
            snprintf(src, sizeof(src), "%s/%s", DIR_ISOLATION, file->d_name);
            snprintf(dst, sizeof(dst), "%s/%s", DIR_KIT, file->d_name);
            if (rename(src, dst) == 0) {
                log_activity("%s - Restored to starter kit.", file->d_name);
            }
        }
    }
    closedir(qdir);
}

void purge_quarantine() {
    DIR *qdir = opendir(DIR_ISOLATION);
    if (!qdir) return;

    struct dirent *file;
    while ((file = readdir(qdir)) != NULL) {
        if (file->d_type == DT_REG) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", DIR_ISOLATION, file->d_name);
            if (remove(path) == 0) {
                log_activity("%s - File deleted.", file->d_name);
            }
        }
    }
    closedir(qdir);
}

void stop_daemon() {
    FILE *fp = fopen(FILE_PID, "r");
    if (!fp) return;

    pid_t pid;
    if (fscanf(fp, "%d", &pid) == 1) {
        if (kill(pid, SIGTERM) == 0) {
            log_activity("Stopped daemon with PID %d.", pid);
            remove(FILE_PID);
        }
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    mkdir(DIR_KIT, 0755);
    mkdir(DIR_ISOLATION, 0755);

    if (argc == 1) {
        fetch_and_extract();
        return 0;
    }

    if (strcmp(argv[1], "--decrypt") == 0) {
        decrypt_daemon();
    } else if (strcmp(argv[1], "--quarantine") == 0) {
        quarantine_files();
    } else if (strcmp(argv[1], "--return") == 0) {
        restore_files();
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        purge_quarantine();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        stop_daemon();
    } else {
        fprintf(stderr, "Usage: %s [--decrypt|--quarantine|--return|--eradicate|--shutdown]\n", argv[0]);
        return 1;
    }

    return 0;
}
