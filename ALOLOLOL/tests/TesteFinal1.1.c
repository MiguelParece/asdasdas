#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

//This test tests if the tfs_copy_from_external_fs modifies the source file, sees if the buffer's size 
//creates erros (it shouldn't) as long as the buffer'size is bigger than the amount of characters of the file, 
// and lastly it checks what happens if we try to give content to a file bigger than its attributed block of size 
int main() {

    char *str_ext_file = "Teste 1.1 tfs_copy_from_external!";
    char *path_copied_file= "/f1";
    char *path_src = "tests/external_file.txt";
    char *path_src2 = "tests/externalLarge.txt";
    char buffer[40];
    char bufferWeird[489];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r,rWeird;

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    assert(tfs_close(f)!=-1);

    //Copy the same content to the previous created file. Should overwrite and not append
    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    rWeird = tfs_read(f, bufferWeird, sizeof(bufferWeird) - 1);
    assert(rWeird == strlen(str_ext_file));
    assert(!memcmp(bufferWeird, str_ext_file, strlen(str_ext_file)));

    assert(tfs_close(f)!=-1);

    //Confirms that copy_from_external can correctly copy the contents
    //of the file , regardless the size of the buffer, as long as the buffer's size>strlen(file). 
    // If that condition isn't met we won't have enough space in the buffer to correctly compare the contents 
    // with memcmp!
    assert(rWeird==r);

    // Copy the same content to a different file in FS, in order to see if the source_file is modified,
    //which it shouldn't
    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

//Try tfs_copy but with a file with a much larger text, and by this we mean, larger then
// the block size that TFS allows, 1 block of size 1024
    f = tfs_copy_from_external_fs(path_src2, path_copied_file);
    assert(f == -1);

    assert(tfs_destroy()!=-1);

    printf("Successful test.\n");

    return 0;
}
