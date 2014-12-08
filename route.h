/*
 * route.h
 *
 *  Created on: 2014年5月29日
 *      Author: qmwx
 */

#ifndef ROUTE_H_
#define ROUTE_H_

#include <stdio.h>
#include "super_block.h"
#include "super_block_path.h"

class route
{
private:
	int from_block;
	int to_block;
	super_block_path *most_likely_path;
	std::vector<super_block_path*>* all_possible_path;
	std::vector<super_block_path*>* all_suit_path;
public:
	route():from_block(0), to_block(0), most_likely_path(NULL), all_possible_path(NULL), all_suit_path(NULL){}
	route(int from, int to);

	super_block* get_from_block()const;
	super_block* get_to_block()const;

	int getFromBlock() const {
		return from_block;
	}

	void setFromBlock(int fromBlock) {
		from_block = fromBlock;
	}

	int getToBlock() const {
		return to_block;
	}

	void setToBlock(int toBlock) {
		to_block = toBlock;
	}

	/**
	 * -----key methods below
	 */
	void find_all_possible_path();
	std::vector<super_block_path*>* get_all_possible_path(){return all_possible_path;}
	void filter_sp_block_path(int sp_num);
	std::vector<super_block_path*>* get_all_suit_path(){return all_suit_path;}


	~route();
	/**
	 * ------------------------print and test ---------------------------------------------
	 */
	void print_all_possible_path_block_num(FILE *fp = stderr);
	void print_all_suit_path_block_num(FILE *fp = stderr);
	void print(FILE *fp = stderr);

};

#endif /* ROUTE_H_ */
