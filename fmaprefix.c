
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <strings.h>


#define MAX_QUERY_LEN  2048


#define MIN(a,b) (((a)<(b))?(a):(b))


static off_t findLineStart(char *ptr, off_t off);
static off_t getLineLen(char *ptr, off_t len, off_t lineStart);
static void printLine(char *ptr, off_t lineStart, off_t lineLen);

int main(int argc, char *argv[]) {

    if(argc < 2) {
        fprintf(stderr, "usage: fmaprefix some_sorted_file.txt prefix");
        return 4;
    }

    const char *fn = argv[1];

    char q[MAX_QUERY_LEN];
    q[0] = '\0';

    for(int i = 2; i < argc; ++ i) {
        if(i > 2) {
            strcat(q, " ");
        }
        strcat(q, argv[i]);
    }


    size_t qlen = strlen(q);

    if(qlen == 0) {
        fprintf(stderr, "I need a query");
        return 3;
    }


    int fd = open(fn, O_RDONLY);

    struct stat sstat;

    if(fd == -1 || fstat(fd, &sstat) == -1) {
        fprintf(stderr, "could not open file: %s\n", strerror(errno));
        return 1;
    }

    size_t len = sstat.st_size;

    char *ptr = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);

    if(ptr == NULL) {
        fprintf(stderr, "mmap returned NULL: %s\n", strerror(errno));
        return 2;
    }

    fprintf(stderr, "mmap was successful; len %d\n", (int) len);


    off_t left = 0;
    off_t right = len;

    off_t found = -1;

    while(left <= right) {

        off_t mid = ((right - left) / 2) + left;
        off_t lineStart = findLineStart(ptr, mid);

        printf("l %d; r %d; m %d; num %d\n", left, right, mid, right - left);

        if(lineStart == -1) {
            break;
        }

        size_t lineLen = getLineLen(ptr, len, lineStart);

        printf("@ ");
        printLine(ptr, lineStart, lineLen);

        int v = strncasecmp(ptr + lineStart, q, MIN(qlen, lineLen));

        printf("v is %d\n", v);

        if(v > 0) {
            // we are greater than query; discard us and anything greater than us
            right = lineStart - 1;
        } else if(v < 0) {
            // we are less than query; discard us and anything less than us
            left = lineStart + lineLen + 1;
        } else {
            // we match the query
            found = lineStart;
            break;
        }
    }

    if(found != -1) {

        // got a hit! now we just need all the matches
        // scan backwards to the first thing with this prefix, then
        // scan forward to the last thing printing as we go
        //
        for(;;) {
            off_t prevLineStart = findLineStart(ptr, found - 1);
            off_t prevLineLength = getLineLen(ptr, len, prevLineStart);

            printf("rewind ");
            printLine(ptr, prevLineStart, prevLineLength);

            if(strncasecmp(ptr + prevLineStart, q, MIN(qlen, prevLineLength)) != 0) {
                // prev line does not match; we found the first hit
                break;
            } else {
                found = prevLineStart;
            }
        }

        for(;;) {

            off_t lineLen = getLineLen(ptr, len, found);

            if(strncasecmp(ptr + found, q, MIN(qlen, lineLen)) != 0) {
                break;
            }

            printLine(ptr, found, lineLen);

            found += lineLen + 1;
        }

    } else {

        fprintf(stderr, "No hits found\n");
    }

    munmap(ptr, len);

    return 0;
}


off_t findLineStart(char *ptr, off_t off) {

    off_t lineStart = off;

    while(ptr[lineStart - 1] != '\n') {

        -- lineStart;

        if(lineStart == 0) {
            return -1;
        }
    }

    return lineStart;

}

off_t getLineLen(char *ptr, off_t len, off_t lineStart) {

    off_t lineLen = 0;

    for(off_t i = lineStart; i < len; ++ i) {
        if(ptr[i] == '\n')
            break;
        ++ lineLen;
    }

    return lineLen;
}

void printLine(char *ptr, off_t lineStart, off_t lineLen) {

    printf("%.*s\n", (int) lineLen, ptr + lineStart);

}












