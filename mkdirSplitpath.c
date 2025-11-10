#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct NODE* root;
extern struct NODE* cwd;

/* Local caps (match README/types.h expectations) */
#define BASENAME_MAX   63     /* fits struct NODE.name[64] */
#define DIRNAME_MAX    511
#define WORKBUF_MAX    511

/* splitPath:
 * - Fills dirName (prefix) and baseName (final component) WITHOUT modifying pathName
 * - Returns pointer to the parent NODE of baseName (or NULL on error)
 * - Prints: ERROR: directory <DIRECTORY> does not exist    on first missing dir
 *
 * Rules from README:
 *   "/"           -> dirName="/", baseName=""
 *   "f1.txt"      -> dirName="",  baseName="f1.txt"       (relative)
 *   "/a/b/c.txt"  -> dirName="/a/b", baseName="c.txt"     (absolute)
 *   "/a/b/c"      -> dirName="/a/b", baseName="c"
 */
struct NODE* splitPath(char* pathName, char* baseName, char* dirName)
{
    /* Defensive init of outputs */
    if (dirName)  dirName[0]  = '\0';
    if (baseName) baseName[0] = '\0';

    /* Edge: empty or "/" */
    if (!pathName || pathName[0] == '\0' || strcmp(pathName, "/") == 0) {
        if (dirName)  { strncpy(dirName, "/", DIRNAME_MAX); dirName[DIRNAME_MAX] = '\0'; }
        if (baseName) { baseName[0] = '\0'; }
        return root;
    }

    /* Start point depends on absolute vs relative */
    struct NODE* start = (pathName[0] == '/') ? root : cwd;

    /* Work on a local copy so we NEVER modify pathName (fixes test #6) */
    char copy[WORKBUF_MAX + 1];
    strncpy(copy, pathName, WORKBUF_MAX);
    copy[WORKBUF_MAX] = '\0';

    /* (Optional) trim trailing slashes (keep lone "/") */
    size_t L = strlen(copy);
    while (L > 1 && copy[L - 1] == '/') { copy[--L] = '\0'; }

    /* Split copy into dirName/baseName using last '/' in the COPY */
    char *last = strrchr(copy, '/');
    if (!last) {
        /* No slash at all: relative single token */
        if (dirName)  dirName[0] = '\0';
        if (baseName) { strncpy(baseName, copy, BASENAME_MAX); baseName[BASENAME_MAX] = '\0'; }
    } else {
        /* base = after last '/', dir = before it (or "/") */
        if (baseName) { strncpy(baseName, last + 1, BASENAME_MAX); baseName[BASENAME_MAX] = '\0'; }

        if (last == copy && copy[0] == '/') {
            /* like "/name" => dirName="/" */
            if (dirName) { strncpy(dirName, "/", DIRNAME_MAX); dirName[DIRNAME_MAX] = '\0'; }
        } else {
            *last = '\0';  /* mutate the COPY (safe) */
            if (dirName) { strncpy(dirName, copy, DIRNAME_MAX); dirName[DIRNAME_MAX] = '\0'; }
        }
    }

    /* Decide the node to start traversal from */
    struct NODE* cur = start;

    /* If dirName empty or "/" no traversal needed */
    if (!dirName || dirName[0] == '\0' || strcmp(dirName, "/") == 0) {
        return cur;
    }

    /* Tokenize a SECOND copy (so dirName itself remains intact) */
    char walk[WORKBUF_MAX + 1];
    strncpy(walk, dirName, WORKBUF_MAX);
    walk[WORKBUF_MAX] = '\0';

    char *tok = strtok(walk, "/");
    while (tok) {
        struct NODE *child = cur->childPtr;
        struct NODE *found = NULL;

        while (child) {
            if (child->fileType == 'D' && strcmp(child->name, tok) == 0) {
                found = child;
                break;
            }
            child = child->siblingPtr;
        }

        if (!found) {
            /* First non-existent directory on the path */
            printf("ERROR: directory %s does not exist\n", tok);
            return NULL;
        }

        cur = found;
        tok = strtok(NULL, "/");
    }

    /* cur now is the parent directory node of baseName */
    return cur;
}

/* mkdir:
 * - Uses splitPath() to resolve parent + baseName
 * - Errors:
 *     "MKDIR ERROR: no path provided"                for "/" or empty
 *     "MKDIR ERROR: directory <DIRECTORY> already exists" if duplicate
 * - Success:
 *     "MKDIR SUCCESS: node <NODE> successfully created"
 */
void mkdir(char pathName[])
{
    if (!pathName || strcmp(pathName, "/") == 0) {
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* buffers for split results */
    char baseName[BASENAME_MAX + 1];
    char dirName[DIRNAME_MAX + 1];

    struct NODE *parent = splitPath(pathName, baseName, dirName);
    if (!parent) return;                     /* splitPath already printed error */

    if (baseName[0] == '\0') {
        /* No final component provided */
        printf("MKDIR ERROR: no path provided\n");
        return;
    }

    /* Duplicate check under 'parent' */
    for (struct NODE *p = parent->childPtr; p; p = p->siblingPtr) {
        if (strcmp(p->name, baseName) == 0) {
            printf("MKDIR ERROR: directory %s already exists\n", baseName);
            return;
        }
    }

    /* Allocate and initialize the new directory node */
    struct NODE *n = (struct NODE*)malloc(sizeof(struct NODE));
    if (!n) { perror("malloc"); return; }

    memset(n, 0, sizeof(*n));
    /* Truncate name to fit struct NODE.name[64] */
    strncpy(n->name, baseName, BASENAME_MAX);
    n->name[BASENAME_MAX] = '\0';

    n->fileType   = 'D';
    n->parentPtr  = parent;
    n->childPtr   = NULL;
    n->siblingPtr = NULL;

    /* Insert as last child of parent */
    if (!parent->childPtr) {
        parent->childPtr = n;
    } else {
        struct NODE *t = parent->childPtr;
        while (t->siblingPtr) t = t->siblingPtr;
        t->siblingPtr = n;
    }

    printf("MKDIR SUCCESS: node %s successfully created\n", pathName);
}
