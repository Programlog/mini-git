#include "minigit.h"

// Initialize a new minigit repository
int cmd_init(void) {
    if (directory_exists(MINIGIT_DIR)) {
        printf("Repository already initialized.\n");
        return 0;
    }
    
    if (create_minigit_directory() != 0) {
        printf("Error: Failed to create repository structure.\n");
        return -1;
    }
    
    printf("Initialized empty minigit repository.\n");
    return 0;
}

// Create the .minigit directory structure
int create_minigit_directory(void) {
    if (create_directory(MINIGIT_DIR) != 0) {
        fprintf(stderr, "Error creating %s directory: %s\n", MINIGIT_DIR, strerror(errno));
        return -1;
    }
    
    // Create objects directory
    if (create_directory(OBJECTS_DIR) != 0) {
        fprintf(stderr, "Error creating %s directory: %s\n", OBJECTS_DIR, strerror(errno));
        return -1;
    }
    
    // Create empty index file
    FILE *index_file = fopen(INDEX_FILE, "w");
    if (!index_file) {
        fprintf(stderr, "Error creating %s: %s\n", INDEX_FILE, strerror(errno));
        return -1;
    }
    fclose(index_file);
    
    // Create empty HEAD file
    FILE *head_file = fopen(HEAD_FILE, "w");
    if (!head_file) {
        fprintf(stderr, "Error creating %s: %s\n", HEAD_FILE, strerror(errno));
        return -1;
    }
    fclose(head_file);
    
    return 0;
} 