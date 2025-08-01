#include "minigit.h"

// Check if file exists
int file_exists(const char *filepath) {
    struct stat st;
    return (stat(filepath, &st) == 0 && S_ISREG(st.st_mode));
}

// Check if directory exists
int directory_exists(const char *dirpath) {
    struct stat st;
    return (stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode));
}

// Create directory (with parents if needed)
int create_directory(const char *dirpath) {
    char temp[MAX_PATH_LENGTH];
    char *p = NULL;
    size_t len;

    strcpy(temp, dirpath);
    len = strlen(temp);
    if (len > 0 && temp[len - 1] == '/')
        temp[len - 1] = 0;
    
    for (p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (!directory_exists(temp)) {
                if (mkdir(temp, 0755) != 0) {
                    return -1;
                }
            }
            *p = '/';
        }
    }
    
    if (!directory_exists(temp)) {
        if (mkdir(temp, 0755) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// Read entire file into memory
int read_file(const char *filepath, char **content, size_t *size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        return -1;
    }
    
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    *content = malloc(*size + 1);
    
    fread(*content, 1, *size, file);
    (*content)[*size] = '\0';
    fclose(file);
    
    return 0;
}

// Write content to file
int write_file(const char *filepath, const char *content, size_t size) {
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        return -1;
    }
    
    size_t written = fwrite(content, 1, size, file);
    fclose(file);
    
    if (written != size) {
        return -1;
    }
    return 0;
}

// Get list of files in current directory (excluding .minigit)
int get_current_files(char ***files, int *count) {
    DIR *dir;
    struct dirent *entry;
    char **file_list = NULL;
    int file_count = 0;
    int capacity = 10;
    
    dir = opendir(".");
    if (!dir) {
        return -1;
    }
    
    file_list = malloc(capacity * sizeof(char*));
    if (!file_list) {
        closedir(dir);
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0 &&
            strncmp(entry->d_name, ".minigit", 8) != 0) {
            
            if (file_count >= capacity) {
                capacity *= 2;
                file_list = realloc(file_list, capacity * sizeof(char*));
                if (!file_list) {
                    closedir(dir);
                    return -1;
                }
            }
            
            file_list[file_count] = malloc(strlen(entry->d_name) + 1);
            strcpy(file_list[file_count], entry->d_name);
            file_count++;
        }
    }
    
    closedir(dir);
    *files = file_list;
    *count = file_count;
    return 0;
}

// Read index file
int read_index(IndexEntry **entries, int *count) {
    FILE *file = fopen(INDEX_FILE, "r");
    if (!file) {
        *entries = NULL;
        *count = 0;
        return 0; // Empty index is OK
    }
    
    IndexEntry *entry_list = NULL;
    int entry_count = 0;
    int capacity = 10;
    char line[MAX_PATH_LENGTH + SHA1_HEX_LENGTH + 10];
    
    entry_list = malloc(capacity * sizeof(IndexEntry));
    if (!entry_list) {
        fclose(file);
        return -1;
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (entry_count >= capacity) {
            capacity *= 2;
            entry_list = realloc(entry_list, capacity * sizeof(IndexEntry));
            if (!entry_list) {
                fclose(file);
                return -1;
            }
        }
        
        char *token = strtok(line, " ");
        if (token) {
            strncpy(entry_list[entry_count].hash, token, SHA1_HEX_LENGTH - 1);
            entry_list[entry_count].hash[SHA1_HEX_LENGTH - 1] = '\0';
            
            token = strtok(NULL, "\n");
            if (token) {
                strncpy(entry_list[entry_count].path, token, MAX_PATH_LENGTH - 1);
                entry_list[entry_count].path[MAX_PATH_LENGTH - 1] = '\0';
                entry_count++;
            }
        }
    }
    
    fclose(file);
    *entries = entry_list;
    *count = entry_count;
    return 0;
}

// Write index file
int write_index(IndexEntry *entries, int count) {
    FILE *file = fopen(INDEX_FILE, "w");
    if (!file) {
        return -1;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %s\n", entries[i].hash, entries[i].path);
    }
    
    fclose(file);
    return 0;
}

// Read HEAD file
int read_head(char *commit_hash) {
    FILE *file = fopen(HEAD_FILE, "r");
    if (!file) {
        commit_hash[0] = '\0';
        return 0; // No HEAD is ok for new repo
    }
    
    if (fgets(commit_hash, SHA1_HEX_LENGTH, file)) {
        // Remove newline if present
        char *newline = strchr(commit_hash, '\n');
        if (newline) *newline = '\0';
        fclose(file);
        return 0;
    }
    
    fclose(file);
    commit_hash[0] = '\0';
    return -1;
}

// Convert SHA-1 binary to hex string
void sha1_to_hex(unsigned char *sha1, char *hex) {
    for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
        sprintf(hex + i * 2, "%02x", sha1[i]);
    }
    hex[SHA1_HEX_LENGTH - 1] = '\0';
}

// Convert hex string to SHA-1 binary
void hex_to_sha1(const char *hex, unsigned char *sha1) {
    for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
        sscanf(hex + i * 2, "%2hhx", &sha1[i]);
    }
}

// Simplified SHA-1 implementation (pretty basic version)
void simple_sha1(const unsigned char *data, size_t len, unsigned char *hash) {    
    // Initialize hash to zeros
    memset(hash, 0, SHA1_DIGEST_LENGTH);
    
    // XOR all bytes with positional values
    for (size_t i = 0; i < len; i++) {
        hash[i % SHA1_DIGEST_LENGTH] ^= data[i] + (i & 0xFF);
    }
    
    // Mix the hash a bit more
    for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
        hash[i] = (hash[i] * 131 + len) & 0xFF;
    }
}

// Compute SHA-1 hash of file
int compute_sha1(const char *filepath, char *hash_output) {
    char *content;
    size_t size;
    
    if (read_file(filepath, &content, &size) != 0) {
        return -1;
    }
    
    unsigned char binary_hash[SHA1_DIGEST_LENGTH];
    simple_sha1((unsigned char*)content, size, binary_hash);
    sha1_to_hex(binary_hash, hash_output);
    
    free(content);
    return 0;
} 