#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct NODE* root;
extern struct NODE* cwd;

/* Match typical course constraints (struct NODE.name[64]) */
#define OUT_CAP 63          /* cap for baseName and dirName */
#define WORK_CAP 511        /* local scratch buffers */

/* safe bounded copy */
static inline void copy_cap(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, cap);
    dst[cap] = '\0';
}

struct NODE* splitPath(char* pathName, char* baseName, char* dirName)
{
    if (baseName) baseName[0] = '\0';
    if (dirName)  dirName[0]  = '\0';

    /* treat empty or "/" as root */
    if (!pathName || pathName[0] == '\0' || strcmp(pathName, "/") == 0) {
        if (dirName)  copy_cap(dirName, OUT_CAP, "/");
        if (baseName) baseName[0] = '\0';
        return root;
    }

    struct NODE* start = (pathName[0] == '/') ? root : cwd;

    /* work on a copy so we NEVER modify pathName */
    char copy[WORK_CAP + 1];
    copy_cap(copy, WORK_CAP, pathName);

    /* trim trailing slashes, but keep single "/" */
    size_t L = strlen(copy);
    while (L > 1 && copy[L - 1] == '/') { copy[--L] = '\0'; }

    /* split dir/base using last slash on the COPY */
    char *last = strrchr(copy, '/');
    if (!last) {
        /* relative single component like "a" */
        if (baseName) copy_cap(baseName, OUT_CAP, copy);
        if (dirName)  dirName[0] = '\0';
    } else {
        if (baseName) copy_cap(baseName, OUT_CAP, last + 1);
        if (last == copy && copy[0] == '/') {
            if (dirName) copy_cap(dirName, OUT_CAP, "/");
        } else {
            *last = '\0'; /* mutate COPY, not pathName */
            if (dirName) copy_cap(dirName, OUT_CAP, copy);
        }
    }

    /* if no traversal ("" or "/"), return start */
    if (!dirName || dirName[0] == '\0' || strcmp(dirName, "/") == 0) {
        return start;
    }

    /* tokenize a second copy of dirName for traversal so the original
       dirName stays intact for rmdir’s error messages */
    char walk[WORK_CAP + 1];
    copy_cap(walk, WORK_CAP, dirName);

    char *tok = strtok(walk, "/");
    struct NODE *cur = start;

    while (tok) {
        struct NODE *child = cur->childPtr, *found = NULL;
        while (child) {
            if (child->fileType == 'D' && strcmp(child->name, tok) == 0) {
                found = child; break;
            }
            child = child->siblingPtr;
        }
        if (!found) {
            /* IMPORTANT: print exactly the token (not dirName), but ensure it’s not empty */
            if (tok[0] == '\0') printf("ERROR: directory / does not exist\n");
            else                printf("ERROR: directory %s does not exist\n", tok);
            return NULL;
        }
        cur = found;
        tok = strtok(NULL, "/");
    }

    return cur;
}

void mkdir(char pathName[])
{
    if (!pathName || strcmp(pathName, "/") == 0) {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    char baseName[OUT_CAP + 1];
    char dirName[OUT_CAP + 1];

    struct NODE *parent = splitPath(pathName, baseName, dirName);
    if (!parent) return;

    if (baseName[0] == '\0') {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* duplicate check */
    for (struct NODE *p = parent->childPtr; p; p = p->siblingPtr) {
        if (strcmp(p->name, baseName) == 0) {
            printf("MKDIR ERROR: directory %s already exists\n", baseName);
            return;
        }
    }

    struct NODE *n = (struct NODE*)malloc(sizeof *n);
    if (!n) { perror("malloc"); return; }
    memset(n, 0, sizeof *n);

    /* truncate to struct name capacity (assumed 64) */
    copy_cap(n->name, OUT_CAP, baseName);
    n->fileType = 'D';
    n->parentPtr = parent;
    n->childPtr = NULL;
    n->siblingPtr = NULL;

    if (!parent->childPtr) parent->childPtr = n;
    else {
        struct NODE *t = parent->childPtr;
        while (t->siblingPtr) t = t->siblingPtr;
        t->siblingPtr = n;
    }

    printf("MKDIR SUCCESS: node %s successfully created\n", pathName);
}

