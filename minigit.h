#ifndef MINIGIT_H
#define MINIGIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

// Constants
#define MINIGIT_DIR ".minigit"
#define OBJECTS_DIR ".minigit/objects"
#define INDEX_FILE ".minigit/index"
#define HEAD_FILE ".minigit/HEAD"
#define SHA1_DIGEST_LENGTH 20
#define SHA1_HEX_LENGTH 41
#define MAX_PATH_LENGTH 1024
#define MAX_MESSAGE_LENGTH 1024

// Structures
typedef struct {
    char path[MAX_PATH_LENGTH];
    char hash[SHA1_HEX_LENGTH];
} IndexEntry;

typedef struct {
    char hash[SHA1_HEX_LENGTH];
    time_t timestamp;
    char message[MAX_MESSAGE_LENGTH];
    int file_count;
    IndexEntry *files;
} Commit;


// Init functions
int cmd_init(void);
int create_minigit_directory(void);

// Add functions
int cmd_add(const char *filepath);
int stage_file(const char *filepath);
int compute_sha1(const char *filepath, char *hash_output);
int store_object(const char *hash, const char *content, size_t content_size);
int update_index(const char *filepath, const char *hash);

// Commit functions

// Status functions

// Util functions
int file_exists(const char *filepath);
int directory_exists(const char *dirpath);
int create_directory(const char *dirpath);
int read_file(const char *filepath, char **content, size_t *size);
int write_file(const char *filepath, const char *content, size_t size);
int get_current_files(char ***files, int *count);
int read_index(IndexEntry **entries, int *count);
int write_index(IndexEntry *entries, int count);
int read_head(char *commit_hash);
int read_commit(const char *commit_hash, Commit *commit);
void sha1_to_hex(unsigned char *sha1, char *hex);
void hex_to_sha1(const char *hex, unsigned char *sha1);

// Super simple SHA-1 implementation
void simple_sha1(const unsigned char *data, size_t len, unsigned char *hash);

#endif // MINIGIT_H 