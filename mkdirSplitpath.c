#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct NODE* root;
extern struct NODE* cwd;

/* --- Local safe helpers --- */
static void copy_cap(char *dst, size_t dstsz, const char *src) {
    if (!dst || dstsz == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    /* copy at most dstsz-1 chars, always NUL-terminate */
    size_t n = dstsz - 1;
    strncpy(dst, src, n);
    dst[n] = '\0';
}

/*
 * splitPath:
 *  - Fills baseName (final component) and dirName (prefix).
 *  - Traverses from root/cwd through dirName and returns the parent NODE*.
 *  - On missing directory, prints:
 *        ERROR: directory <TOKEN> does not exist
 *    and returns NULL.
 */
struct NODE* splitPath(char* pathName, char* baseName, char* dirName)
{
    /* Defensive defaults for the caller's buffers */
    if (baseName) baseName[0] = '\0';
    if (dirName)  dirName[0]  = '\0';

    if (!pathName || pathName[0] == '\0' || strcmp(pathName, "/") == 0) {
        /* Treat empty or "/" as root parent */
        copy_cap(dirName, 512, "/");
        return root;
    }

    /* Detect the ACTUAL max name length from struct NODE.name */
    struct NODE _probe;
    const size_t NAME_CAP = (sizeof _probe.name > 0 ? sizeof _probe.name - 1 : 0);

    /* Start point: absolute → root, relative → cwd */
    struct NODE* start = (pathName[0] == '/') ? root : cwd;

    /* Work on a bounded temporary copy we can mutate */
    char tmp[512];
    copy_cap(tmp, sizeof tmp, pathName);

    /* Trim trailing slashes but keep lone "/" */
    size_t L = strlen(tmp);
    while (L > 1 && tmp[L - 1] == '/') { tmp[--L] = '\0'; }

    /* Split into dirName + baseName using the last '/' */
    char* lastSlash = strrchr(tmp, '/');
    if (!lastSlash) {
        /* No slash at all: relative single token like "a...long..." */
        if (baseName) copy_cap(baseName, NAME_CAP + 1, tmp);
        if (dirName)  dirName[0] = '\0';
    } else {
        /* baseName = after last '/', dirName = before it (or "/") */
        if (baseName) copy_cap(baseName, NAME_CAP + 1, lastSlash + 1);

        if (lastSlash == tmp && tmp[0] == '/') {
            if (dirName) copy_cap(dirName, 512, "/");
        } else {
            *lastSlash = '\0';
            if (dirName) copy_cap(dirName, 512, tmp);
        }
    }

    /* If no traversal needed ("" or "/"), return the chosen start */
    if (!dirName || dirName[0] == '\0' || strcmp(dirName, "/") == 0) {
        return start;
    }

    /* Tokenize dirName and traverse down */
    char walk[512];
    copy_cap(walk, sizeof walk, dirName);

    char* tok = strtok(walk, "/");
    struct NODE* cur = start;

    while (tok) {
        /* Search only directories among cur's children */
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
 *  - Treats pathName == "/" as "no arg" (your main defaults missing path to "/").
 *  - Uses splitPath() to locate the parent and final name.
 *  - Duplicate check, allocate node, insert as last child.
 */
void mkdir(char pathName[])
{
    if (!pathName || strcmp(pathName, "/") == 0) {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* Detect name capacity from struct NODE */
    struct NODE _probe;
    const size_t NAME_CAP = (sizeof _probe.name > 0 ? sizeof _probe.name - 1 : 0);

    char baseName[256];  /* working buffer for final component (bigger than any struct name) */
    char dirName[512];

    struct NODE* parent = splitPath(pathName, baseName, dirName);
    if (!parent) return;

    if (baseName[0] == '\0') {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* Duplicate check under parent */
    for (struct NODE* p = parent->childPtr; p; p = p->siblingPtr) {
        if (strcmp(p->name, baseName) == 0) {
            printf("MKDIR ERROR: directory %s already exists\n", baseName);
            return;
        }
    }

    /* Allocate & initialize new node */
    struct NODE* n = (struct NODE*)malloc(sizeof(struct NODE));
    if (!n) { perror("malloc"); return; }
    memset(n, 0, sizeof *n);

    /* Copy/truncate final name to fit struct NODE.name exactly */
    if (NAME_CAP > 0) {
        strncpy(n->name, baseName, NAME_CAP);
        n->name[NAME_CAP] = '\0';
    } else {
        n->name[0] = '\0';
    }

    n->fileType   = 'D';
    n->parentPtr  = parent;
    n->childPtr   = NULL;
    n->siblingPtr = NULL;

    /* Insert as last child */
    if (!parent->childPtr) {
        parent->childPtr = n;
    } else {
        struct NODE* t = parent->childPtr;
        while (t->siblingPtr) t = t->siblingPtr;
        t->siblingPtr = n;
    }

    printf("MKDIR SUCCESS: node %s successfully created\n", pathName);
}
