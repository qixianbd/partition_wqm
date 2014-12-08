#include "super_block_path.h"
#include "loop_block.h"

super_block_path::super_block_path()
{
    path = new super_block_list();
}

super_block_path::~super_block_path()
{
    delete path;
}

super_block* super_block_path::first_super_block()
{
    super_block_list_iter iter(path);
    if(!iter.is_empty())
    {
        super_block *block = iter.step();
        return block;
    }
    else
        return 0;
}

super_block* super_block_path::last_super_block()
{
    super_block *last = 0;
    super_block_list_iter iter(path);
    while(!iter.is_empty())
    {
        last = iter.step();
    }
    return last;
}

/*
 *super_block_path::convert_to_super_block() --- convert super blocks to super block which contains super block list.
 */
super_block* super_block_path::convert_to_super_block()
{
    super_block *nodes = new super_block(super_block::BLOCK_LIST);
    super_block_list_iter iter(path);
    while(!iter.is_empty())
    {
        super_block *sb = iter.step();
        if(sb->knd() == super_block::BLOCK)
            nodes->add_block(sb->first_block());
        else if(sb->knd() == super_block::LOOP)
        {
            loop_block *loop = (loop_block *)sb;
            nodes->add_blocks(loop->most_likely_path()->nodes());
        }
    }
    return nodes;
}

void super_block_path::add_super_block(super_block *sb)
{
    if(!this->contains(sb))
        path->append(sb);
}

void super_block_path::add_super_blocks(super_block_list *sb_list)
{
    super_block_list_iter iter(sb_list);
    while(!iter.is_empty())
    {
        super_block *sblock = iter.step();
        path->append(sblock);
    }
}

bool super_block_path::contains(super_block *sb)
{
    super_block_list_iter iter(path);
    while(!iter.is_empty())
    {
        super_block *block = iter.step();
        if(block == sb)
            return true;
    }
    return false;
}

unsigned int super_block_path::static_size()
{
    unsigned int size = 0;
    super_block_list_iter iter(path);
    while(!iter.is_empty())
    {
        super_block *sblock = iter.step();
        size += sblock->static_size(); //修改此处，发现block_size统计时有问题，原因在于call_over_head统计有问题
    }
    return size;
}

void super_block_path::print_block_list_num(FILE *fp)
{
	super_block_list_iter iter(path);
	fprintf(fp, "[");
	while(!iter.is_empty())
	{
		super_block *sblock = iter.step();
		fprintf(fp, "%d, ", sblock->block_num());
	}
	fprintf(fp, "]\n");
	fflush(fp);
}


void super_block_path::print(FILE *fp)
{
    super_block_list_iter iter(path);
    while(!iter.is_empty())
    {
        super_block *sblock = iter.step();
        sblock->print_relationship(fp);
    }
}
