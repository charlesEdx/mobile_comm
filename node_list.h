
#ifndef _NODE_LIST_H
#define _NODE_LIST_H

#ifdef _cplusplus
exetern "C" {
#endif

// A linked list node
struct node_s {
	struct node_s	*next;
	struct node_s	*prev;
	void			*data;
};

typedef struct node_s Node_t;


void nodeList_push(Node_t **list, void *new_data);
void *nodeList_pop(Node_t **list);
void nodeList_create(Node_t **list);
void nodeList_destroy(Node_t **list);


#ifdef _cplusplus
}
#endif

#endif	// _NODE_LIST_H