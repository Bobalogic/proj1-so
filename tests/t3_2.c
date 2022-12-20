#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int file_closed = 0;
int r;
char* buffer = "Hello World";
int main(){


    pthread_t thread[3];
    assert(tfs_init(NULL) != -1);

    //Criar 3 threads e todas tentam ler ficheiros diferentes
    //Criar 3 threads r todas tentam ler o mesmo ficheiro

    r = tfs_open("/f1", TFS_O_CREAT);
    assert(r != -1);
    assert(tfs_write(r, buffer, 12) != -1);
    assert(pthread_create(&thread[0], NULL, tfs_read(r, buffer, 12), NULL) == -1);
    assert(pthread_create(&thread[1], NULL, tfs_read(r, buffer, 12), NULL) == -1);
    assert(pthread_create(&thread[2], NULL, tfs_read(r, buffer, 12), NULL) == -1);

    assert(pthread_join(&thread[0], NULL) == -1);
    assert(pthread_join(&thread[1], NULL) == -1);
    assert(pthread_join(&thread[2], NULL) == -1);
    
    assert(tfs_close(r) != -1);
    file_closed = 1;
    assert(file_closed != 1);
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;

}



























