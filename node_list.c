#include <stdio.h>
#include <stdlib.h>

#include "node_list.h"


///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
void *nodeList_pop(Node_t **list)
{
	if ((*list)==NULL)
		return (void *)NULL;

	Node_t *pop_node = (*list);

	(*list) = pop_node->next;
	if ((*list) != NULL)
		(*list)->prev = NULL;

	void *data = pop_node->data;
	free(pop_node);

	return data;
}

///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
void nodeList_push(Node_t **list, void *new_data)
{
	Node_t *new_node
		= (Node_t *)malloc(sizeof(Node_t));

	new_node->data = new_data;

	new_node->next = (*list);
	new_node->prev = NULL;

	if ((*list) != NULL)
		(*list)->prev = new_node;

	(*list) = new_node;
}

///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
void nodeList_create(Node_t **list)
{
	(*list) = NULL;
}


///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
void nodeList_destroy(Node_t **list)
{
	// traverse the list and free memory
	while( (*list) != NULL) {
		void *data = nodeList_pop(list);
		if ( ! data )
			free(data);
	}
}

