/*
 * helper.cpp
 *
 *  Created on: 2012-2-16
 *      Author: harry
 */

#include "helper.h"

#include <cassert>
#include <vector>


/**
 * +测试通过
 *[beginIndex, ...)
 *允许返回empty vec
 */
std::vector<int> sub_num_vec(const std::vector<int> &src_num_vec,
		const int beginIndex) {
	assert(beginIndex >= 0 );
	assert( beginIndex <= src_num_vec.size());//当取"="时返回empty vec

	std::vector<int> ret_num_vec;
	for (int i = beginIndex; i < src_num_vec.size(); ++i) {
		ret_num_vec.push_back(src_num_vec[i]);
	}

	return ret_num_vec;
}


/**
 * +测试通过
 * [beginIndex, endIndex)
 * assert endIndex >= beginIndex ,return empty when endIndex == beginIndex
 * 允许返回empty vec
 */
std::vector<int> sub_num_vec(const std::vector<int> &src_num_vec, const int beginIndex, const int endIndex)
{
	assert(beginIndex >= 0 );
	assert( beginIndex < src_num_vec.size());
	assert(endIndex >= beginIndex);
	assert(endIndex <= src_num_vec.size());

	std::vector<int> ret_num_vec;
	for (int i = beginIndex; i < endIndex; ++i) {
		ret_num_vec.push_back(src_num_vec[i]);
	}

	return ret_num_vec;
}

/**
 * @pre-condition  src_num_vec contains nelem
 * @description 返回第一个nelem之前的子vec
 * @ret      可以返回empty vec
 */
std::vector<int> sub_num_vec_front_splited_by_elem(const std::vector<int> src_num_vec,
		const int nelem) {
	//assert(!src_num_vec.empty());

	unsigned index = 0;
	for (; index < src_num_vec.size(); ++index) {
		if (src_num_vec[index] == nelem)
			break;
	}
	assert(index >= 0);
	assert(index < src_num_vec.size());  //保证存在

	return sub_num_vec(src_num_vec, 0, index);
}


/**
 * @pre-condition  src_num_vec contains nelem
 * @description 返回最后一个nelem之后的子vec
 * @ret     可以返回empty vec
 */
std::vector<int> sub_num_vec_back_splited_by_elem(const std::vector<int> src_num_vec, const int nelem){
	//assert(!src_num_vec.empty());

	int index = src_num_vec.size()-1;
	for (; index >= 0; --index) {
		if (src_num_vec[index] == nelem)
			break;
	}
	assert(index >= 0); //保证存在


	return sub_num_vec(src_num_vec, index+1);

}

/**
 * 返回nelem 在num_vec的第一个下标
 */
int find_index(const std::vector<int> num_vec, const int nelem) {
	assert(!num_vec.empty());

	int index = 0;
	for (; index < num_vec.size(); ++index) {
		if (num_vec[index] == nelem)
			break;
	}assert(index >= 0);
	assert(index < num_vec.size());
	//保证存在

	return index;
}
