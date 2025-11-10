#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct NODE* root;
extern struct NODE* cwd;

/*
 * splitPath:
 *  - Separates pathName into dirName (everything before the last '/')
 *    and baseName (the final component).
 *  - Traverses the filesystem tree from root or cwd through dirName.
 *  - Returns the NODE* of the parent directory for baseName,
 *    or NULL if any directory in dirName does not exist.
 */
struct NODE* splitPath(char* pathName, char* baseName, char* dirName)
{
    baseName[0] = '\0';
    dirName[0]  = '\0';

    if (!pathName || pathName[0] == '\0' || strcmp(pathName, "/") == 0) {
        strcpy(dirName, "/");
        return root;
    }

    struct NODE* start = (pathName[0] == '/') ? root : cwd;

    char tmp[512];
    strncpy(tmp, pathName, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    /* remove trailing slashes */
    size_t L = strlen(tmp);
    while (L > 1 && tmp[L - 1] == '/') tmp[--L] = '\0';

    char* lastSlash = strrchr(tmp, '/');
    if (!lastSlash) {
        /* relative single token */
        strcpy(baseName, tmp);
        dirName[0] = '\0';
    } else {
        strcpy(baseName, lastSlash + 1);
        if (lastSlash == tmp && tmp[0] == '/') {
            strcpy(dirName, "/");
        } else {
            *lastSlash = '\0';
            strncpy(dirName, tmp, sizeof(tmp) - 1);
            dirName[sizeof(tmp) - 1] = '\0';
        }
    }

    if (dirName[0] == '\0' || strcmp(dirName, "/") == 0)
        return start;

    /* traverse tokens of dirName */
    char walk[512];
    strncpy(walk, dirName, sizeof(walk) - 1);
    walk[sizeof(walk) - 1] = '\0';

    char* tok = strtok(walk, "/");
    struct NODE* cur = start;

    while (tok) {
        struct NODE* child = cur->childPtr;
        struct NODE* found = NULL;

        while (child) {
            if (child->fileType == 'D' && strcmp(child->name, tok) == 0) {
                found = child;
                break;
            }
            child = child->siblingPtr;
        }

        if (!found) {
            printf("ERROR: directory %s does not exist\n", tok);
            return NULL;
        }

        cur = found;
        tok = strtok(NULL, "/");
    }

    return cur; /* parent of baseName */
}

/*
 * mkdir:
 *  - Calls splitPath() to locate the parent of the new directory.
 *  - Validates name, detects duplicates, allocates and links the new node.
 *  - Prints required messages exactly as specified.
 */
void mkdir(char pathName[])
{
    if (!pathName || strcmp(pathName, "/") == 0) {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    char baseName[64];
    char dirName[512];

    struct NODE* parent = splitPath(pathName, baseName, dirName);
    if (!parent)
        return;

    if (baseName[0] == '\0') {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* duplicate check */
    struct NODE* p = parent->childPtr;
    while (p) {
        if (strcmp(p->name, baseName) == 0) {
            printf("MKDIR ERROR: directory %s already exists\n", baseName);
            return;
        }
        p = p->siblingPtr;
    }

    /* allocate & initialize */
    struct NODE* n = (struct NODE*)malloc(sizeof(struct NODE));
    if (!n) {
        perror("malloc");
        return;
    }

    memset(n, 0, sizeof(*n));
    strncpy(n->name, baseName, sizeof(n->name) - 1);
    n->name[sizeof(n->name) - 1] = '\0';
    n->fileType   = 'D';
    n->parentPtr  = parent;
    n->childPtr   = NULL;
    n->siblingPtr = NULL;

    /* insert as last child */
    if (!parent->childPtr)
        parent->childPtr = n;
    else {
        struct NODE* tail = parent->childPtr;
        while (tail->siblingPtr)
            tail = tail->siblingPtr;
        tail->siblingPtr = n;
    }

    printf("MKDIR SUCCESS: node %s successfully created\n", pathName);
}
