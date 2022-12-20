#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "TesteFinal1.2!";
char const *str_ext_file = "Teste 1.1 tfs_copy_from_external!";
char const *source_path1="tests/external_file.txt";
char const target_path1[] = "/f1";
char const link_path1[] = "/l1";
char const target_path2[] = "/f2";
char const link_path2[] = "/l2";
char const sym_link_path1[]="/sl1";
char const sym_link_path2[]="/sl2";
char const *sym_external_path="/sle";
uint8_t buffer[sizeof(file_contents)];
char buffer_ext[128];

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

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

    //Create target_path1
    int f = tfs_open(target_path1, TFS_O_CREAT);
    ssize_t r;
    assert(f != -1);
    assert(tfs_close(f) != -1);
    
    //Create hard_link1
    assert(tfs_link(target_path1, link_path1) != -1);
    write_contents(link_path1);
    assert_contents_ok(target_path1);
    
    //Create target_path2
    f = tfs_open(target_path2, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);
    write_contents(target_path2);
    
    //Create  hard_link2
    assert(tfs_link(target_path2, link_path2) != -1);
    assert_contents_ok(link_path2);

    //Create 1 sym_link pointing to hardlink_1 
    assert(tfs_sym_link(link_path1,sym_link_path1)!=-1);
    assert_contents_ok(sym_link_path1);
    
    

    // "Change" a sym_link to a hard_link and verifies if its possible (it should) to open a sym_link 
    // that previously pointed to a sym_link and now is pointing to a hard_link
    assert(tfs_sym_link(sym_link_path1,sym_link_path2)!=-1);
    assert_contents_ok(sym_link_path2);
    assert(tfs_unlink(sym_link_path1)!=-1);
    assert(tfs_link(target_path1,sym_link_path1)!=-1);//Despite saying sym_link this is actually a hard_link
    write_contents(sym_link_path1);
    f=tfs_open(sym_link_path1,TFS_O_CREAT);             // ""        ""      ""    ""  ""    ""    "" ""   ""
    assert(f!=-1);
    assert(tfs_close(f)!=-1);
    assert_contents_ok(sym_link_path2);
    assert_contents_ok(sym_link_path1);               // ""        ""      ""    ""  ""    ""    "" ""   ""
    assert(tfs_unlink(sym_link_path1)!=-1);             

    //Try to make a hard_link pointing to a sym_link. Should give error.
    assert(tfs_link(sym_link_path2,link_path1)==-1);
    assert(tfs_unlink(sym_link_path2)!=-1); 

    //Try to make a tfs_copy_from_external_fs where the destination path is a symlink.
    assert(tfs_sym_link(link_path1,sym_external_path)!=-1);
    write_contents(sym_external_path);
    assert(tfs_copy_from_external_fs(source_path1,sym_external_path)!=-1);
    f = tfs_open(sym_external_path, TFS_O_CREAT);
    assert(f != -1);
    // Verifies if the content is correct
    r = tfs_read(f, buffer_ext, sizeof(buffer_ext) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer_ext, str_ext_file, strlen(str_ext_file)));

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
