/*
 * util.cpp
 *
 *  Created on: 2011-12-28
 *      Author: harry
 */

#include "node.h"

namespace util {

node::node() {
	this->node_group = std::vector<int>();
	this->edge_evalue = EDGE_INF;
	this->spawn_pos = NULL;
	this->status = INIT;
}

node::node(const std::vector<int> node_vec) {
	node();
	this->setNode_group(node_vec);
}

node::~node() {

}

double node::getEdge_evalue() const {
	return edge_evalue;
}

std::vector<int> node::getNode_group() const {
	return node_group;
}

tnle *node::getSpawn_pos() const {
	return spawn_pos;
}

node::node_group_status node::getStatus() const {
	return status;
}

void node::setEdge_evalue(double edge_evalue) {
	this->edge_evalue = edge_evalue;
}

void node::setNode_group(std::vector<int> node_group) {
	this->node_group = node_group;
}

void node::setSpawn_pos(tnle *spawn_pos) {
	this->spawn_pos = spawn_pos;
}

void node::setStatus(node_group_status status) {
	this->status = status;
}

node::node_kind node::getKind() const {
	return kind;
}

void node::setKind(node_kind kind) {
	this->kind = kind;
}

/**
 * non-loop format:[1, 2, 3, 4]
 * loop format:(4)
 */
void node::print(FILE *fp) {
	assert(this->node_group.size() > 0);
	fflush(fp);

	if (NODE_GROUP == this->getKind()) {
		fprintf(fp, "[%d", this->node_group[0]);
		fflush(fp);
		for (int i = 1; i < this->node_group.size(); ++i) {
			fprintf(fp, ", %d", this->node_group[i]);
		}
		fprintf(fp, "]");
	} else {
		fprintf(fp, "(%d)", this->node_group[0]);
	}
	fflush(fp);
}

}
