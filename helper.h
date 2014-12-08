/*
 * helper.h
 *
 *  Created on: 2012-2-16
 *      Author: harry
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <vector>

/**
 * +测试通过
 * [beginIndex, ...)
 * @param beginIndex - 起始索引，包括
 * 不允许返回empty vec
 */
std::vector<int> sub_num_vec(const std::vector<int> &src_num_vec, const int beginIndex);

/**
 * +测试通过
 * @descript - [beginIndex,endIndex)
 *             assert endIndex >= beginIndex ,return empty when endIndex == beginIndex
 * @param beginIndex - 起始索引，包括
 * @param endIndex   - 结束索引，不包括
 * 允许返回empty vec
 */
std::vector<int> sub_num_vec(const std::vector<int> &src_num_vec, const int beginIndex, const int endIndex);

/**
 * +测试通过
 * @pre-condition  src_num_vec contains nelem
 * @description 返回第一个nelem之前的子vec
 * @ret      可以返回empty vec
 */
std::vector<int> sub_num_vec_front_splited_by_elem(const std::vector<int> src_num_vec, const int nelem);


/**
 * +测试通过
 * @pre-condition  src_num_vec contains nelem
 * @description 返回最后一个nelem之后的子vec
 * @ret     可以返回empty vec
 */
std::vector<int> sub_num_vec_back_splited_by_elem(const std::vector<int> src_num_vec, const int nelem);

/**
 * 返回nelem 在num_vec的第一个下标
 */
int find_index(const std::vector<int> num_vec, const int nelem);
#endif /* HELPER_H_ */
