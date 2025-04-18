# Sisop-2-2025-IT29

- Bayu Kurniawan - 5027241055
- Angga Firmansyah - 5027241062
- Oryza Qiara Ramadhani - 5027241084

# Soal 1

Dikerjakan oleh : Bayu Kurniawan

a. Downloading the Clues 
Karena kamu telah diberikan sebuah link Clues oleh Cyrus, kamu membuat file bernama action.c yang dimana kalau dijalankan seperti biasa tanpa argumen tambahan akan mendownload dan meng-unzip file tersebut secara langsung. Saat kamu melihat isi dari Clues tersebut, isinya berupa 4 folder yakni ClueA - ClueD dan di masing-masing folder tersebut terdapat .txt files dan isinya masih tidak jelas, mungkin beberapa kata di dalam .txt file yang dapat dicari di inventory website? (Note: inventory bersifat untuk seru-seruan saja). Jangan lupa untuk menghapus Clues.zip setelah diekstrak dan buatlah apabila folder Clues sudah ada, maka command tersebut tidak akan mendownload Clues.zip lagi apabila dijalankan. 

```bash
int download_file(const char *url, const char *output_filename) {
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(output_filename, "wb");
        if (!fp) {
            perror("Failed to open file for writing");
            return 1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        fclose(fp);
        curl_easy_cleanup(curl);
    }
    return (res == CURLE_OK) ? 0 : 1;
}

void unzip_file(const char *zip_filename) {
    int err = 0;
    struct zip *archive = zip_open(zip_filename, 0, &err);

    if (!archive) {
        fprintf(stderr, "Gagal membuka file zip: %s\n", zip_strerror(archive));
        return;
    }

    int num_entries = zip_get_num_entries(archive, 0);
    for (int i = 0; i < num_entries; i++) {
        const char *name = zip_get_name(archive, i, 0);
        if (!name) continue;

        // Buat direktori jika diperlukan
        char *dir = strdup(name);
        char *last_slash = strrchr(dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            mkdir_p(dir);  // Fungsi pembantu untuk membuat direktori
            free(dir);
        }

        struct zip_file *file = zip_fopen_index(archive, i, 0);
        if (!file) {
            fprintf(stderr, "Gagal membuka file %s dalam zip\n", name);
            continue;
        }

        FILE *out = fopen(name, "wb");
        if (!out) {
            fprintf(stderr, "", name);
            zip_fclose(file);
            continue;
        }        char buf[8192];
        zip_int64_t n;
        while ((n = zip_fread(file, buf, sizeof(buf))) > 0) {
            fwrite(buf, 1, n, out);
        }

        fclose(out);
        zip_fclose(file);
    }

    zip_close(archive);
}

void mkdir_p(const char *dir) {
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}
// Fungsi untuk memeriksa apakah direktori ada
int dir_exists(const char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}

void download_clues() {
    if (dir_exists("Clues")) {
        printf("Folder Clues sudah ada. Tidak perlu mendownload lagi.\n");
        return;
    }

    const char *url = "https://drive.google.com/uc?export=download&id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK";
    const char *zip_filename = "Clues.zip";

    printf("Mendownload Clues.zip...\n");
    if (download_file(url, zip_filename) == 0) {
        printf("Mengekstrak Clues.zip...\n");
        unzip_file(zip_filename);
        remove(zip_filename);
        printf("Clues.zip telah dihapus setelah diekstrak.\n");
    } else {
        printf("Gagal mendownload Clues.zip\n");
    }
}
```

pada kode tersebut terdapat beberapa fungsi seperti `download_file`, `unzip_file`, dan `mkdir_p` yang berfungsi untuk mendownload, unzip dan membuat direktori untuk menyimpan hasil ekstrak dari file yang telah di download, kemudian fungsi tersebut digabungkan melalui fungsi `download_clues`. saat dijalankan menggunakan `./action` pada terminal akan menghasilkan output sebagai berikut:
![image](https://github.com/user-attachments/assets/0a73348b-a14d-49ee-a917-7511ac959dc7)

b. Filtering the Files 
Karena kebanyakan dari file tersebut berawal dengan 1 huruf atau angka, kamu pun mencoba untuk memindahkan file-file yang hanya dinamakan dengan 1 huruf dan 1 angka tanpa special character kedalam folder bernama Filtered. Kamu tidak suka kalau terdapat banyak clue yang tidak berguna jadi disaat melakukan filtering, file yang tidak terfilter dihapus. Karena kamu tidak ingin membuat file kode lagi untuk filtering, maka kamu menggunakan file sebelumnya untuk filtering file-file tadi dengan menambahkan argumen saat ingin enjalankan action.c

```bash
int is_valid_filter_filename(const char *filename) {
    if (strlen(filename) != 5) return 0; // "x.txt" = 5 karakter
    if (filename[1] != '.' || strcmp(filename + 2, "txt") != 0) {
        return 0;
    }

    char first_char = filename[0];
    return (isdigit(first_char) || (isalpha(first_char) && islower(first_char)));
}

void filter_files() {
    DIR *dir;
    struct dirent *ent;
    char subdirs[4][10] = {"ClueA", "ClueB", "ClueC", "ClueD"};

    // Buat folder Filtered jika belum ada
    if (!dir_exists("Filtered")) {
        mkdir("Filtered", 0755);
    }

    for (int i = 0; i < 4; i++) {
        char path[MAX_FILENAME];
        snprintf(path, sizeof(path), "Clues/%s", subdirs[i]);

        if ((dir = opendir(path)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_REG) {
                    char *filename = ent->d_name;
                    if (is_valid_filter_filename(filename)) {
                        // Pindahkan file ke folder Filtered
                        char src_path[MAX_FILENAME];
                        char dest_path[MAX_FILENAME];

                        snprintf(src_path, sizeof(src_path), "%s/%s", path, filename);
                        snprintf(dest_path, sizeof(dest_path), "Filtered/%s", filename);

                        rename(src_path, dest_path);
                    } else {
                        // Hapus file yang tidak valid
                        char file_path[MAX_FILENAME];
                        snprintf(file_path, sizeof(file_path), "%s/%s", path, filename);
                        remove(file_path);
                    }
                }
            }
            closedir(dir);
        }
    }
    printf("File telah difilter dan disimpan di folder Filtered.\n");
}

// Fungsi pembanding untuk sorting file
int compare_files(const void *a, const void *b) {
    const FileInfo *fileA = (const FileInfo *)a;
    const FileInfo *fileB = (const FileInfo *)b;
    return fileA->value - fileB->value;
}
```
Pada fungsi diatas terdapat fungsi yang digunakan untuk melakukan filter clue-clue yang diperlukan serta untuk menghapus clue yang tidak diperlukan. Fungsi `compare_file` berfungsi untuk membandingan 2 file untuk di sorting. jika dijalankan menggunakan command `./action -m Filter` di terminal maka akan menghasilkan tampilan sebagai berikut:
![image](https://github.com/user-attachments/assets/2028d83e-bd2e-4ee9-8ac4-4efdfdeb64f5)

c. Combine the File Content 
Di setiap file .txt yang telah difilter terdapat satu huruf dan agar terdapat progress, Cyrus memberikan clue tambahan untuk meletakan/redirect isi dari setiap .txt file tersebut kedalam satu file yaitu Combined.txt dengan menggunakan FILE pointer. Tetapi, terdapat urutan khusus saat redirect isi dari .txt tersebut, yaitu urutannya bergantian dari .txt dengan nama angka lalu huruf lalu angka lagi lalu huruf lagi. Lalu semua file .txt sebelumnya dihapus. Seperti halnya tadi, agar efisien kamu ingin menjalankan action.c dengan argumen tambahan. 

```bash
void combine_files() {
    DIR *dir;
    struct dirent *ent;
    FileInfo number_files[MAX_FILES];
    FileInfo letter_files[MAX_FILES];
    int num_count = 0;
    int let_count = 0;

    if ((dir = opendir("Filtered")) == NULL) {
        printf("Folder Filtered tidak ditemukan.\n");
        return;
    }
// Pisahkan file angka dan huruf
    while ((ent = readdir(dir)) != NULL && (num_count < MAX_FILES || let_count < MAX_FILES)) {
        if (ent->d_type == DT_REG) {
            char *filename = ent->d_name;
            if (strlen(filename) >= 5 && strcmp(filename + strlen(filename) - 4, ".txt") == 0) {
                char first_char = filename[0];

                if (isdigit(first_char)) {
                    strncpy(number_files[num_count].name, filename, MAX_FILENAME);
                    number_files[num_count].value = first_char - '0';
                    num_count++;
                } else if (isalpha(first_char)) {
                    strncpy(letter_files[let_count].name, filename, MAX_FILENAME);
                    letter_files[let_count].value = first_char - 'a';
                    let_count++;
                }
            }
        }
    }
    closedir(dir);

    // Urutkan file angka dan huruf secara terpisah
    qsort(number_files, num_count, sizeof(FileInfo), compare_files);
    qsort(letter_files, let_count, sizeof(FileInfo), compare_files);

    // Gabungkan isi file dengan urutan bergantian
    FILE *combined = fopen("Combined.txt", "w");
    if (!combined) {
        perror("Gagal membuat file Combined.txt");
        return;
    }
int max_count = (num_count > let_count) ? num_count : let_count;
    for (int i = 0; i < max_count; i++) {
        // Ambil dari angka dulu jika ada
        if (i < num_count) {
            char filepath[MAX_FILENAME];
            snprintf(filepath, sizeof(filepath), "Filtered/%s", number_files[i].name);

            FILE *file = fopen(filepath, "r");
            if (file) {
                char content[2];
                if (fgets(content, sizeof(content), file)) {
                    fputc(content[0], combined);
                }
                fclose(file);
                remove(filepath);
            }
        }

        // Kemudian ambil dari huruf jika ada
        if (i < let_count) {
            char filepath[MAX_FILENAME];
            snprintf(filepath, sizeof(filepath), "Filtered/%s", letter_files[i].name);

            FILE *file = fopen(filepath, "r");
            if (file) {
                char content[2];
                if (fgets(content, sizeof(content), file)) {
                    fputc(content[0], combined);
                }
                fclose(file);
                remove(filepath);
            }
        }
    }
    fclose(combined);
    printf("Isi file telah digabungkan ke Combined.txt dengan urutan yang benar.\n");
}
```
Fungsi diatas digunakan untuk menggabungkan clue clue yang ada kedalam file Combined.txt, penggabungan ini dilakukan secara bergantian antara file berisi huruf dan angka. Jika command `./action -m Combine`dijalankan pada terminal akan menghasilkan keluaran sebagai berikut.
![image](https://github.com/user-attachments/assets/aae4ff94-85e6-4cef-a238-3f185e6a71c0)

jika dicek isi file `Combined.txt` maka didalamnya akan terdapat isi sebagai berikut.
![image](https://github.com/user-attachments/assets/12b7144d-0dae-4219-89fc-f0e2459b956c)

d. Decode the file 
Karena isi Combined.txt merupakan string yang random, kamu  memiliki ide untuk menggunakan Rot13 untuk decode string tersebut dan meletakan hasil dari yang telah di-decode tadi kedalam file bernama Decoded.txt. Jalankan file action.c dengan argumen tambahan untuk Proses decoding ini. 

```bash
void rot13_decode(const char *input, char *output) {
    for (int i = 0; input[i]; i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z') {
            output[i] = 'a' + ((c - 'a' + 13) % 26);
        } else if (c >= 'A' && c <= 'Z') {
            output[i] = 'A' + ((c - 'A' + 13) % 26);
        } else {
            output[i] = c;
        }
    }
}

// Fungsi untuk bagian d: Decode file
void decode_file() {
    FILE *combined = fopen("Combined.txt", "r");
    if (!combined) {
        printf("File Combined.txt tidak ditemukan.\n");
        return;
    }

    // Baca isi file
    char content[1024] = {0};
    fgets(content, sizeof(content), combined);
    fclose(combined);

    // Decode dengan ROT13
    char decoded[1024] = {0};
    rot13_decode(content, decoded);

    // Simpan hasil decode
    FILE *decoded_file = fopen("Decoded.txt", "w");
    if (decoded_file) {
        fputs(decoded, decoded_file);
        fclose(decoded_file);
        printf("File telah didecode dan disimpan sebagai Decoded.txt.\n");
    } else {
        perror("Gagal membuat file Decoded.txt");
    }
}
```

Fungsi diatas digunakan untuk melakukan decode pada file Combined.txt menggunakan rot13. jika dijalankan menggunakan command `./action -m Docode` maka akan menghasilkan keluaran file baru bernama `Decoded.txt`
![image](https://github.com/user-attachments/assets/285e0e88-2406-4b36-876a-5acb4525e399)

dan isi dari file `Decoded.txt` merupakan password yang diperlukan untuk menyelesaikan soal ini.
![image](https://github.com/user-attachments/assets/3376b06a-2fe8-4714-b724-1292ce9b25ca)


e. Password Check 
Karena kamu sudah mendapatkan password tersebut, kamu mencoba untuk mengecek apakah password yang sudah kamu dapatkan itu benar atau tidak dengan cara di-input ke lokasi tadi.
![image](https://github.com/user-attachments/assets/830cab91-cfd3-4727-a0a5-cba01ce13f56)
![image](https://github.com/user-attachments/assets/af1fb978-626c-49df-8052-3620a0135f14)




# Soal 2

Dikerjakan oleh : Angga Firmansyah

# Library yang Digunakan
- stdio.h: Untuk input/output dasar seperti fopen, fprintf, fgets.
- stdlib.h: Untuk manajemen memori dan proses seperti malloc, exit, popen.
- string.h: Untuk manipulasi string seperti strlen, strchr, strcspn.
- unistd.h: Untuk manajemen proses dan file seperti fork, execvp, umask, setsid.
- sys/types.h: Untuk tipe data terkait proses seperti pid_t.
- sys/stat.h: Untuk pemeriksaan status file seperti mkdir, stat.
- dirent.h: Untuk membaca direktori menggunakan opendir, readdir, closedir.

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
- wget digunakan untuk mendownload file zip dari URL.
- unzip digunakan untuk mengekstrak isi zip ke dalam folder yang telah ditentukan.
- remove(FILE_ZIP) menghapus file zip setelah ekstraksi selesai

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
- setsid() digunakan untuk membuat proses menjadi daemon.
- umask(0) mengatur mask untuk hak akses file.
- Proses daemon ini berjalan terus-menerus untuk mendekripsi dan memindahkan file.

- valid_base64 & decode_b64
- Fungsi valid_base64 memastikan bahwa nama file valid sebagai Base64, sementara decode_b64 digunakan untuk mendekripsi nama file tersebut.
- fork() digunakan untuk membuat proses anak.
- execvp() digunakan untuk menjalankan perintah shell yang diberikan dalam args.
- waitpid() digunakan untuk menunggu proses anak selesai sebelum melanjutkan.
- Dengan menggunakan algoritma base64 untung melakukan dekripsi file starter_kit yang akan dipindahkan. Berikut adalah contoh dokumentasi hasil menyalakan dekripsi dengan bukti activity.log :

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

Dokumentasi : 

![image](https://github.com/user-attachments/assets/6a618f6b-ecb4-4984-82fa-cfaa8ebe7e89)

Error handling ketika command program yang dijalankan memiliki kesalahan penulisan maka akan muncul notifikasi penggunaan command yang benar.

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

Dari sini dapat kita lihat bahwa setelah menjalankan command list user maka akan mengeluarkan output proses apa saja yang sedang berjalan. Dalam list tersebut akan memuat PID, nama proses, penggunaan CPU dan juga memori. Empat informasi proses itu akan diperoleh dari `/proc/<pid>/status` dan `/proc/<pid>/stat`.

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

Fitur ini akan mengaktifkan proses debugmon yang akan berjalan di background lalu `sleep(5)` ini akan memantau semua prosesnya secara berkala tiap 5 detik. Dan disetiap proses baru akan disimpan pada debugmon.log dengan status `RUNNING`.

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

Fitur ini akan menghentikan daemon yang sedang berjalan. Program akan membaca PID dari file debugmon.pid dan mengirim sinyal SIGTERM ke proses tersebut. Jika berhasil, file PID akan dihapus dan pesan bahwa daemon telah berhenti akan ditampilkan.

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

Perintah ini akan menargetkannya pada proses seperti sh, bash, node dan debugmon lalu apabila sudah berhasil dihentikan maka akan dicatat pada debugmon.log sehingga status akan berubah menjadi `FAILED`.

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

Saat command revert dijalankan, proses yang sebelumnya sudah dimatikan akan diaktifkan kembali menjadi ke semula dan status nya pun akan kembali menjadi `RUNNING`.

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

Semua proses akan tercatat pada debugmon.log ini dengan format `[DD:MM:YYYY]-[HH:MM:SS]_NAMAPROSES_STATUS`

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
Pada bagian ini akan memeriksa apakah command yang kita ketik itu sudah sesuai dengan format, apabila tidak sesuai maka akan keluar pesan bahwa `Perintah tidak valid`

# REVISI

Dari yang sebelumnya pada saat kita memanggil file debugmon.log itu hanya akan menampilkan proses nya sedang dalam tahap running atau failed saja, namun setelah direvisi akan ada tambahan informasi sedang atau sehabis memberikan perintah "stop", "fail" atau "revert" pada terminal tersebut.

- Ini pada saat kita menjalankan `./debugmon stop`
  ![image](https://github.com/user-attachments/assets/857ea388-0fbf-4685-8118-098cf4de9cb7)

- Ini pada saat kita mematikan background process
  ![image](https://github.com/user-attachments/assets/c3272a94-abc2-4b3f-8bac-c6de0a1bebd7)

- Ini apabila kita sedang memerintah untuk mengembalikan akses ke semula (REVERT)
![image](https://github.com/user-attachments/assets/70092ceb-2727-4d08-bb38-6103207423ba)
