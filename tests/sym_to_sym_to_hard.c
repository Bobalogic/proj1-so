#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA!";
char const target_path1[] = "/f1";
char const link_path1[] = "/l1";
char const target_path2[] = "/f2";
char const link_path2[] = "/l2";
char const target_path3[] = "/f3";
char const link_path3[] = "/l3";
char const bad_link[] = "f3";
char const sym_link_path1[] = "/sl1";
char const sym_link_path2[] = "/sl2";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Write to symlink and read original file
    {
        int f = tfs_open(target_path1, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);

        assert_empty_file(target_path1); // sanity check
    }

    // Create a file
    assert(tfs_open(target_path1, TFS_O_CREAT) != -1);

    // Create two sym links to that file (which is a hardlink to that file)
    assert(tfs_sym_link(target_path1, sym_link_path1) != -1);
    assert(tfs_sym_link(sym_link_path1, sym_link_path2) != -1);

    assert(tfs_open(sym_link_path2, 0) != -1);

    // Remove hardlink to the original file
    assert(tfs_unlink(target_path1) != -1);

    // Open sym link to another sym link to a hard link
    assert(tfs_open(sym_link_path2, 0) == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
