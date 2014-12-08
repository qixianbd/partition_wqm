/*
 * node.h
 *
 *  Created on: 2011-12-28
 *      Author: harry
 */

#ifndef NODE_H_
#define NODE_H_

#include <vector>
#include "common.h"

namespace util {

class node {
public:
	enum node_group_status{
		INIT,
		BIG,
		SMALL,
		MIDDLE
	};

	enum node_kind{ LOOP, NODE_GROUP};
    enum { EDGE_INF = 0x00FFFFFF};
private:
    std::vector<int> node_group;
    double edge_evalue;
    node_group_status status;
    node_kind kind;
    /**
	 * spawn_pos;是由外部由set方法传入，由get传出，外部负责内存回收
	 */
    tnle *spawn_pos;
public:
    //static const int EDGE_INF;
public:
    node();
    node(const std::vector<int> node_vec);
    virtual ~node();
    double getEdge_evalue() const;
    std::vector<int> getNode_group() const;
    tnle *getSpawn_pos() const;
    node_group_status getStatus() const;
    void setEdge_evalue(double edge_evalue);
    void setNode_group(std::vector<int> node_group);
    void setSpawn_pos(tnle *spawn_pos);
    void setStatus(node_group_status status);

    node_kind getKind() const;
    void setKind(node_kind kind);
    bool is_loop()const{return kind == LOOP;}
    bool is_node_group()const{return kind == NODE_GROUP;}


    /**
     * format:[1, 2, 3, 4]
     */
    void print(FILE *fp);
};

//const int EDGE_INF = 0x00FFFFFF;

DECLARE_LIST_CLASS(node_list, node *);

}

#endif /* NODE_H_ */
