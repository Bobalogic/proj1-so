#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>


int file_closed = 0;
int r;

void* funThread(void *arg){
    (void) arg;
    file_closed = 1; 

    assert(tfs_close(r) != -1);

    return NULL;
}

int main(){

    assert(tfs_close(r) != -1);

    pthread_t thread;
    r = tfs_open("/f1", TFS_O_CREAT);
    assert(r != -1);

    assert(pthread_create(&thread, NULL, funThread, NULL) == 0);
    assert(tfs_destroy() != -1);
    assert(file_closed == 1);

    printf("Successful test.\n");

    return 0;
}