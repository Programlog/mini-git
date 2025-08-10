#include "minigit.h"

// Create a commit from staged changes
int cmd_commit(const char *message) {
    char commit_hash[SHA1_HEX_LENGTH];
    IndexEntry *entries = NULL;
    int count = 0;
    
    // Check if repository is initialized
    if (!directory_exists(MINIGIT_DIR)) {
        printf("Error: Not a minigit repository. Run 'minigit init' first.\n");
        return -1;
    }
    
    // Check for staged changes
    if (read_index(&entries, &count) != 0) {
        printf("Error: Failed to read staged files.\n");
        return -1;
    }
    
    if (count == 0) {
        printf("No changes staged for commit.\n");
        if (entries) free(entries);
        return 0;
    }
    
    // Create commit object
    if (create_commit_object(message, commit_hash) != 0) {
        printf("Error: Failed to create commit.\n");
        if (entries) free(entries);
        return -1;
    }
    
    // Update HEAD to point to new commit
    if (update_head(commit_hash) != 0) {
        printf("Error: Failed to update HEAD.\n");
        if (entries) free(entries);
        return -1;
    }
    
    printf("Committed changes: %s\n", message);
    printf("Files committed: %d\n", count);
    
    if (entries) free(entries);
    return 0;
}

// Create a commit object with metadata and file references
int create_commit_object(const char *message, char *commit_hash) {
    IndexEntry *entries = NULL;
    int count = 0;
    char commit_content[4096];
    char timestamp_str[64];
    time_t current_time;
    
    // Read staged files from index
    if (read_index(&entries, &count) != 0) {
        return -1;
    }
    
    // Get current timestamp
    current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Build commit content
    int offset = 0;
    offset += snprintf(commit_content + offset, sizeof(commit_content) - offset,
                      "timestamp: %s\n", timestamp_str);
    offset += snprintf(commit_content + offset, sizeof(commit_content) - offset,
                      "message: %s\n", message);
    offset += snprintf(commit_content + offset, sizeof(commit_content) - offset,
                      "files: %d\n", count);
    
    for (int i = 0; i < count; i++) {
        offset += snprintf(commit_content + offset, sizeof(commit_content) - offset,
                          "%s %s\n", entries[i].hash, entries[i].path);
    }
    
    // Compute hash of commit content
    unsigned char binary_hash[SHA1_DIGEST_LENGTH];
    simple_sha1((unsigned char*)commit_content, strlen(commit_content), binary_hash);
    sha1_to_hex(binary_hash, commit_hash);
    
    // Store commit object
    char commit_path[MAX_PATH_LENGTH];
    snprintf(commit_path, sizeof(commit_path), "%s/%s", OBJECTS_DIR, commit_hash);
    
    
    if (write_file(commit_path, commit_content, strlen(commit_content)) != 0) {
        if (entries) free(entries);
        return -1;
    }
    
    if (entries) free(entries);
    return 0;
}

// Update HEAD to point to the latest commit
int update_head(const char *commit_hash) {
    FILE *head_file = fopen(HEAD_FILE, "w");
    if (!head_file) {
        return -1;
    }
    
    fprintf(head_file, "%s\n", commit_hash);
    fclose(head_file);
    

    return 0;
}

// Read a commit object (utility function for status and other commands)
int read_commit(const char *commit_hash, Commit *commit) {
    char commit_path[MAX_PATH_LENGTH];
    char *content;
    size_t size;
    
    if (strlen(commit_hash) == 0) {
        return -1; // No commit hash
    }
    
    snprintf(commit_path, sizeof(commit_path), "%s/%s", OBJECTS_DIR, commit_hash);
    if (read_file(commit_path, &content, &size) != 0) {
        return -1;
    }
    
    // Parse commit content
    strncpy(commit->hash, commit_hash, SHA1_HEX_LENGTH - 1);
    commit->hash[SHA1_HEX_LENGTH - 1] = '\0';
    char *line = strtok(content, "\n");
    commit->file_count = 0;
    commit->files = NULL;
    
    while (line != NULL) {
        if (strncmp(line, "timestamp: ", 11) == 0) {
            // Parse timestamp (very simplified)
            commit->timestamp = time(NULL);
        } else if (strncmp(line, "message: ", 9) == 0) {
            strncpy(commit->message, line + 9, MAX_MESSAGE_LENGTH - 1);
            commit->message[MAX_MESSAGE_LENGTH - 1] = '\0';
        } else if (strncmp(line, "files: ", 7) == 0) {
            commit->file_count = atoi(line + 7);
            if (commit->file_count > 0) {
                commit->files = malloc(commit->file_count * sizeof(IndexEntry));
            }
        } else if (strlen(line) > SHA1_HEX_LENGTH && commit->files != NULL) {
            // Parse file entry (hash path)
            static int file_index = 0;
            if (file_index < commit->file_count) {
                char *hash = strtok(line, " ");
                char *path = strtok(NULL, "");
                if (hash && path) {
                    strncpy(commit->files[file_index].hash, hash, SHA1_HEX_LENGTH - 1);
                    commit->files[file_index].hash[SHA1_HEX_LENGTH - 1] = '\0';
                    strncpy(commit->files[file_index].path, path, MAX_PATH_LENGTH - 1);
                    commit->files[file_index].path[MAX_PATH_LENGTH - 1] = '\0';
                    file_index++;
                }
            }
        }
        line = strtok(NULL, "\n");
    }
    
    free(content);
    return 0;
} 