# Sisop-2-2025-IT29

- Bayu Kurniawan - 5027241055
- Angga Firmansyah - 5027241062
- Oryza Qiara Ramadhani - 5027241084

# Soal 2

Dikerjakan oleh : Angga Firmansyah

A. Sebagai teman yang baik, Mafuyu merekomendasikan Kanade untuk mendownload dan unzip sebuah starter kit berisi file - file acak (sudah termasuk virus) melalui link berikut agar dapat membantu Kanade dalam mengidentifikasi virus - virus yang akan datang. Jangan lupa untuk menghapus file zip asli setelah melakukan unzip

- Mendownload file dan melakukan unzip file, serta menghapus file zip yang telah di unzip :

```bash
#define FILE_ZIP "starterkit.zip"
#define URL_DOWNLOAD "drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download"

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
```


#

B. Setelah mendownload starter kit tersebut, Mafuyu ternyata lupa bahwa pada starter kit tersebut, tidak ada alat untuk mendecrypt nama dari file yang diencrypt menggunakan algoritma Base64. Oleh karena itu, bantulah Mafuyu untuk membuat sebuah directory karantina yang dapat mendecrypt nama file yang ada di dalamnya (Hint: gunakan daemon).

Penggunaan: 

/starterkit --decrypt

- Pembuatan function dan kode untuk membuat dekripsi file:
```bash
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

```

#

C. Karena Kanade adalah orang yang sangat pemalas (kecuali jika membuat musik), maka tambahkan juga fitur untuk memindahkan file yang ada pada directory starter kit ke directory karantina, dan begitu juga sebaliknya.

Penggunaan: 

./starterkit --quarantine (pindahkan file dari directory starter kit ke karantina)

./starterkit --return (pindahkan file dari directory karantina ke starter kit)

- Pembuatan kode untuk melakukan pemindahan file ke direktori karantina dan mengembalikan isi folder ke starter_kit

```bash
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

```

#


D. Ena memberikan ide kepada mereka untuk menambahkan fitur untuk menghapus file - file yang ada pada directory karantina. Mendengar ide yang bagus tersebut, Kanade pun mencoba untuk menambahkan fitur untuk menghapus seluruh file yang ada di dalam directory karantina.

Penggunaan: 

./starterkit --eradicate 

- Pembubatan 

```bash
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

```

#

E. Karena tagihan listrik Kanade sudah membengkak dan tidak ingin komputernya menyala secara terus - menerus, ia ingin program decrypt nama file miliknya dapat dimatikan secara aman berdasarkan PID dari proses program tersebut.

Penggunaan:

./starterkit --shutdown

- Pembuatan

```bash
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


```

#

F. Mafuyu dan Kanade juga ingin program mereka dapat digunakan dengan aman dan nyaman tanpa membahayakan penggunanya sendiri, mengingat Mizuki yang masih linglung setelah keluar dari labirin Santerra De Laponte. Oleh karena itu, tambahkan error handling sederhana untuk mencegah penggunaan yang salah pada program tersebut.

#

G. Terakhir, untuk mencatat setiap penggunaan program ini, Kanade beserta Mafuyu ingin menambahkan log dari setiap penggunaan program ini dan menyimpannya ke dalam file bernama activity.log.

Format:
```bash
Decrypt: 
[dd-mm-YYYY][HH:MM:SS] - Successfully started decryption process with PID <pid>.

Quarantine:
[dd-mm-YYYY][HH:MM:SS] - <nama file> - Successfully moved to quarantine directory.

Return:
[dd-mm-YYYY][HH:MM:SS] - <nama file> - Successfully returned to starter kit directory.

Eradicate:
[dd-mm-YYYY][HH:MM:SS] - <nama file> - Successfully deleted.

Shutdown:
[dd-mm-YYYY][HH:MM:SS] - Successfully shut off decryption process with PID <pid>.
```

- Pembuatan activity log :

```bash
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

```
#
