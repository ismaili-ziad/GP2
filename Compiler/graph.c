/* ///////////////////////////////////////////////////////////////////////////

  ================================
  graph.c - Chris Bak (14/07/2014)
  ================================

/////////////////////////////////////////////////////////////////////////// */

#include "graph.h"

Label blank_label = { .mark = NONE, .list = NULL, .list_length = 0,
                      .has_list_variable = false };

Graph *newGraph(void) 
{
    Graph *new_graph = malloc(sizeof(Graph));

    if(new_graph == NULL) 
    {
      printf("Memory exhausted during graph construction.\n");
      exit(1);
    }
    
    new_graph->nodes = calloc(MAX_NODES, sizeof(Node *));
    if(new_graph->nodes == NULL)
    {
      printf("Memory exhausted during graph construction.\n");
      exit(1);
    }
 
    new_graph->edges = calloc(MAX_EDGES, sizeof(Edge *));
    if(new_graph->edges == NULL)
    {
      printf("Memory exhausted during graph construction.\n");
      exit(1);
    }

    new_graph->free_node_slots = newStack(); 
    new_graph->free_edge_slots = newStack();
  
    new_graph->next_node_index = 0;
    new_graph->next_edge_index = 0;

    new_graph->number_of_nodes = 0;
    new_graph->number_of_edges = 0;

    /* Hash tables indexed by label class. g_direct_hash generates hash keys
     * from void pointers. Label classes are converted to pointers with the
     * G_INT_TO_POINTER macro so that they can be used as hash keys. It is
     * done this way because the integer hashing functions did not function
     * properly. */
    new_graph->nodes_by_label = g_hash_table_new(g_direct_hash, g_direct_equal);    
    new_graph->edges_by_label = g_hash_table_new(g_direct_hash, g_direct_equal);    

    new_graph->root_nodes = NULL;
 
    return new_graph;
}

/* Invariants on graphs:
 * (1) For 0 <= i <= graph->next_node_index, graph->next_node_index > 0,
 *     node_array[i] is either a pointer to a node or NULL. In the latter case,
 *     i is in the free node slot list.
 * (2) The number of non-NULL pointers in the node array is equal to 
 *     graph->number_of_nodes.
 * (3) For 0 <= i <= graph->next_edge_index, graph->next_edge_index > 0,
 *     edge_array[i] is either a pointer to a edge or NULL. In the latter case,
 *     i is in the free edge slot list.
 * (4) The number of non-NULL pointers in the edge array is equal to 
 *     graph->number_of_edges.
 * (5) For 0 <= i <= node->next_out_edge_index, next_out_edge_index > 0,
 *     outedges_array[i] is either a pointer to an edge or NULL. In the latter
 *     case, i is in the free out edge slot list.
 * (6) The number of non-NULL pointers in the outedges array is equal to 
 *     node->outdegree.
 * (7) For 0 <= i <= node->next_in_edge_index, next_in_edge_index > 0, 
 *     inedges_array[i] is either a pointer to an edge or NULL. In the latter
 *     case, i is in the free in edge slot list.
 * (8) The number of non-NULL pointers in the inedges array is equal to 
 *     node->indegree.
 * (9) Edge consistency: For all edges E referred to outside of the graph's
 *     edge array (i.e. in a node's inedge/outedge list), E is the edge pointed
 *     to by the (E.index)th pointer of the edge array.
 * (10) Node consistency: For all nodes N referred to outside of the graph's
 *      node (i.e. the edge's source and target), N is the node pointed to by
 *      the (N.index)th pointer of the node array.
 * (11) For all edges E, if S is E's source and T is E's target, then E is in
 *      S's outedge list and E is in T's inedge list (source/target consistency). 
 * (12) A node with label class L is in the nodes_by_label table entry indexed
 *      by the hash key L.
 * (13) An edge with label class L is in the edges_by_label table entry indexed
 *      by the hash key L.
 */

bool validGraph(Graph *graph)
{
   if(graph == NULL ||
     (graph->number_of_edges == 0 && graph->number_of_nodes == 0)) 
   {
      print_to_console("You asked me to validate the empty graph.\n"
                       "The empty graph trivially satisfies all the invariants.\n"
                       "Have a nice day!\n\n");
      return true;
   }

   bool valid_graph = true, slot_found = false;
   int graph_index, node_index, free_slot, node_count = 0, edge_count = 0;
   StackNode *iterator = NULL;
   
   for(graph_index = 0; graph_index < graph->next_node_index; graph_index++)    
   {
      Node *node = getNode(graph, graph_index);
    
      /* Invariant (1) */
      if(node == NULL) 
      { 
         iterator = graph->free_node_slots->top;

         while(iterator != NULL)
         {
            free_slot = iterator->data->free_slot;
            if(free_slot == graph_index) 
            {
               slot_found = true;
               break;
            }
            iterator = iterator->next;
         }

         if(!slot_found && graph->next_node_index > 0)
         {
            print_to_console("The graph does not satisfy the invariants.\n" 
                             "Pointer at node array index %d is NULL but the "
                             "index is not in the free node slot list.\n", 
                             graph_index);
            valid_graph = false;
         }
         slot_found = false;
      }     
      else
      {
         /* Keep a count of the number of nodes in the array. */
         node_count++;

         for(node_index = 0; node_index < node->next_out_edge_index; node_index++)
         {
            Edge *node_edge = getOutEdge(node, node_index);

            /* Invariant (5) */
            if(node_edge == NULL) 
            {
               iterator = node->free_out_edge_slots->top;

               while(iterator != NULL)
               {
                  free_slot = iterator->data->free_slot;
                  if(free_slot == node_index) 
                  {
                     slot_found = true;
                     break;
                  }
                  iterator = iterator->next;
               }

               if(!slot_found && node->next_out_edge_index > 0)
               {
                  print_to_console("The graph does not satisfy the invariants.\n"
                                   "Pointer at outedge array index %d is NULL "
                                   "but the index is not in the free outedge "
                                   "slot list.\n", graph_index);
                  valid_graph = false;
               }
               slot_found = false;
            }   
            else
            {
               /* Keep a count of the number of outedges in the array. */
               edge_count++;

               Edge *graph_edge = getEdge(graph, node_edge->index); 
               /* Invariant (9) */
               if(node_edge != graph_edge) 
               {
                  print_to_console("Graph does not satisfy the invariants.\nNode %d's"
                                   "outedge %d is inconsistent with the graph's edge "
                                   "table.\n", graph_index, node_edge->index);   
                  valid_graph = false;
               }
            }
         }
         /* Invariant (6) */
         if(node->outdegree != edge_count)
         {
            print_to_console("Graph does not satisfy the invariants.\nNode %d's"
                             "outdegree %d is not equal to the number of edges "
                             "%d in its outedges array.\n", node->index, 
                             node->outdegree, edge_count);
            valid_graph = false;
         }
         
         edge_count = 0;

         for(node_index = 0; node_index < node->next_in_edge_index; node_index++)
         {
            Edge *node_edge = getInEdge(node, node_index);

            /* Invariant (7) */
            if(node_edge == NULL) 
            {
               iterator = node->free_in_edge_slots->top;

               while(iterator != NULL)
               {
                  free_slot = iterator->data->free_slot;
                  if(free_slot == node_index) 
                  {
                     slot_found = true;
                     break;
                  }
                  iterator = iterator->next;
               }

               if(!slot_found && node->next_in_edge_index > 0)
               {
                  print_to_console("The graph does not satisfy the invariants.\n"
                                   "Pointer at outedge array index %d is NULL "
                                   "but the index is not in free outedge slot "
                                   "list.\n", graph_index);
                  valid_graph = false;
               }
               slot_found = false;
            }   
            else
            {
               /* Keep a count of the number of outedges in the array. */
               edge_count++;

               Edge *graph_edge = getEdge(graph, node_edge->index); 
               /* Invariant (9) */
               if(node_edge != graph_edge) 
               {
                  print_to_console("The graph does not satisfy the invariants.\n"
                                   "Node %d's outedge %d is inconsistent with the "
                                   "graph's edge table.\n", 
                                   graph_index, node_edge->index);   
                  valid_graph = false;
               }
            }
         }
         /* Invariant (8) */
         if(node->indegree != edge_count)
         {
            print_to_console("The graph does not satisfy the invariants.\nNode "
                             "%d's indegree %d is not equal to the number of "
                             "edges %d in its inedges array.\n", node->index, 
                             node->indegree, edge_count);
            valid_graph = false;
         } 

         edge_count = 0;

         /* Invariant (12) */
         GSList *node_list = getNodesByLabel(graph, node->label_class);

         if(g_slist_find(node_list, node) == NULL)
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Node %d does not occcur in the hash list of its "
                             "label class %d.\n", graph_index, 
                             node->label_class);   
            valid_graph = false;
         }   
      }
   }
   /* Invariant (2) */
   if(node_count != graph->number_of_nodes)
   {
      print_to_console("The graph does not satisfy the invariants.\nThe number "
                       "of nodes %d is not equal to the number of nodes in its "
                       "node array %d.\n", graph->number_of_nodes, node_count);
      valid_graph = false;
   }   

   for(graph_index = 0; graph_index < graph->next_edge_index; graph_index++)    
   {
      Edge *edge = getEdge(graph, graph_index);

      /* Invariant (3) */
      slot_found = false;

      if(edge == NULL) 
      { 
         StackNode *iterator = graph->free_edge_slots->top;

         while(iterator != NULL)
         {
            free_slot = iterator->data->free_slot;
            if(free_slot == graph_index) 
            {
               slot_found = true;
               break;
            }
            iterator = iterator->next;
         }

         if(!slot_found && graph->next_edge_index > 0)
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Pointer at edge array index %d is NULL but the "
                             "index is not in free edge slot list.\n", 
                             graph_index);
            valid_graph = false;
         }
      }    
      else
      {
         /* Keep a count of the number of edges in the array. */
         edge_count++;

         Node *source = getSource(edge); 
         Node *node = getNode(graph, source->index);
 
         /* Invariant (10) */
         if(source != node) 
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Edge %d's source %d is inconsistent with the "
                             "graph's node array.\n", graph_index, 
                             source->index);   
            valid_graph = false;
         }   
   
         int counter;
         bool edge_found = false;

         /* Search for edge in the out_edges array of its source.  */
         for(counter = 0; counter < source->outdegree; counter++)
         {
            Edge *out_edge = getOutEdge(source, counter);
            if(out_edge == edge)
            {
               edge_found = true;
               break;
            }
         }
 
         /* Invariant (11) */
         if(!edge_found)
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Edge %d does not occur in node %d's outedge"
                             "array.\n", graph_index, source->index);   
            valid_graph = false;
         }   


         Node *target = getTarget(edge); 
         node = getNode(graph, target->index);

         /* Invariant (10) */
         if(target != node) 
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Edge %d's target %d is inconsistent with the "
                             "graph's node array.\n", graph_index, 
                             target->index);   
            valid_graph = false;
         }   

         edge_found = false;

         /* Search for edge in the in_edges array of its source.  */
         for(counter = 0; counter < target->indegree; counter++)
         {
            Edge *in_edge = getInEdge(target, counter);
            if(in_edge == edge)
            {
               edge_found = true;
               break;
            }
         }
 
         /* Invariant (11) */
         if(!edge_found)
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Edge %d does not occur in node %d's inedge "
                             "array.\n", graph_index, target->index);   
            valid_graph = false;
         }   
      
   
         /* Invariant 139) */
         GSList *edge_list = getEdgesByLabel(graph, edge->label_class);

         if(g_slist_find(edge_list, edge) == NULL)
         {
            print_to_console("The graph does not satisfy the invariants.\n"
                             "Edge %d does not occcur in the hash list of its "
                             "label class %d.\n", graph_index, 
                             edge->label_class);   
            valid_graph = false;
         }  
      }
   }
   /* Invariant (4) */
   if(edge_count != graph->number_of_edges)
   {
      print_to_console("The graph does not satisfy the invariants.\nThe number "
                       "of edges %d is not equal to the number of edges in its "
                       "edge array %d.\n", graph->number_of_edges, edge_count);
      valid_graph = false;
   }     
    
   if(valid_graph) print_to_console ("Graph satisfies all the data invariants!\n");
   printf("\n");
   return valid_graph;
}


LabelClass getLabelClass(Label *label)
{
   int length = label->list_length;

   if(label->has_list_variable) return LISTVAR_L;
   if(label->list == NULL) return EMPTY_L;
   if(length > 1)
   {
      switch(length)
      {
         case 2: return LIST2_L;
         case 3: return LIST3_L;
         case 4: return LIST4_L;
         case 5: return LIST5_L;

         default: print_to_log("Error (getLabelClass): The length of the " 
                               "passed list exceeds the GP 2 maximum.\n");
      }
   }

   /* The list has length 1. */
   ListElement *atom = (ListElement*)label->list->data;
   
   switch(atom->type)
   {
      case VARIABLE:

           return ATOMIC_VAR_L;     

      case INTEGER_CONSTANT:

      case NEG:

           return INT_L;

      case CHARACTER_CONSTANT:

      case STRING_CONSTANT:

      case CONCAT:

           return STRING_L;

      default:
           print_to_log("Error (getLabelClass): First element of passed list "
                        "has unexpected type %d.\n", atom->type);
           break;
   }

   return LISTVAR_L;
}
       
   

Node *newNode(bool root, Label *label)
{
   Node *node = malloc(sizeof(Node));

   if(node == NULL) 
   {
      print_to_log("Memory exhausted during node construction.\n");
      exit(1);
   }
 
   node->index = 0;
   node->root = root;
   if(label == NULL)
   {
      node->label_class = EMPTY_L;
      node->label = &blank_label;
   }
   else
   { 
      node->label_class = getLabelClass(label);
      node->label = label;
   }
   node->indegree = 0;
   node->outdegree = 0;

   node->out_edges = calloc(MAX_INCIDENT_EDGES, sizeof(Edge *));

   if(node->out_edges == NULL)
   {
      printf("Memory exhausted during node construction.\n");
      exit(1);
   }

   node->in_edges = calloc(MAX_INCIDENT_EDGES, sizeof(Edge *));

   if(node->in_edges == NULL)
   {
      printf("Memory exhausted during node construction.\n");
      exit(1);
   }

   node->free_out_edge_slots = newStack();
   node->free_in_edge_slots = newStack();

   node->next_out_edge_index = 0;
   node->next_in_edge_index = 0;

   return node;
}


Edge *newEdge(bool bidirectional, Label *label, Node *source,
              Node *target)
{
   Edge *edge = malloc(sizeof(Edge));

   if(edge == NULL) {
      print_to_log("Memory exhausted during node construction.\n");
      exit(1);
   }
	
   edge->index = 0;
   edge->bidirectional = bidirectional;
   if(label == NULL)
   {
      edge->label_class = EMPTY_L;
      edge->label = &blank_label;
   }
   else
   { 
      edge->label_class = getLabelClass(label);
      edge->label = label;
   }
   edge->source = source;
   edge->target = target;
   
   return edge;
}
	

void addNode(Graph *graph, Node *node) 
{
    /* Add the node to the pointer array. First check the free node slots stack
     * to see if there are any holes in the node array. */
    StackData *data = pop(graph->free_node_slots);
    if(data == NULL) 
    {
       graph->nodes[graph->next_node_index] = node;
       node->index = graph->next_node_index;
       graph->next_node_index++;
    }
    else 
    {
       graph->nodes[data->free_slot] = node;
       node->index = data->free_slot;
       free(data);
    }

    /* Update graph->nodes_by_label by adding the new node to the appropriate
     * node list and inserting the updated list into the hash table. */
    void *label_key = GINT_TO_POINTER(node->label_class);
    GSList *node_list = g_hash_table_lookup(graph->nodes_by_label, label_key);
    node_list = g_slist_prepend(node_list, node);
    g_hash_table_insert(graph->nodes_by_label, label_key, node_list);
    
    /* Update graph->root_nodes. */
    if(node->root) graph->root_nodes = g_slist_prepend(graph->root_nodes, node);

    graph->number_of_nodes++;
}


void addEdge(Graph *graph, Edge *edge) 
{
    /* Add the edge to the pointer array. First check the free edge slots stack
     * to see if there are any holes in the edge array. */
    StackData *data = pop(graph->free_edge_slots);
    if(data == NULL) 
    {
       graph->edges[graph->next_edge_index] = edge;
       edge->index = graph->next_edge_index;
       graph->next_edge_index++;
    }
    else
    { 
       graph->edges[data->free_slot] = edge;    
       edge->index = data->free_slot;
       free(data);
    }

    /* Update the source and target nodes with the new edge: first add the
     * edge to the out_edges/in_edges array and then update the degree. */ 
    Node *source = edge->source;

    data = pop(source->free_out_edge_slots);
    if(data == NULL) 
    {
       source->out_edges[source->next_out_edge_index] = edge;
       source->next_out_edge_index++;
    }
    else 
    {
       source->out_edges[data->free_slot] = edge;
       free(data);
    }

    source->outdegree++;

    Node *target = edge->target;

    data = pop(target->free_in_edge_slots);
    if(data == NULL) 
    {
       target->in_edges[target->next_in_edge_index] = edge;
       target->next_in_edge_index++;
    }
    else 
    {
       target->in_edges[data->free_slot] = edge;
       free(data);
    }

    target->indegree++;

    /* Update graph->nodes_by_label by adding the new edge to the appropriate
     * edge list and inserting the updated list into the hash table. */
    void *label_key = GINT_TO_POINTER(edge->label_class);
    GSList *edge_list = g_hash_table_lookup(graph->edges_by_label, label_key);
    edge_list = g_slist_prepend(edge_list, edge);
    g_hash_table_insert(graph->edges_by_label, label_key, edge_list);

    graph->number_of_edges++;
}


void removeNode(Graph *graph, int index)
{   
    Node *node = getNode(graph, index);  
 
    if(node->indegree > 0 || node->outdegree > 0)
    {
       print_to_console("Error (removeNode): Cannot remove node (%d) with "
                        "incident edges.\n", node->index);
       print_to_log("Error (removeNode): Cannot remove node (%d) with "
                    "incident edges.\n", node->index);
       return;
    }

    void *label_key = GINT_TO_POINTER(node->label_class);

    /* Remove the node from the label classes hash table by removing the
     * node from the node list and updating the hash table entry. */
    GSList *node_list = g_hash_table_lookup(graph->nodes_by_label, label_key);
    node_list = g_slist_remove(node_list, node);

    /* If the node list is empty, remove the key from the hash table. Otherwise
     * overwrite the hash table entry with the updated node list. */
    if(node_list == NULL) g_hash_table_remove(graph->nodes_by_label, label_key);
    else g_hash_table_insert(graph->nodes_by_label, label_key, node_list); 

    /* Remove the node from the root node list if necessary. */
    if(node->root) graph->root_nodes = g_slist_remove(graph->root_nodes, node); 

    /* Remove the node from the pointer array and set the pointer to NULL. */
    freeNode(node);
    graph->nodes[index] = NULL;

    /* If the node's index is the last index in the array, it is not 
     * necessary to create a free slot: only decrement next_node_index.
     */
    if(index == graph->next_node_index - 1) graph->next_node_index--;
    else
    {
       StackData *data = malloc(sizeof(StackData));
       if(data == NULL)
       {
          print_to_log("Error (removeNode): Memory exhausted during free slot "
                       "construction.\n");
          exit(1);
       }
       data->free_slot = index;
       push(graph->free_node_slots, data);
    }
    graph->number_of_nodes--;
}


void removeEdge(Graph *graph, int index) 
{
    /* Update the source and target nodes: first decrement the appropriate
     * degree and then remove the edge from the out_edges/in_edges pointer 
     * arrays. */ 
    Edge *edge = getEdge(graph, index);
 
    /* Find the edge in the source's out_edges array and set the pointer to 
     * NULL. */
    Node *source = edge->source;
    int counter = 0;
    while(counter < source->next_out_edge_index)
    {
       if(source->out_edges[counter] == edge) 
       {
          source->out_edges[counter] = NULL;
          source->outdegree--;

          /* If the source's index is the last index in the array, it is not 
           * necessary to create a free slot: only decrement
           * next_out_edge_index.
           */
          if(counter == source->next_out_edge_index - 1) 
             source->next_out_edge_index--;
          else
          {
             StackData *data = malloc(sizeof(StackData));
             if(data == NULL)
             {
                print_to_log("Error (removeEdge): Memory exhausted during "
                             "free outedge slot construction.\n");
                exit(1);
             }
             data->free_slot = counter;
             push(source->free_out_edge_slots, data);
          }
          break;
       } 
       else counter++;
    }

    /* Repeat for the target's in_edges array. */
    Node *target = edge->target;
    counter = 0; 
    while(counter < target->next_in_edge_index)
    {
       if(target->in_edges[counter] == edge) 
       {
          target->in_edges[counter] = NULL;
          target->indegree--;

          /* If the target's index is the last index in the array, it is not 
           * necessary to create a free slot: only decrement
           * next_in_edge_index.
           */
          if(counter == target->next_in_edge_index - 1) 
             target->next_in_edge_index--;
          else
          {
             StackData *data = malloc(sizeof(StackData));
             if(data == NULL)
             {
                print_to_log("Error (removeEdge): Memory exhausted during "
                             "free inedge slot onstruction.\n");
                exit(1);
             }
             data->free_slot = counter;
             push(target->free_in_edge_slots, data);
          }

          break;
       } 
       else counter++;
    }

    void *label_key = GINT_TO_POINTER(edge->label_class);

    /* Remove the edge from the label classes hash table by removing the
     * edge from the edge list and inserting the updated list into
     * the hash table. */
    GSList *edge_list = g_hash_table_lookup(graph->edges_by_label, label_key);
    edge_list = g_slist_remove(edge_list,edge);

    /* If the edge list is empty, remove the key from the hash table. Otherwise
     * overwrite the hash table entry with the updated edge list. */
    if(edge_list == NULL) g_hash_table_remove(graph->edges_by_label, label_key);
    else g_hash_table_insert(graph->edges_by_label, label_key, edge_list); 

    /* Remove the edge from the pointer array and set the pointer to NULL. */
    freeEdge(edge);
    graph->edges[index] = NULL;

    /* If the edge's index is the last index in the array, it is not 
     * necessary to create a free slot: only decrement next_edge_index.
     */
    if(index == graph->next_edge_index - 1) graph->next_edge_index--;
    else
    {
       StackData *data = malloc(sizeof(StackData));
       if(data == NULL)
       {
          print_to_log("Error (removeEdge): Memory exhausted during free slot "
                       "construction.\n");
          exit(1);
       }
       data->free_slot = index;
       push(graph->free_edge_slots, data);
    }
    graph->number_of_edges--;
}


void relabelNode(Graph *graph, Node *node, Label *new_label, bool change_label,
                 bool change_root) 
{
    if(change_root)
    {
       if(node->root)
       {
          node->root = false;
          graph->root_nodes = g_slist_remove(graph->root_nodes, node);
       }
       else
       {
          node->root = true;
          graph->root_nodes = g_slist_prepend(graph->root_nodes, node);
       }
    }

    if(change_label == false) return;
    else
    {  
       /* node->label is freed before being pointed to new_label. */
       if(node->label != &blank_label) freeLabel(node->label); 

       LabelClass new_label_class;
       if(new_label == NULL)
       {          
          node->label = &blank_label; 
          new_label_class = EMPTY_L;
       }
       else
       {
          node->label = new_label;
          new_label_class = getLabelClass(new_label);
       }
       /* If the label classes differ, the graph's nodes_by_label table needs to 
        * be updated. */
       if(node->label_class != new_label_class) 
       {
          void *hash_key_1 = GINT_TO_POINTER(node->label_class);
          void *hash_key_2 = GINT_TO_POINTER(new_label_class);
          node->label_class = new_label_class;

          /* Remove the node from the list indexed by its old label class. */
          GSList *old_node_list = 
             g_hash_table_lookup(graph->nodes_by_label, hash_key_1);
          old_node_list = g_slist_remove(old_node_list, node); 
          g_hash_table_replace(graph->nodes_by_label, hash_key_1, old_node_list);

          /* Add the node to the list indexed by its new label class. */
          GSList *new_node_list = 
              g_hash_table_lookup(graph->nodes_by_label, hash_key_2);
          new_node_list = g_slist_prepend(new_node_list, node);
          g_hash_table_replace(graph->nodes_by_label, hash_key_2, new_node_list);
       }   
    }
}

void relabelEdge(Graph *graph, Edge *edge, Label *new_label, 
                 bool change_label, bool change_bidirectional)
{		
    if(change_bidirectional) edge->bidirectional = !edge->bidirectional;

    /* If record is true, then edge->label is pointed to by the graph
     * change created above. Otherwise, edge->label should be freed
     * before it is pointed to new_label. */

    if(change_label == false) return;
    else
    {
       /* edge->label is freed before being pointed to new_label. */
       if(edge->label != &blank_label) freeLabel(edge->label); 

       LabelClass new_label_class;
       if(new_label == NULL)
       {          
          edge->label = &blank_label; 
          new_label_class = EMPTY_L;
       }
       else
       {
          edge->label = new_label;
          new_label_class = getLabelClass(new_label);
       }
       /* If the label classes differ, the graph's edges_by_label table needs to
        * be updated. */
       if(edge->label_class != new_label_class) 
       {
          void *hash_key_1 = GINT_TO_POINTER(edge->label_class);
          void *hash_key_2 = GINT_TO_POINTER(new_label_class);
          edge->label_class = new_label_class;

          /* Remove the edge from the list indexed by its old label class. */
          GSList *old_edge_list = 
              g_hash_table_lookup(graph->edges_by_label, hash_key_1);
          old_edge_list = g_slist_remove(old_edge_list, edge); 
          g_hash_table_replace(graph->edges_by_label, hash_key_1, old_edge_list);

          /* Add the edge to the list indexed by its new label class. */
          GSList *new_edge_list = 
              g_hash_table_lookup(graph->edges_by_label, hash_key_2);
          new_edge_list = g_slist_prepend(new_edge_list, edge);
          g_hash_table_replace(graph->edges_by_label, hash_key_2, new_edge_list);
       }   
    }
}


Stack *graph_stack = NULL;

void copyGraph (Graph *graph)
{
   /* Need to check if arrays in the original graph have increased in size. */
   Graph *graph_copy = newGraph();

   graph_copy->next_node_index = graph->next_node_index;
   graph_copy->next_edge_index = graph->next_edge_index;

   graph_copy->number_of_nodes = graph->number_of_nodes;
   graph_copy->number_of_edges = graph->number_of_edges;

   int index;

   /* Copy all nodes and edges with their labels. Note that the index of each
    * node and edge copy is the same as the index of the original. This is
    * important for copying the rest of the graph structure.

    * Edges are copied first so that the node's incident edge arrays can
    * be populated with the pointers to the edge copies. */
   for(index = 0; index < graph->next_edge_index; index++)
   {
      Edge *edge = getEdge(graph, index);

      if(edge == NULL) 
      {
         StackData *data = malloc(sizeof(StackData));
         if(data == NULL)
         {
            print_to_log("Error (copyGraph): Memory exhausted during free edge "
                         "slot construction.\n");
            exit(1);
         }
         data->free_slot = index;
         push(graph_copy->free_edge_slots, data);
      }
      else
      {
         Edge *edge_copy = malloc(sizeof(Edge));
         if(edge_copy == NULL)
         {
            print_to_log("Memory exhausted during graph copying.\n");
            exit(1);
         }    
         edge_copy = memcpy(edge_copy, edge, sizeof(Edge));
         edge_copy->label = copyLabel(edge->label);
         graph_copy->edges[index] = edge_copy;

         /* Populate the edge-by-label-class hash table of the graph copy. */
         void *hash_key = GINT_TO_POINTER(edge_copy->label_class);

         GSList *edge_list = getEdgesByLabel(graph_copy, edge_copy->label_class);
         edge_list = g_slist_prepend(edge_list, edge_copy);
         g_hash_table_insert(graph_copy->edges_by_label, hash_key, edge_list);     
      }
   }

   for(index = 0; index < graph->next_node_index; index++)
   {
      Node *node = getNode(graph, index);
      
      if(node == NULL)
      {
         StackData *data = malloc(sizeof(StackData));
         if(data == NULL)
         {
            print_to_log("Error (copyGraph): Memory exhausted during free node "
                         "slot construction.\n");
            exit(1);
         }
         data->free_slot = index;
         push(graph_copy->free_node_slots, data);
      }
      else 
      {
         Node *node_copy = malloc(sizeof(Node)); 
         if(node_copy == NULL)
         {
            print_to_log("Memory exhausted during graph copying.\n");
            exit(1);
         }   
         node_copy = memcpy(node_copy, node, sizeof(Node));
         node_copy->label = copyLabel(node->label);
         node_copy->free_out_edge_slots = newStack();
         node_copy->free_in_edge_slots = newStack();

         /* Create and populate a new out_edge array since the node copy points
          * to the same array as the original node. */
         int counter;

         node_copy->out_edges = calloc(MAX_INCIDENT_EDGES, sizeof(Edge *));
         if(node->out_edges == NULL)
         {
            printf("Memory exhausted during graph copying.\n");
            exit(1);
         }

         for(counter = 0; counter < node->next_out_edge_index; counter++)
         {
            Edge *edge = getOutEdge(node, counter);

            if(edge == NULL) 
            {
               StackData *data = malloc(sizeof(StackData));
               if(data == NULL)
               {
                  print_to_log("Error (copyGraph): Memory exhausted during "
                               "free outedge slot construction.\n");
                  exit(1);
               }
               data->free_slot = counter;
               push(node_copy->free_out_edge_slots, data);
            }
            else
            {
               Edge *edge_copy = getEdge(graph_copy, edge->index);
               node_copy->out_edges[counter] = edge_copy;
            }
         }

         /* Create and populate a new in_edge array. */
         node_copy->in_edges = calloc(MAX_INCIDENT_EDGES, sizeof(Edge *));
         if(node->in_edges == NULL)
         {
            printf("Memory exhausted during graph copying.\n");
            exit(1);
         }
         for(counter = 0; counter < node->next_in_edge_index; counter++)
         {
            Edge *edge = getInEdge(node, counter);

            if(edge == NULL) 
            {
               StackData *data = malloc(sizeof(StackData));
               if(data == NULL)
               {
                  print_to_log("Error (copyGraph): Memory exhausted during "
                               "free inedge slot construction.\n");
                  exit(1);
               }
               data->free_slot = counter;
               push(node_copy->free_in_edge_slots, data);
            }
            else
            {
               Edge *edge_copy = getEdge(graph_copy, edge->index);
               node_copy->in_edges[counter] = edge_copy;
            }
         }

         graph_copy->nodes[index] = node_copy;

         /* Populate the node-by-label-class hash table of the graph copy. */
         void *hash_key = GINT_TO_POINTER(node_copy->label_class);

         GSList *node_list = getNodesByLabel(graph_copy, node_copy->label_class);
         node_list = g_slist_prepend(node_list, node_copy);
         g_hash_table_insert(graph_copy->nodes_by_label, hash_key, node_list);     

         /* Populate the root nodes list. */
         if(node_copy->root)
         {
            graph_copy->root_nodes = g_slist_prepend(graph_copy->root_nodes, 
                                                     node_copy);
         }
      }
   }

   /* Iterate through the copied edges to update their source and target
    * pointers. */
   for(index = 0; index < graph_copy->next_edge_index; index++)
   {
      Edge *edge_copy = getEdge(graph_copy, index);

      Node *source = getSource(edge_copy);
      Node *source_copy = getNode(graph_copy, source->index);
      edge_copy->source = source_copy;

      Node *target = getTarget(edge_copy);
      Node *target_copy = getNode(graph_copy, target->index);
      edge_copy->target = target_copy;
   }  
 
   if(graph_stack == NULL) graph_stack = newStack();
   
   StackData *data = malloc(sizeof(StackData));
   if(data == NULL)
   {
      print_to_log("Error (copyGraph): Memory exhausted during graph "
                   "construction.\n");
      exit(1);
   }
   data->graph = graph_copy;
   push(graph_stack, data);
}

Graph *restoreGraph(Graph *graph)
{ 
   freeGraph(graph);
   StackData *data = pop(graph_stack);
   Graph *output = data->graph;
   free(data);
   return output;
}

void freeGraphStack(Stack *graph_stack)
{
   /* Before calling freeStack, free all the pointers to graphs. */
   StackNode *iterator = graph_stack->top;

   while(iterator != NULL)
   {
      if(iterator->data->graph) freeGraph(iterator->data->graph);
      iterator = iterator->next;
   }
   freeStack(graph_stack);
}


Label *copyLabel(Label *label)
{
   if(label == &blank_label) return &blank_label;

   Label *label_copy = malloc(sizeof(Label));

   if(label_copy == NULL) {
      print_to_log("Memory exhausted during label copying.\n");
      exit(1);
   }

   if(label->list == NULL) label_copy->list = NULL;
   else
   {
      GList *list_copy = NULL;
      /* The list is copied in reverse order so that elements are prepended at
      * each step. */
      GList *list_to_copy = g_list_last(label->list);

      while(list_to_copy != NULL)
      {
         ListElement *atom = (ListElement*)list_to_copy->data;
         ListElement *atom_copy = copyListElement(atom);
         list_copy = g_list_prepend(list_copy, atom_copy);
         list_to_copy = list_to_copy->prev;
      }

      label_copy->mark = label->mark;
      label_copy->list = list_copy;
      label_copy->list_length = label->list_length;
      label_copy->has_list_variable = label->has_list_variable;
   }
   return label_copy;
}


ListElement *copyListElement(ListElement *atom)
{ 
   ListElement *atom_copy = malloc(sizeof(ListElement));

   if(atom_copy == NULL)
   {
      print_to_log("Error (copyListElement): Memory exhausted during "
                   "creation of a list element.\n");
      exit(1);
   }

   /* Duplicate any string values and recursively copy any sub-expressions. */
   atom_copy->type = atom->type;

   switch(atom->type)
   {
      case VARIABLE:

           atom_copy->value.name = strdup(atom->value.name);

           break;
      
      case INTEGER_CONSTANT:

           atom_copy->value.number = atom->value.number;

           break;

      case CHARACTER_CONSTANT:

      case STRING_CONSTANT:

           atom_copy->value.string = strdup(atom->value.string);

           break;

      case NEG:
 
           atom_copy->value.exp = copyListElement(atom->value.exp);

           break;

      case CONCAT:

           atom_copy->value.bin_op.left_exp = 
              copyListElement(atom->value.bin_op.left_exp);
           atom_copy->value.bin_op.right_exp = 
              copyListElement(atom->value.bin_op.right_exp);

           break;

      default:
             print_to_log("Error (copyListElement): Atom type %d should not "
                          "occur here.\n", atom->type);
             return NULL;
   }

   return atom_copy;
}


/* Querying functions */

Node *getNode(Graph *graph, int index)
{
   if(index > graph->next_node_index) 
   {
      print_to_log("Error (getNode): Passed index exceeds node size of the "
                   "graph.\n");
      return NULL;
   }
   else return graph->nodes[index];
}

Edge *getEdge(Graph *graph, int index)
{
   if(index > graph->next_edge_index) 
   {
      print_to_log("Error (getEdge): Passed index exceeds edge size of the "
                   "graph.\n");
      return NULL;
   }
   else return graph->edges[index];
}

GSList *getRootNodes(Graph *graph)
{
   return graph->root_nodes;
}


GSList *getNodesByLabel(Graph *graph, LabelClass label_class) 
{
   void *hash_key = GINT_TO_POINTER(label_class);
   return g_hash_table_lookup(graph->nodes_by_label, hash_key);
}
   

GSList *getEdgesByLabel(Graph *graph, LabelClass label_class) 
{
   void *hash_key = GINT_TO_POINTER(label_class);
   return g_hash_table_lookup(graph->edges_by_label, hash_key);
}


Edge *getOutEdge(Node *node, int index)
{
   if(index > node->next_out_edge_index) 
   {
      print_to_log("Error (getOutEdge): Passed index exceeds size of the "
                   "node's out_edges array.\n");
      return NULL;
   }
   else return node->out_edges[index];
}


Edge *getInEdge(Node *node, int index)
{
   if(index > node->next_in_edge_index) 
   {
      print_to_log("Error (getInEdge): Passed index exceeds size of the "
                   "node's in_edges array.\n");
      return NULL;
   }
   else return node->in_edges[index];
}


Node *getSource(Edge *edge) 
{
   return edge->source;
}

Node *getTarget(Edge *edge) 
{
   return edge->target;
}


Label *getNodeLabel(Node *node) 
{
   return node->label;
}

Label *getEdgeLabel(Edge *edge) 
{
   return edge->label;
}

int getIndegree (Node *node) 
{
   return node->indegree;
}

int getOutdegree (Node *node) 
{
   return node->outdegree;
}


void printGraph(Graph *graph) 
{
   int index, node_count = 0, edge_count = 0;

   if(graph == NULL) 
   {
      printf("[ | ]\n");
      return;
   }

   if(graph->number_of_nodes == 0) 
   {
      printf("[ | ]\n");
      return;
   }

   printf("[ ");
   for(index = 0; index < graph->next_node_index; index++)
      if(graph->nodes[index]) 
      {
         /* Five nodes per line */
         if(node_count != 0 && node_count % 5 == 0) printf("\n  ");
         node_count++;

         Node *node = getNode(graph, index);
         if(node->root)
              printf("(n%d(R), ", index);
         else printf("(n%d, ", index);
         if(node->label->list) printList(node->label->list);
         else printf("empty");
         printMark(node->label->mark, false);
         printf(") ");
      }
 
   if(graph->number_of_edges == 0)
   {
      printf("| ]\n");
      return;
   }

   printf("|\n  ");
   for(index = 0; index < graph->next_edge_index; index++)
      if(graph->edges[index]) 
      {
         /* Three edges per line */
         if(edge_count != 0 && edge_count % 3 == 0) printf("\n  ");
         edge_count++;

         Edge *edge = getEdge(graph, index);
         if(edge->bidirectional)
              printf("(e%d(B), ", index);
         else printf("(e%d, ", index);
         printf("n%d, n%d, ", edge->source->index, edge->target->index);
         if(edge->label->list) printList(edge->label->list);
         else printf("empty");
         printMark(edge->label->mark, false);
         printf(") ");
      }
   printf("]\n\n");
}

void printVerboseGraph(Graph *graph) 
{
    int index;
    printf("Nodes\n=====\n");
    for(index = 0; index < graph->next_node_index; index++)
       if(graph->nodes[index]) printVerboseNode(graph->nodes[index]);
 
    printf("Edges\n=====\n");
    for(index = 0; index < graph->next_edge_index; index++)
       if(graph->edges[index]) printVerboseEdge(graph->edges[index]);
    printf("\n");

    printf("Root Node List\n==============\n");
    GSList *iterator = graph->root_nodes;
    while(iterator != NULL)
    {
       Node *node = (Node *)iterator->data;
       printVerboseNode(node); 
       iterator = iterator->next;
    }
    printf("\n");
}

void printVerboseNode(Node *node)
{
    printf("Index: %d", node->index);
    if(node->root) printf(" (Root)");
    printf("\n");
    printf("Label Class: %d\n", node->label_class);
    printf("Label: ");
    if(node->label->list) 
    {
       printList(node->label->list);
       printf("\n");
    }
    else printf("empty\n");
    printMark(node->label->mark, true);
    printf("Indegree: %d. Outdegree: %d\n\n", node->indegree, node->outdegree);
}

void printVerboseEdge(Edge *edge) 
{
    printf("Index: %d", edge->index);
    if(edge->bidirectional) printf(" (Bidirectional)");
    printf("\n");
    printf("Label Class: %d\n", edge->label_class);
    printf("Label: ");
    if(edge->label->list) 
    {
       printList(edge->label->list);
       printf("\n");
    }
    else printf("empty\n");
    printMark(edge->label->mark, true);
    printf("\n");
    printf("Source: %d. Target: %d\n\n", edge->source->index, edge->target->index);
    printf("\n");
}

void printList(GList *list) 
{
    while(list != NULL) 
    {
        printListElement((ListElement*)list->data);
        if(list->next) printf(" : ");
        list = list->next;
    } 
}

void printListElement(ListElement* elem) 
{
    switch(elem->type) 
    {
	case VARIABLE: 
	     printf("%s", elem->value.name);
	     break;

	case INTEGER_CONSTANT: 
	     printf("%d", elem->value.number);
	     break;

	case CHARACTER_CONSTANT:
	     printf("\"%s\"", elem->value.string);
	     break;

	case STRING_CONSTANT:
	     printf("\"%s\"", elem->value.string);
	     break;

	case INDEGREE:
	     printf("indeg(%s)", elem->value.node_id);
	     break;
 
	case OUTDEGREE:
	     printf("outdeg(%s)", elem->value.node_id);
	     break;

	case LIST_LENGTH:
	     printf("llength(");
	     printList(elem->value.list_arg);
	     printf(")");
	     break;

	case STRING_LENGTH:
	     printf("slength(");
	     printListElement(elem->value.str_arg);
	     printf(")");
	     break;

	case NEG:
	     printf("- ");
	     printListElement(elem->value.exp);
	     break;

	case ADD:
	     printf("(");
	     printListElement(elem->value.bin_op.left_exp);
	     printf(" + ");
	     printListElement(elem->value.bin_op.right_exp);
	     printf(")");
	     break;

	case SUBTRACT:
	     printf("(");
	     printListElement(elem->value.bin_op.left_exp);
	     printf(" - ");
	     printListElement(elem->value.bin_op.right_exp);
	     printf(")");
	     break;

	case MULTIPLY:
	     printf("(");
	     printListElement(elem->value.bin_op.left_exp);
	     printf(" * ");
	     printListElement(elem->value.bin_op.right_exp);
	     printf(")");
	     break;

	case DIVIDE:
	     printf("(");
	     printListElement(elem->value.bin_op.left_exp);
	     printf(" / ");
	     printListElement(elem->value.bin_op.right_exp);
	     printf(")");
	     break;

	case CONCAT:
	     printf("(");
	     printListElement(elem->value.bin_op.left_exp);
	     printf(" . ");
	     printListElement(elem->value.bin_op.right_exp);
	     printf(")");
	     break;

	default: printf("Unexpected List Element Type: %d\n",
		       (int)elem->type); 
		 break;
    }
}

void printMark(MarkType mark, bool verbose)
{
   switch(mark)
   {
      case NONE:

           break;

      case RED:
         
           if(verbose) printf("Mark: Red\n");
           else printf(" # red");

           break;

      case GREEN:

           if(verbose) printf("Mark: Green\n");
           else printf(" # green");

           break;

      case BLUE:

           if(verbose) printf("Mark: Blue\n");
           else printf(" # blue");

           break;

      case GREY:

           if(verbose) printf("Mark: Grey\n");
           else printf(" # grey");

           break;

      case DASHED:

           if(verbose) printf("Mark: Dashed\n");
           else printf(" # dashed");

           break;

      case ANY:

           if(verbose) printf("Mark: Any\n");
           else printf(" # any");

           break;

      default:
           print_to_log("Error (printMark): Unexpected mark type %d\n", mark);

           break;
   }
}

void freeGraph(Graph *graph) 
{
   if(graph == NULL) return;

   int index;

   for(index = 0; index < graph->next_node_index; index++)
      if(graph->nodes[index]) freeNode(graph->nodes[index]);
   if(graph->nodes) free(graph->nodes);

   for(index = 0; index < graph->next_edge_index; index++)
      if(graph->edges[index]) freeEdge(graph->edges[index]);
   if(graph->edges) free(graph->edges);

   if(graph->free_node_slots) freeStack(graph->free_node_slots);
   if(graph->free_edge_slots) freeStack(graph->free_edge_slots);

   if(graph->nodes_by_label) 
   {
      g_hash_table_foreach(graph->nodes_by_label, freeGSList, NULL);
      g_hash_table_destroy(graph->nodes_by_label); 
   }

   if(graph->edges_by_label) 
   {
       g_hash_table_foreach(graph->edges_by_label, freeGSList, NULL);
       g_hash_table_destroy(graph->edges_by_label);  
   }

   if(graph->root_nodes) g_slist_free(graph->root_nodes);

   /*if(graph->graph_change_stack)
   {
      while(graph->graph_change_stack->top != NULL)
      {
         GraphChange *change = (GraphChange*)pop(graph->graph_change_stack);
         freeGraphChange(change);
      }
      free(graph->graph_change_stack);
   }*/
   free(graph);
}

void freeNode(Node *node) 
{
   if(node == NULL) return;
   if(node->label && node->label != &blank_label) freeLabel(node->label);
   if(node->in_edges) free(node->in_edges);
   if(node->out_edges) free(node->out_edges);
   if(node->free_out_edge_slots) freeStack(node->free_out_edge_slots);
   if(node->free_in_edge_slots) freeStack(node->free_in_edge_slots);
   free(node);
}

void freeEdge(Edge *edge) 
{
   if(edge == NULL) return;
   if(edge->label && edge->label != &blank_label) freeLabel(edge->label);
   free(edge);
}


void freeGSList(gpointer key, gpointer value, gpointer data) 
{
    g_slist_free(value);
}

void freeLabel(Label *label)
{
   if(label == NULL) return;
   if(label->list) g_list_free_full(label->list, freeListElement);
   free(label);
}

void freeListElement(void *atom)
{
   if(atom == NULL) return;

   ListElement* elem = (ListElement*)atom;

   switch(elem->type) 
   {
     case VARIABLE:

           if(elem->value.name)
             free(elem->value.name);

           break;


     case INTEGER_CONSTANT:

           break;


     case CHARACTER_CONSTANT:
       
     case STRING_CONSTANT:

           if(elem->value.string)
             free(elem->value.string);

           break;


     case INDEGREE:

     case OUTDEGREE:

           if(elem->value.node_id) 
             free(elem->value.node_id);

           break;


     case LIST_LENGTH:

           if(elem->value.list_arg)
             g_list_free_full(elem->value.list_arg, freeListElement);
             
           break;


     case STRING_LENGTH:

           if(elem->value.str_arg)
             freeListElement(elem->value.str_arg);

           break;

     case NEG:

           if(elem->value.exp) freeListElement(elem->value.exp);

           break;


     case ADD:

     case SUBTRACT:

     case MULTIPLY:

     case DIVIDE:

     case CONCAT:

           if(elem->value.bin_op.left_exp)
             freeListElement(elem->value.bin_op.left_exp);
           if(elem->value.bin_op.right_exp)  
             freeListElement(elem->value.bin_op.right_exp);

           break;


     default: printf("Unexpected List Element Type: %d\n",
                     (int)elem->type); 
               break;

   }
   free(elem);
}

/* void freeGraphChange(GraphChange *change)
{
   switch(change->type)
   {
      case ADD_NODE:

           if(change->data.added_node) free(change->data.added_node);
           if(change) free(change);

           break;


      case ADD_EDGE:

           if(change->data.added_edge) free(change->data.added_edge);
           if(change) free(change);

           break;


      case REMOVE_NODE:

           if(change->data.removed_node.name) 
              free(change->data.removed_node.name);
           if(change->data.removed_node.label)
             freeLabel(change->data.removed_node.label);
           if(change) free(change);

           break;


      case REMOVE_EDGE:

           if(change->data.removed_edge.name) 
              free(change->data.removed_edge.name);
           if(change->data.removed_edge.label)
             freeLabel(change->data.removed_edge.label);
           if(change->data.removed_edge.source_name) 
              free(change->data.removed_edge.source_name);
           if(change->data.removed_edge.target_name) 
              free(change->data.removed_edge.target_name);
           if(change) free(change);

           break;


      case RELABEL_NODE:

           if(change->data.relabelled_node.name) 
              free(change->data.relabelled_node.name);
           if(change->data.relabelled_node.old_label) 
              free(change->data.relabelled_node.old_label);
           if(change) free(change);           

           break;


      case RELABEL_EDGE:

           if(change->data.relabelled_edge.name) 
              free(change->data.relabelled_edge.name);
           if(change->data.relabelled_edge.old_label) 
              free(change->data.relabelled_edge.old_label);
           if(change) free(change);           

           break;


      default: 
           print_to_log("Error (freeGraphChange): Unexepected change type "
                        "%d.\n",change->type); 
           break;      
   }  
} 

void restoreGraph (Graph *graph)
{
   while(graph->graph_change_stack->top != 0)
   {
      GraphChange *change = (GraphChange*) pop(graph->graph_change_stack);

      switch(change->type)
      {
         case ADD_NODE:
         {
              string name = change->data.added_node;
              Node *node = g_hash_table_lookup(graph->nodes, name);

              removeNode(graph, node, false);

              if(name) free(name);
              if(change) free(change);

              break;
         }

         case ADD_EDGE:
         {
              string name = change->data.added_edge;
              Edge *edge = g_hash_table_lookup(graph->edges, name);

              removeEdge(graph, edge, false);

              if(name) free(name);
              if(change) free(change);

              break;
         }

         case REMOVE_NODE:
         {
              string name = change->data.removed_node.name;

              Node *node = newNode(change->data.removed_node.name,
                                   change->data.removed_node.root, 
                                   change->data.removed_node.label);
              addNode(graph, node, false);

              if(name) free(name);
              if(change) free(change);

              break;
         }

         case REMOVE_EDGE:
         {
              string name = change->data.removed_edge.name;
              string source_name = change->data.removed_edge.source_name;
              string target_name = change->data.removed_edge.target_name;

              Node *source = g_hash_table_lookup(graph->nodes, source_name);
              Node *target = g_hash_table_lookup(graph->nodes, target_name);
              Edge *edge = newEdge(name, change->data.removed_edge.bidirectional,
                                   change->data.removed_edge.label, source,
                                   target);
              addEdge(graph, edge, false);

              if(name) free(name);
              if(source_name) free(source_name);
              if(target_name) free(target_name);
              if(change) free(change);           

              break; 
         }

         case RELABEL_NODE:
         {
              string name = change->data.relabelled_node.name;

              Node *node = g_hash_table_lookup(graph->nodes, name);
              relabelNode(graph, node, change->data.relabelled_node.old_label, 
                          change->data.relabelled_node.change_root, false);
              
              if(name) free(name);
              if(change) free(change);           

              break;
         }

         case RELABEL_EDGE:
         {
              string name = change->data.relabelled_edge.name;

              Edge *edge = g_hash_table_lookup(graph->edges, name);
              relabelEdge(graph, edge, change->data.relabelled_edge.old_label,
                          change->data.relabelled_edge.change_bidirectional, 
                          false);

              if(name) free(name);
              if(change) free(change);    

              break;
         }
         default: 
              print_to_log("Error (restoreGraph): Unexepected change type %d.\n",
                           change->type); 
              break;
      }
   } 
} */

