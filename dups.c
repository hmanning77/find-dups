#include<stdlib.h>
#include<stdio.h>
#include<dirent.h>
#include<string.h>

#include<openssl/evp.h>

#define BUFSIZE 4096

typedef struct FileInfo {
    char *filename;
    unsigned long size;
    char *hash;
    struct FileInfo *next;
} FileInfo;

typedef struct FileInfoList {
    struct FileInfo *head;
} FileInfoList;

typedef struct HashIndexEntry {
    char *hash;
    struct FileInfo *files;
    struct HashIndexEntry *next;
} HashIndexEntry;

typedef struct HashIndex {
    struct HashIndexEntry *head;
} HashIndex;

/*
 * Function to walk a directory tree and populate a list of file names and
 * sizes.
 */
void collect_files(const char *dirname, FileInfoList *list) {
    DIR *target_dir = opendir(dirname);
    struct dirent *next_entry;
    while (next_entry = readdir(target_dir)) {
        /* Calculate the full name relative name of the file or directory */
        size_t memsize = strlen(dirname) + 1 + strlen(next_entry->d_name) + 1;
        char *full_name = (char *)malloc(memsize);
        memset(full_name, 0, memsize);
        strcpy(full_name, dirname);
        strcat(full_name, "/");
        strcat(full_name, next_entry->d_name);

        if (next_entry->d_type == DT_REG) {
            FileInfo *new_entry = (FileInfo *)malloc(sizeof(FileInfo));
            memset(new_entry, 0, sizeof(FileInfo));

            new_entry->filename = full_name;

            /* Get file size */
            FILE *f = fopen(full_name, "r");
            fseek(f, 0L, SEEK_END);
            new_entry->size = ftell(f);
            fclose(f);

            new_entry->hash = NULL;
            new_entry->next = NULL;

            /* Add the new entry to the list */
            if (list->head == NULL) {
                list->head = new_entry;
            } else {
                FileInfo *next_entry = list->head;
                while (next_entry->next != NULL) {
                    next_entry = next_entry->next;
                }
                next_entry->next = new_entry;
            }
        } else if (next_entry->d_type == DT_DIR) {
            /* Don't recurse on the current or parent directories */
            if (strcmp(next_entry->d_name, ".") != 0 &&
                strcmp(next_entry->d_name, "..") != 0) {
                collect_files(full_name, list);
            }
            free(full_name);  /* Directory names don't get saved in the list */
        }
    }
    closedir(target_dir);
}

void calculate_hashes(FileInfoList *files) {
    const EVP_MD *md = EVP_sha1();
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    FileInfo *entry = files->head;
    while (entry != NULL) {
        EVP_DigestInit_ex(mdctx, md, NULL);

        /* Read file into digester */
        FILE *f = fopen(entry->filename, "r");
        unsigned char buffer[BUFSIZE];
        size_t bytes_read = fread(buffer, 1, BUFSIZE, f);
        while (bytes_read > 0) {
            EVP_DigestUpdate(mdctx, buffer, bytes_read);
            bytes_read = fread(buffer, 1, BUFSIZE, f);
        }

        /* Read digest, format it, and copy it into info struct */
        int hash_len = 0;
        EVP_DigestFinal_ex(mdctx, buffer, &hash_len);
        size_t memsize = hash_len * 2 + 1;
        char *hash_str = (char *)malloc(memsize);
        memset(hash_str, 0, memsize);
        char formatted[3];
        for (int i=0; i < hash_len; i++) {
            sprintf(formatted, "%02x", buffer[i]);
            if (i == 0) {
                strcpy(hash_str, formatted);
            } else {
                strcat(hash_str, formatted);
            }
        }
        hash_str[hash_len * 2] = '\0';
        entry->hash = hash_str;

        entry = entry->next;
    }
    EVP_MD_CTX_destroy(mdctx);
    EVP_cleanup();
}

void add_file_to_index_entry(HashIndexEntry *target, FileInfo *file) {
    FileInfo *new_entry = (FileInfo *)malloc(sizeof(FileInfo));
    memset(new_entry, 0, sizeof(FileInfo));
    memcpy(new_entry, file, sizeof(FileInfo));
    new_entry->next = NULL;
    if (target->files == NULL) {
        target->files = new_entry;
    } else {
        FileInfo *current_file = target->files;
        while (current_file->next != NULL) {
            current_file = current_file->next;
        }
        current_file->next = new_entry;
    }
}

HashIndex *generate_index(FileInfoList *files) {
    HashIndex *index = (HashIndex *)malloc(sizeof(HashIndex));
    memset(index, 0, sizeof(HashIndex));
    index->head = NULL;

    FileInfo *file = files->head;
    while (file != NULL) {
        HashIndexEntry *current_hash = index->head;
        /* Edge case, first entry */
        if (current_hash == NULL) {
            HashIndexEntry *new_entry = (HashIndexEntry *)malloc(sizeof(HashIndexEntry));
            memset(new_entry, 0, sizeof(HashIndexEntry));

            new_entry->hash = file->hash;
            add_file_to_index_entry(new_entry, file);
            new_entry->next = NULL;

            index->head = new_entry;
            file = file->next;
            continue;
        }

        while (current_hash != NULL) {
            if (strcmp(current_hash->hash, file->hash) == 0) {
                add_file_to_index_entry(current_hash, file);
                break;
            } else if (current_hash->next == NULL) {
                HashIndexEntry *new_entry = (HashIndexEntry *)malloc(sizeof(HashIndexEntry));
                memset(new_entry, 0, sizeof(HashIndexEntry));
                new_entry->hash = file->hash;
                add_file_to_index_entry(new_entry, file);
                new_entry->next = NULL;
                current_hash->next = new_entry;
                break;
            }
            current_hash = current_hash->next;
        }
        file = file->next;
    }

    return index;
}

void print_duplicates(HashIndex *index) {
    HashIndexEntry *current_entry = index->head;
    unsigned long diskspace = 0;
    while (current_entry != NULL) {
        FileInfo *current_file = current_entry->files;
        if (current_file->next != NULL) {
            printf("%s\n", current_entry->hash);
            while (current_file != NULL) {
                printf("\t%s\n", current_file->filename);
                diskspace += current_file->size;
                current_file = current_file->next;
            }
            printf("\n");
        }
        current_entry = current_entry->next;
    }
    if (diskspace > 0) {
        printf("%i bytes wasted space\n", diskspace);
    }
}

void print_filenames(FileInfoList *files) {
    /* Test code: print out the filenames of all files found */
    FileInfo *entry = files->head;
    while (entry != NULL) {
        printf("%s\n", entry->filename);
        entry = entry->next;
    }
}

void print_hashes(FileInfoList *files) {
    /* Test code: Print out the hash of each filename */
    FileInfo *entry = files->head;
    while (entry != NULL) {
        printf("%s -- %s\n", entry->filename, entry->hash);
        entry = entry->next;
    }
}

void print_hash_index(HashIndex *index) {
    /* Test code: print out the hash index */
    HashIndexEntry *current_entry = index->head;
    while (current_entry != NULL) {
        printf("%s\n", current_entry->hash);
        FileInfo *current_file = current_entry->files;
        while (current_file != NULL) {
            printf("\t%s\n", current_file->filename);
            current_file = current_file->next;
        }
        printf("\n");
        current_entry = current_entry->next;
    }
}

int main(int argc, char** argv) {
    /* Initialise file list */
    FileInfoList *files = (FileInfoList *)malloc(sizeof(FileInfoList));
    memset(files, 0, sizeof(FileInfoList));
    files->head == NULL;

    /* Get list of all files in the directory */
    collect_files(".", files);

    /* Index the files by their hash */
    calculate_hashes(files);
    HashIndex *index = generate_index(files);

    /* print_filenames(files); */
    /* print_hashes(files); */
    /* print_hash_index(index); */
    print_duplicates(index);

    return 0;
}
