#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int r, s, t;
char* buffer =  "Hello World number two";

int main(){
    pthread_t thread[3];
    assert(tfs_init(NULL) != -1);

    r = tfs_open("/f1", TFS_O_CREAT);
    s = tfs_open("/f2", TFS_O_CREAT);
    t = tfs_open("/f2", TFS_O_CREAT);

    assert(r != -1 && s != -1 && t != -1);
    assert(tfs_write(r, buffer, 23) != -1);
    assert(tfs_write(s, buffer, 23) != -1);
    assert(tfs_write(t, buffer, 23) != -1);
    
    assert(pthread_create(&thread[0], NULL, tfs_read(r, buffer, 23), NULL) == -1);
    assert(pthread_create(&thread[1], NULL, tfs_read(s, buffer, 23), NULL) == -1);
    assert(pthread_create(&thread[2], NULL, tfs_read(t, buffer, 23), NULL) == -1);

    assert(pthread_join(&thread[0], NULL) == -1);
    assert(pthread_join(&thread[1], NULL) == -1);
    assert(pthread_join(&thread[2], NULL) == -1);

    assert(tfs_close(r) != -1);
    assert(tfs_close(s) != -1);
    assert(tfs_close(t) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;

}