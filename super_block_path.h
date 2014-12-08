#ifndef SUPER_BLOCK_PATH_H
#define SUPER_BLOCK_PATH_H

#include <suif1.h>
#include "super_block.h"

class super_block_path
{
private:
    super_block_list *path;

public:
    super_block_path();
    virtual ~super_block_path();

    super_block_list* super_blocks() {return path;}
    super_block* first_super_block();
    super_block* last_super_block();
    super_block* convert_to_super_block();
    void add_super_block(super_block *sb);
    void add_super_blocks(super_block_list *sb_list);
    bool contains(super_block *sb);
    unsigned int static_size();

    void print_block_list_num(FILE *fp);
    void print(FILE *fp = stdout);
};

#endif
