#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>


#define STARTER_KIT_DIR "starter_kit"
#define QUARANTINE_DIR "quarantine"
#define LOG_FILE "activity.log"
#define PID_FILE "daemon.pid"
#define DOWNLOAD_URL "drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download"
#define ZIP_NAME "starterkit.zip"

void write_log(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%02d-%02d-%04d][%02d:%02d:%02d] - ",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}

void execute_command(char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork failed");
    }
}

void download_and_unzip() {
    char *wget_argv[] = {"wget", "-O", ZIP_NAME, DOWNLOAD_URL, NULL};
    execute_command(wget_argv);

    mkdir(STARTER_KIT_DIR, 0755);
    char *unzip_argv[] = {"unzip", "-q", ZIP_NAME, "-d", STARTER_KIT_DIR, NULL};
    execute_command(unzip_argv);

    remove(ZIP_NAME);
}

int is_base64(const char *str) {
    while (*str) {
        if (!(isalnum(*str) || *str == '+' || *str == '/' || *str == '='))
            return 0;
        str++;
    }
    return 1;
}

char *base64_decode(const char *data) {
    char command[512];
    snprintf(command, sizeof(command), "echo '%s' | base64 -d", data);
    FILE *fp = popen(command, "r");
    if (!fp) return NULL;

    char *decoded = malloc(256);
    if (!decoded) return NULL;

    if (fgets(decoded, 256, fp) == NULL) {
        pclose(fp);
        free(decoded);
        return NULL;
    }
    pclose(fp);
    decoded[strcspn(decoded, "\n")] = 0;
    return decoded;
}

void move_to_quarantine() {
    struct stat st = {0};
    if (stat(QUARANTINE_DIR, &st) == -1) {
        mkdir(QUARANTINE_DIR, 0700);
    }

    DIR *d = opendir(STARTER_KIT_DIR);
    if (!d) return;

    struct dirent *dir;
    char src[512], dest[512];
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            snprintf(src, sizeof(src), "%s/%s", STARTER_KIT_DIR, dir->d_name);
            snprintf(dest, sizeof(dest), "%s/%s", QUARANTINE_DIR, dir->d_name);
            if (rename(src, dest) == 0) {
                write_log("%s - Successfully moved to quarantine directory.", dir->d_name);
            }
        }
    }
    closedir(d);
}

void daemon_decrypt() {
    struct stat st = {0};
    if (stat("quarantine", &st) == -1) {
        mkdir("quarantine", 0700);
    }

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        FILE *fp = fopen("daemon.pid", "w");
        if (fp) {
            fprintf(fp, "%d\n", pid);
            fclose(fp);
            write_log("Successfully started decryption process with PID %d.", pid);
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
        DIR *d = opendir("quarantine");
        if (!d) {
            sleep(5);
            continue;
        }

        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG && is_base64(dir->d_name)) {
                char oldname[512], newname[512];
                snprintf(oldname, sizeof(oldname), "quarantine/%s", dir->d_name);

                char *decoded = base64_decode(dir->d_name);
                if (decoded && strlen(decoded) > 0 && strchr(decoded, '/') == NULL) {
                    snprintf(newname, sizeof(newname), "quarantine/%s", decoded);

                    if (rename(oldname, newname) == 0) {
                        write_log("%s - Successfully decrypted to %s.", dir->d_name, decoded);
                    }
                }

                free(decoded);
            }
        }
        closedir(d);
        sleep(5);
    }
}


void return_files() {
    DIR *d = opendir(QUARANTINE_DIR);
    if (!d) return;

    struct dirent *dir;
    char src[512], dest[512];
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            snprintf(src, sizeof(src), "%s/%s", QUARANTINE_DIR, dir->d_name);
            snprintf(dest, sizeof(dest), "%s/%s", STARTER_KIT_DIR, dir->d_name);
            if (rename(src, dest) == 0) {
                write_log("%s - Successfully returned to starter kit directory.", dir->d_name);
            }
        }
    }
    closedir(d);
}

void eradicate_quarantine() {
    DIR *d = opendir(QUARANTINE_DIR);
    if (!d) return;

    struct dirent *dir;
    char filepath[512];
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            snprintf(filepath, sizeof(filepath), "%s/%s", QUARANTINE_DIR, dir->d_name);
            if (remove(filepath) == 0) {
                write_log("%s - Successfully deleted.", dir->d_name);
            }
        }
    }
    closedir(d);
}

void shutdown_daemon() {
    FILE *fp = fopen(PID_FILE, "r");
    if (!fp) return;

    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return;
    }
    fclose(fp);

    if (kill(pid, SIGTERM) == 0) {
        write_log("Successfully shut off decryption process with PID %d.", pid);
        remove(PID_FILE);
    }
}

int main(int argc, char *argv[]) {
    mkdir(STARTER_KIT_DIR, 0755);
    mkdir(QUARANTINE_DIR, 0755);

    if (argc == 1) {
        
        download_and_unzip();
        return 0;
    }

    if (strcmp(argv[1], "--decrypt") == 0) {
        daemon_decrypt();
    } else if (strcmp(argv[1], "--quarantine") == 0) {
        move_to_quarantine();
    } else if (strcmp(argv[1], "--return") == 0) {
        return_files();
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        eradicate_quarantine();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        shutdown_daemon();
    } else {
        fprintf(stderr, "Usage: %s [--decrypt|--quarantine|--return|--eradicate|--shutdown]\n", argv[0]);
        return 1;
    }

    return 0;
}


