#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct NODE* root;
extern struct NODE* cwd;

#define BASENAME_CAP   63   /* one less than baseName buffer (64) */
#define DIRNAME_CAP    511  /* one less than dirName buffer (512) */
#define TMP_CAP        511
#define WALK_CAP       511
#define NODE_NAME_CAP  63   /* struct NODE.name[64] */

static void scpy_cap(char *dst, const char *src, size_t cap) {
    /* Copy at most 'cap' chars; always NUL-terminate */
    if (!dst) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, cap);
    dst[cap] = '\0';
}

struct NODE* splitPath(char* pathName, char* baseName, char* dirName)
{
    /* defensive defaults */
    if (baseName) baseName[0] = '\0';
    if (dirName)  dirName[0]  = '\0';

    if (!pathName || pathName[0] == '\0' || strcmp(pathName, "/") == 0) {
        scpy_cap(dirName, "/", DIRNAME_CAP);
        return root;
    }

    struct NODE* start = (pathName[0] == '/') ? root : cwd;

    /* work on a temporary copy */
    char tmp[TMP_CAP + 1];
    scpy_cap(tmp, pathName, TMP_CAP);

    /* trim trailing slashes but keep lone "/" */
    size_t L = strlen(tmp);
    while (L > 1 && tmp[L - 1] == '/') { tmp[--L] = '\0'; }

    char* lastSlash = strrchr(tmp, '/');
    if (!lastSlash) {
        /* relative single token like "a...long..." (truncate safely) */
        scpy_cap(baseName, tmp, BASENAME_CAP);
        if (dirName) dirName[0] = '\0';
    } else {
        /* base = after last '/', dir = before it (or "/") */
        scpy_cap(baseName, lastSlash + 1, BASENAME_CAP);

        if (lastSlash == tmp && tmp[0] == '/') {
            scpy_cap(dirName, "/", DIRNAME_CAP);
        } else {
            *lastSlash = '\0';
            scpy_cap(dirName, tmp, DIRNAME_CAP);
        }
    }

    /* if no traversal needed */
    if (dirName[0] == '\0' || strcmp(dirName, "/") == 0) {
        return start;
    }

    /* tokenize dirName and traverse */
    char walk[WALK_CAP + 1];
    scpy_cap(walk, dirName, WALK_CAP);

    char* tok = strtok(walk, "/");
    struct NODE* cur = start;

    while (tok) {
        struct NODE* child = cur->childPtr;
        struct NODE* found = NULL;

        while (child) {
            /* Only match directories */
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

void mkdir(char pathName[])
{
    if (!pathName || strcmp(pathName, "/") == 0) {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    char baseName[BASENAME_CAP + 1];
    char dirName[DIRNAME_CAP + 1];

    struct NODE* parent = splitPath(pathName, baseName, dirName);
    if (!parent) return;

    if (baseName[0] == '\0') {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* duplicate check under parent */
    for (struct NODE* p = parent->childPtr; p; p = p->siblingPtr) {
        if (strcmp(p->name, baseName) == 0) {
            printf("MKDIR ERROR: directory %s already exists\n", baseName);
            return;
        }
    }

    /* allocate & initialize new node */
    struct NODE* n = (struct NODE*)malloc(sizeof(struct NODE));
    if (!n) { perror("malloc"); return; }

    memset(n, 0, sizeof(*n));
    scpy_cap(n->name, baseName, NODE_NAME_CAP);
    n->fileType   = 'D';
    n->parentPtr  = parent;
    n->childPtr   = NULL;
    n->siblingPtr = NULL;

    /* insert as last child */
    if (!parent->childPtr) {
        parent->childPtr = n;
    } else {
        struct NODE* t = parent->childPtr;
        while (t->siblingPtr) t = t->siblingPtr;
        t->siblingPtr = n;
    }

    printf("MKDIR SUCCESS: node %s successfully created\n", pathName);
}
