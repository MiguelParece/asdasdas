#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int main() {

    //char *str = "AAA!";
    char *path = "/f1";
    //char buffer[40];

    assert(tfs_init(NULL) != -1);

    int f;
    int fi;

    
    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    fi = tfs_open(path,TFS_O_TRUNC);
    assert(fi != -1);

    printf("Successful test.\n");


    return 0;
}
