#include "minigit.h"

// Add/stage a file for commit
int cmd_add(const char *filepath) {
    // Check if repository is already initialized
    if (!directory_exists(MINIGIT_DIR)) {
        printf("Error: Not a minigit repository. Run 'minigit init' first.\n");
        return -1;
    }
    
    if (!file_exists(filepath)) {
        printf("Error: File '%s' does not exist.\n", filepath);
        return -1;
    }
    
    // Stage the file
    if (stage_file(filepath) != 0) {
        printf("Error: Failed to stage file '%s'.\n", filepath);
        return -1;
    }
    
    printf("Added '%s' to staging area.\n", filepath);
    return 0;
}

int stage_file(const char *filepath) {
    char hash[SHA1_HEX_LENGTH];
    char *content;
    size_t content_size;
    
    // Compute hash of file
    if (compute_sha1(filepath, hash) != 0) {
        fprintf(stderr, "Error computing hash for '%s'\n", filepath);
        return -1;
    }
    
    // Read content
    if (read_file(filepath, &content, &content_size) != 0) {
        fprintf(stderr, "Error reading file '%s'\n", filepath);
        return -1;
    }
    
    // Store object in obj directory
    if (store_object(hash, content, content_size) != 0) {
        fprintf(stderr, "Error storing object for '%s'\n", filepath);
        free(content);
        return -1;
    }
    
    free(content);
    
    // Update index
    if (update_index(filepath, hash) != 0) {
        fprintf(stderr, "Error updating index for '%s'\n", filepath);
        return -1;
    }
    
    return 0;
}

// Store file content as object
int store_object(const char *hash, const char *content, size_t content_size) {
    char object_path[MAX_PATH_LENGTH];
    sprintf(object_path, "%s/%s", OBJECTS_DIR, hash);
    
    if (file_exists(object_path)) {
        return 0;
    }
    
    return write_file(object_path, content, content_size);
}

// Update index with file path and hash
int update_index(const char *filepath, const char *hash) {
    IndexEntry *entries = NULL;
    int count = 0;
    int found = 0;
    
    // Read current index
    if (read_index(&entries, &count) != 0) {
        return -1;
    }
    
    // Check if file is already in index and update it
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].path, filepath) == 0) {
            strncpy(entries[i].hash, hash, SHA1_HEX_LENGTH - 1);
            entries[i].hash[SHA1_HEX_LENGTH - 1] = '\0';
            found = 1;
            break;
        }
    }
    
    // Otherwise add new it
    if (!found) {
        entries = realloc(entries, (count + 1) * sizeof(IndexEntry));
        if (!entries) {
            return -1;
        }
        
        strncpy(entries[count].path, filepath, MAX_PATH_LENGTH - 1);
        entries[count].path[MAX_PATH_LENGTH - 1] = '\0';
        strncpy(entries[count].hash, hash, SHA1_HEX_LENGTH - 1);
        entries[count].hash[SHA1_HEX_LENGTH - 1] = '\0';
        count++;
    }
    
    // Write updated index
    int result = write_index(entries, count);
    
    if (entries) {
        free(entries);
    }
    
    return result;
} 