# Sisop-2-2025-IT29

- Bayu Kurniawan - 5027241055
- Angga Firmansyah - 5027241062
- Oryza Qiara Ramadhani - 5027241084

# Soal 2

Dikerjakan oleh : Angga Firmansyah

A. Sebagai teman yang baik, Mafuyu merekomendasikan Kanade untuk mendownload dan unzip sebuah starter kit berisi file - file acak (sudah termasuk virus) melalui link berikut agar dapat membantu Kanade dalam mengidentifikasi virus - virus yang akan datang. Jangan lupa untuk menghapus file zip asli setelah melakukan unzip

Penyelesaian : 

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
Function diatas(fetch_and_extract) mendownload file yang telah diberikan di link dan juga menghapus file.zip yang telah di download dan telah di ekstrak/

Dokumentasi : 
![image](https://github.com/user-attachments/assets/2a9399b6-ff42-4e9b-9520-2fcfba82ad26)
![image](https://github.com/user-attachments/assets/cb928aca-3aee-4e6b-be52-d2f17556eb8e)



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

Dengan menggunakan algoritma base64 untung melakukan dekripsi file starter_kit yang akan dipindahkan. Berikut adalah contoh dokumentasi hasil menyalakan dekripsi dengan bukti activity.log :

Dokumentasi : 

![image](https://github.com/user-attachments/assets/cbb99d51-0e73-45d0-a390-ded984f35594)


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

Contoh Dokumentasi hasil quarantine dengan dekripsi --decrypt sedang berjalan, maka hasil file yang telah diubah ke file karantina. Hasil file yang belum terdekripsi telah terdekripsi dengan hasil sebagai berikut

Dokumentasi : 

![image](https://github.com/user-attachments/assets/36c62256-76ac-4583-b495-d1fa77961768)


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
Dokumentasi : 
![image](https://github.com/user-attachments/assets/e80ccc86-9572-47e3-9777-f6d81af1eba4)

Diatas merupakan hasil pekerjaan --eradicate yang telah menghapus seluruh file yang berada di direktori quarantine.


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

Dokumentasi : ![image](https://github.com/user-attachments/assets/fb4e7704-d8bd-42bb-902f-a0d6e5aa1382)

Penjelasan : Diatas merupakan hasil dari activity log yang dimana program dekripsi diatas berhasil berhenti 

#  REVISI SOAL

a. Diperlukan revisi dikarenakan file yang telah terdownload tidak terjadi duplikasi file saat command /starterkit dijalankan. Saat selesai dilakukan revisi, file yang didownload telah bisa didownload dengan duplikat sebagai berikut :

- Pengerjaan Kode :

```bash
void fetch_and_extract() {
    char *get[] = {"wget", "-O", FILE_ZIP, URL_DOWNLOAD, NULL};
    run_process(get);


    const char *temp_dir = "tmp_extract";
    mkdir(temp_dir, 0755);

    
    char *unzip[] = {"unzip", "-q", FILE_ZIP, "-d", (char *)temp_dir, NULL};
    run_process(unzip);


    DIR *tmp = opendir(temp_dir);
    if (tmp) {
        struct dirent *entry;
        while ((entry = readdir(tmp)) != NULL) {
            if (entry->d_type == DT_REG) {
                char src[512], dst[512];
                snprintf(src, sizeof(src), "%s/%s", temp_dir, entry->d_name);

                
                snprintf(dst, sizeof(dst), "%s/%s", DIR_KIT, entry->d_name);
                int counter = 1;
                while (access(dst, F_OK) == 0) {
                    char newname[512];
                    
                    char *dot = strrchr(entry->d_name, '.');
                    if (dot) {
                        size_t base_len = dot - entry->d_name;
                        char base[256], ext[256];
                        strncpy(base, entry->d_name, base_len);
                        base[base_len] = '\0';
                        snprintf(ext, sizeof(ext), "%s", dot);
                        snprintf(newname, sizeof(newname), "%s/%s(%d)%s", DIR_KIT, base, counter++, ext);
                    } else {
                        snprintf(newname, sizeof(newname), "%s/%s(%d)", DIR_KIT, entry->d_name, counter++);
                    }
                    snprintf(dst, sizeof(dst), "%s", newname);
                }

                
                int in = open(src, O_RDONLY);
                int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (in != -1 && out != -1) {
                    char buf[4096];
                    ssize_t len;
                    while ((len = read(in, buf, sizeof(buf))) > 0) {
                        write(out, buf, len);
                    }
                    log_activity("%s - Extracted to %s.", entry->d_name, dst);
                    close(in);
                    close(out);
                }
            }
        }
        closedir(tmp);
    }

   
    char *rm_tmp[] = {"rm", "-rf", (char *)temp_dir, NULL};
    run_process(rm_tmp);
    remove(FILE_ZIP);
}

```

Dokumentasi : 

![image](https://github.com/user-attachments/assets/e7821057-9841-4828-bd16-5f46cf5df605)

Dokumentasi diatas merupakan bukti bahwa sudah terjadi pembetulan untuk pengerjaan kode soal bagian A.

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

Dokumentasi : ![image](https://github.com/user-attachments/assets/efcea73c-36d3-41cb-8241-596c5b6bc62f)


# Soal 4

Pengerjaan dari Oryza Qiara Ramadhani (084)

Fitur yang akan digunakan dalam debugmon ini:
- List Process yang akan menampilkan proses dari user bersama dengan tambahan informasi seperti CPU dan memori.
- Daemon mode yang akan menjalankan background program serta mencatat proses milik user.
- Stop daemon yang akan memberhentikan program.
- Fail daemon yang akan mematikan proses milik user.
- Revert daemon yang akan mengembalikan akses user setelah proses dimatikan.

Penyelesaian:

A. Mengetahui semua aktivitas user 
```bash
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
```
Menggunakan command `./debugmon list <user>`
![image](https://github.com/user-attachments/assets/7282206d-a5cc-413e-a12a-253de7e1fadf)
Dari sini dapat kita lihat bahwa setelah menjalankan command list user maka akan mengeluarkan output proses apa saja yang sedang berjalan.

B. Pembuatan proses background (Daemon)
```bash
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
                    debugmon_log(name, "RUNNING");
                }
            }
        }
        closedir(proc);
        sleep(5);
    }
}
```
Menggunakan command `./debugmon daemon <user>`
![image](https://github.com/user-attachments/assets/cd80abf4-4c59-40f5-a983-067d6cc6b31a)

C. Penghentian background process
```bash
void stop_daemon() {
    FILE *pf = fopen(PID_FILE, "r");
    if (!pf) { printf("Daemon tidak berjalan.\n"); return; }
    int pid;
    fscanf(pf, "%d", &pid);
    fclose(pf);
    if (kill(pid, SIGTERM) == 0) {
        remove(PID_FILE);
        printf("Daemon berhenti.\n");
    } else {
        perror("Gagal menghentikan daemon");
    }
}
```
Menggunakan command `./debugmon stop`
![image](https://github.com/user-attachments/assets/7c3b59be-86a8-4dcb-8b10-a15f5650ea11)

D. Mematikan background process
```bash
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
                debugmon_log(proc_name, "FAILED");
            } else {
                debugmon_log(proc_name, "KILL_FAILED");
            }
        }
    }
    closedir(proc);
}
```
Menggunakan command `./debugmon fail <user>`
![image](https://github.com/user-attachments/assets/b0b0d427-9ae4-492b-a25a-86cc6ec67914)
![image](https://github.com/user-attachments/assets/59494cb7-f773-488b-99a3-193756f3dd2e)

D. Mengembalikan akses user 
```bash
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

        char line[256], name[256] = "?", proc_name[256] = "?", mem[256] = "0";
        uid_t pid_uid = -1;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Uid:\t%u", &pid_uid)) continue;
            if (sscanf(line, "Name:\t%255s", name)) continue;
            if (sscanf(line, "VmRSS: %255[^\n]", mem)) break;
        }
        fclose(f);
        if (pid_uid == uid) {
            debugmon_log(name, "RUNNING");
        }
    }
    closedir(proc);
    printf("Akses user %s kembali seperti semula.\n", user);
}
```
Menggunakan command `./debugmon revert <user>`
![image](https://github.com/user-attachments/assets/b28a871e-bbcf-47e9-b67a-3e812a7255c9)

- Penyimpanan hasil proses yang telah dijalankan
```bash
void debugmon_log(const char *proc, const char *status){
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(log, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s\n",
            tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            proc, status);
    fclose(log);
}
```
![image](https://github.com/user-attachments/assets/e7d892df-f42a-4ce6-817b-53b6b0c4f7f7)

- Kumpulan argumen debugmon
```bash
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
```
