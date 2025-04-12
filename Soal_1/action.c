#include <zip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <curl/curl.h>

#define MAX_FILENAME 256
#define MAX_FILES 100

typedef struct {
    char name[MAX_FILENAME];
    int is_number;
    int value;
} FileInfo;

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

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

// Fungsi untuk memeriksa apakah nama file valid untuk filter (1 huruf atau 1 angka)
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

// Fungsi untuk bagian c: Gabungkan konten file (diperbaiki)
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
// Fungsi untuk decode ROT13
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        // Jika tidak ada argumen, jalankan download clues
        download_clues();
    } else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filter_files();
        } else if (strcmp(argv[2], "Combine") == 0) {
            combine_files();
        } else if (strcmp(argv[2], "Decode") == 0) {
            decode_file();
        } else {
            printf("Argumen tidak valid. Gunakan salah satu dari:\n");
            printf("./action -m Filter\n");
            printf("./action -m Combine\n");
            printf("./action -m Decode\n");
        }
    } else {
        printf("Penggunaan:\n");
        printf("./action                - Untuk mendownload dan mengekstrak clues\n");
        printf("./action -m Filter     - Untuk memfilter file\n");
        printf("./action -m Combine    - Untuk menggabungkan file\n");
        printf("./action -m Decode     - Untuk mendecode file\n");
    }

    return 0;
}
