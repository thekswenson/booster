/*
Here we implement the faster, tree traversal based, Transfer Index computation.
*/
#include "rapid_transfer.h"
#include "heavy_paths.h"


/*
Compute the Transfer Index (TI) for all edges, comparing a reference tree to
an alternative (bootstrap) tree.

This is the faster version that is based on assigning an index by traversing
the ref_tree.
We compute the rooted TI using the rooted Transfer Distance (TD). The rooted
TD between node u in ref_tree and node v in alt_tree is the size of symmetric
difference L(u)-L(v) U L(v)-L(u) (where L(u) is the leaf set in the subtree
rooted at u).

We start by computing the TI for a leaf u. Then the TI is computed for parent
u', as long as u is the "heavy" child of u'. This is repeated, starting at
each leaf.

At the end, the transfer index for edge i will be in transfer_index[i].

If getsets is true, then compute the transfer sets for each edge of ref_tree,
not just the index (i.e. the size of the sets).
*/
void compute_transfer_indices_fast(Tree *ref_tree, const int n,
                                   const int m, Tree *alt_tree,
                                   int *transfer_indices,
                                   bool getsets)
{
  set_leaf_bijection(ref_tree, alt_tree);  //Map leaves between the two trees

  DB_CALL(0, print_nodes_post_order(ref_tree));
  DB_TRACE(0, "alt_tree:\n");
  DB_CALL(0, print_nodes_post_order(alt_tree));

  Path* heavypath_root = do_heavy_decomposition(alt_tree->node0, getsets);
  DB_CALL(0, verify_all_leaves_touched(alt_tree));

  Node** ref_leaves = ref_tree->leaves->a; //Leaves in ref_tree

    //Compute the TI for each node, following paths from leaves in ref_tree up
    //to the root, calling add_leaf on leaves from pendant subtrees. At the end
    //of each loop, the TI will be the value of d_min at the root of alt_tree.
  Node* u;                                 //Node to follow to root of ref_tree
  for(int i=0; i < ref_tree->nb_taxa; i++)
  {
    u = ref_leaves[i];                     //Start with a leaf

    DB_TRACE(0, "------------------ new heavy path in ref -----------------\n");
    DB_CALL(0, fprintf(stderr, "ref_tree start "); print_node(u));
    DB_CALL(0, fprintf(stderr, "alt_tree ");
               print_nodes_TIvars(alt_tree->a_nodes, alt_tree->nb_nodes));

    //print_HPT_dot(heavypath_root, alt_tree->node0, i);
                                        //Get TI on ref heavypath starting at u
    add_heavy_path(u, alt_tree, true, getsets);
    //print_HPT_dot(heavypath_root, alt_tree->node0, 100+i);
    reset_heavy_path(u, true, getsets); //Reset TI variables on the HPT
  }

  free_HPT(heavypath_root);

  nodeTI_to_edgeTI(ref_tree);                  //Move node values to the edges
  edgeTI_to_array(ref_tree, transfer_indices); //Copy edge values into the array
}


/*
Compute the Transfer Index (TI) for all edges, comparing a reference tree to
an alternative _balanced_ (bootstrap) tree.  This does not do a heavypath
decomposition of the alternative tree.

This is the faster version that is based on assigning an index by traversing
the ref_tree.
We compute the rooted TI using the rooted Transfer Distance (TD). The rooted
TD between node u in ref_tree and node v in alt_tree is the size of symmetric
difference L(u)-L(v) U L(v)-L(u) (where L(u) is the leaf set in the subtree
rooted at u).

We start by computing the TI for a leaf u. Then the TI is computed for parent
u', as long as u is the "heavy" child of u'. This is repeated, starting at
each leaf.

At the end, transfer_index[i] will have the transfer index for edge i.
*/
void compute_transfer_indices_fast_BALANCED(Tree *ref_tree, const int n,
                                            const int m, Tree *alt_tree,
                                            int *transfer_indices)
{
  set_leaf_bijection(ref_tree, alt_tree);  //Map leaves between the two trees

  DB_CALL(0, print_nodes_post_order(ref_tree));
  DB_TRACE(0, "alt_tree:\n");
  DB_CALL(0, print_nodes_post_order(alt_tree));

  Node** ref_leaves = ref_tree->leaves->a; //Leaves in ref_tree

    //Compute the TI for each node, following paths from leaves in ref_tree up
    //to the root, calling add_leaf on leaves from pendant subtrees. At the end
    //of each loop, the TI will be the value of d_min at the root of alt_tree.
  Node* u;                                 //Node to follow to root of ref_tree
  for(int i=0; i < ref_tree->nb_taxa; i++)
  {
    u = ref_leaves[i];                     //Start with a leaf

    DB_TRACE(0, "------------------ new heavy path ------------------------\n");
    DB_CALL(0, fprintf(stderr, "ref_tree "); print_node(u));
    DB_CALL(0, fprintf(stderr, "alt_tree ");
            print_nodes_TIvars(alt_tree->a_nodes, alt_tree->nb_nodes));

    //print_tree_dot(alt_tree, "alt_tree_0.dot", false);
    add_heavy_path(u, alt_tree, false, false); //Get TI on heavy path starting at u
    //print_tree_dot(alt_tree, "alt_tree_1.dot", false);
    reset_heavy_path(u, false, false);         //Reset TI associated variables
    //print_tree_dot(alt_tree, "alt_tree_2.dot", false);
  }

  nodeTI_to_edgeTI(ref_tree);                  //Move node values to the edges
  edgeTI_to_array(ref_tree, transfer_indices); //Copy edge values into the array
}


/*
Copy the edge Transfer Index values into the given array.
*/
void edgeTI_to_array(Tree *tree, int *transfer_indices)
{
  for(int i=0; i < tree->nb_edges; i++)
    transfer_indices[i] = tree->a_edges[i]->transfer_index;
}

/*
Follow a leaf u in ref_tree up to the root along its heavy path in ref_tree.
Call add_leaf on the leaves in the subtrees off the path.

If use_HPT is false, then assume a balanced alt_tree, otherwise use a heavypath
tree on the alt_tree.

If getsets is true, then maintain sets to be transferred.
*/
void add_heavy_path(Node *u, Tree *alt_tree, bool use_HPT, bool getsets)
{
  Path* hpt_root;
  if(use_HPT)
    hpt_root = get_HPT_root(u->other);

  while(u)                               //Have not visited the root and have
  {                                      //not seen a heavier sibling
    DB_TRACE(0, "________\n");
    DB_TRACE(0, "++++++++ children of "); DB_CALL(0, print_node(u));

      //Add the leaves from the light subtree:
    if(u->nneigh == 1)                          //u is a leaf in ref_tree, so
    {                                           //can't be heavier than sibling
      DB_TRACE(0, "leaf - "); DB_CALL(0, print_node(u));

      if(use_HPT)
        add_leaf_HPT(u->other, getsets);
      else
        add_leaf(u->other, getsets);                     //add_leaf on v (in alt_tree)
    }
    else
    {
      DB_TRACE(0, "subtree - "); DB_CALL(0, printNA(u->lightleaves));
      for(int i=0; i < u->lightleaves->i; i++)  //a subtree
      {
        if(use_HPT)                             //add_leaf on leaves in subtree
          add_leaf_HPT(u->lightleaves->a[i]->other, getsets);
        else
          add_leaf(u->lightleaves->a[i]->other, getsets);
      }
    }

      //Record the transfer index:
    if(use_HPT)
    {
      u->ti_min = get_ti_min(hpt_root);
      u->ti_max = get_ti_max(hpt_root);

      //print_HPT_dot(hpt_root, alt_tree->node0, 0);
      //fprintf(stderr, "(%i, %i) ref_tree ", u->ti_min, u->ti_max);
      //print_node(u);

      if(getsets)
      {
        NodeArray *tset = get_transfer_set_HPT(hpt_root, alt_tree);
        assert(tset->i == min(u->ti_min, alt_tree->nb_taxa - u->ti_max));
        freeNA(tset);
      }
    }
    else
    {
      u->ti_min = alt_tree->node0->d_min;
      u->ti_max = alt_tree->node0->d_max;

      u->min_node = get_min_node(alt_tree);
      u->max_node = get_max_node(alt_tree);

      //fprintf(stderr, "(%i, %i) ref_tree ", u->ti_min, u->ti_max);
      //print_node(u);
      //fprintf(stderr, "       alt_tree ");
      //print_node(u->min_node);
      //fprintf(stderr, "                ");
      //print_node(u->max_node);
      //fprintf(stderr, "distance: %i,%i - %i,%i\n", alt_tree->node0->d_min, alt_tree->node0->d_max, transfer_distance(alt_tree, u->min_node), transfer_distance(alt_tree, u->max_node));
      //fprintf(stderr, "index:    %i\n", transfer_index(alt_tree));
      //fprintf(stderr, "          %i\n", transfer_distance(alt_tree, u->min_node));
      //fprintf(stderr, "          %i\n", transfer_distance(alt_tree, u->max_node));
      //fprintf(stderr, "ismin?    %i\n", transfer_index_is_min(alt_tree));
      assert(transfer_index(alt_tree) ==
             (transfer_index_is_min(alt_tree) ? transfer_distance(alt_tree, u->min_node) : transfer_distance(alt_tree, u->max_node)));

      NodeArray *tset = get_transfer_set(alt_tree);
      //fprintf(stderr, "TRANSFER SET: ");
      //printNA(tset);

      NodeArray *min_tset = get_transfer_set_for_node(alt_tree, u->min_node, false);
      NodeArray *max_tset = get_transfer_set_for_node(alt_tree, u->max_node, true);
      assert(tset->i == (transfer_index_is_min(alt_tree) ? min_tset->i : max_tset->i));
      freeNA(tset);
      freeNA(min_tset);
      freeNA(max_tset);
    }
    DB_CALL(0, fprintf(stderr, "++++++++ TI: %i %i\n", u->ti_min, u->ti_max));

      //Head upwards in ref_tree:
    if(u->depth != 0 && u == u->neigh[0]->heavychild)
      u = u->neigh[0];       //u in on the heavy side of its parent.
    else
      u = NULL;              //u is root or light child
  }
  DB_TRACE(0, ".....done.....\n");
}

/*
Follow a leaf in ref_tree up to the root. Call reset_leaf on the leaves in
the subtrees off the path.

If use_HPT is false, then assume balanced alt_tree, otherwise reset values on
the HeavyPath Tree (HPT).

If getsets is true, then maintain sets to be transferred.
*/
void reset_heavy_path(Node* u, bool use_HPT, bool getsets)
{
  while(u)                                  //Have not visited the root and have
  {                                         //not seen a heavier sibling
      //Add the leaves from the light subtree:
    if(u->nneigh == 1)                      //a leaf
    {
      if(use_HPT)
        reset_leaf_HPT(u->other, getsets);  //call add_leaf on the Path for v
      else
        reset_leaf(u->other, getsets);      //call add_leaf on v
    }
    else
    {
      for(int i=0; i < u->lightleaves->i; i++)
        if(use_HPT)
          reset_leaf_HPT(u->lightleaves->a[i]->other, getsets);
        else
          reset_leaf(u->lightleaves->a[i]->other, getsets);
    }

      //Head upwards:
    if(u->depth == 0 || u != u->neigh[0]->heavychild)
      u = NULL;              //u is root or light child
    else
      u = u->neigh[0];       //u is on the heavy side of its parent.
  }
}


/*
Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.

If getset is true, then do the bookkeeping for the transfer set.
*/
void add_leaf(Node *leaf, bool getset)
{
  assert_is_leaf(leaf);

    //Follow the path from the root to the leaf, updating the d_lazy values
    //with the diff values, subtracting 1, but adding 1 to the diff values of
    //nodes off the path:
  Node **path = path_to_root(leaf);
  for(int i = leaf->depth; i > 0; i--)              //For all non-leaf Nodes
  {
    DB_TRACE(0, "current: "); DB_CALL(0, print_node(path[i]));
    DB_TRACE(0, "         "); DB_CALL(0, print_node_TIvars(path[i]));
    path[i]->d_lazy += path[i]->diff - 1;
    if(getset)
      addNodeNA(path[i]->exclude, leaf);
    path[i-1]->diff += path[i]->diff;               //Push difference down

    int startindex = 1;                             //Not the root
    if(path[i]->depth == 0)
      startindex = 0;                               //The root

    for(int j = startindex; j < path[i]->nneigh; j++)
      if(path[i]->neigh[j] != path[i-1])            //A node off the path
      {
        path[i]->neigh[j]->diff += path[i]->diff+1;
        if(getset)
          addNodeNA(path[i]->neigh[j]->include, leaf);
      }

    path[i]->diff = 0;
    DB_TRACE(0, "         "); DB_CALL(0, print_node_TIvars(path[i]));
  }
  DB_TRACE(0, "leaf   : "); DB_CALL(0, print_node(path[0]));
  DB_TRACE(0, "         "); DB_CALL(0, print_node_TIvars(path[0]));
  leaf->d_lazy += leaf->diff - 1;
  leaf->diff = 0;
  if(getset)
    addNodeNA(leaf->exclude, leaf);
  DB_TRACE(0, "         "); DB_CALL(0, print_node_TIvars(path[0]));

    //Follow the path back up to the root, updating the d_min and d_max values
    //for each pair of siblings:
  update_dminmax_on_path(path, leaf->depth+1);
  free(path);
}

/*
Reset the d_min, d_max, d_lazy, and diff values for the path from the given
leaf (from alt_tree) to the root.
*/
void reset_leaf(Node *leaf, bool getset)
{
  assert_is_leaf(leaf);

    //Follow the path from the root to the leaf, resetting the values along
    //the way:
  Node *n = leaf;
  Node *pathchild = NULL;
  while(1)                     //While not the root
  {
    n->d_lazy = n->subtreesize;
    n->d_max = n->subtreesize;
    n->d_min = 1;
    n->diff = 0;
    if(getset)
      clearNA(n->exclude);
    if(n->nneigh != 1)         //not the leaf
    {
      for(int i=1; i < n->nneigh; i++)
        if(n->neigh[i] != pathchild)    //reset other children
        {
          n->neigh[i]->diff = 0;
          if(getset)
            clearNA(n->neigh[i]->include);
        }

      if(n->depth == 0)        //the root
      {
        n->neigh[0]->diff = 0; //reset the first child
        if(getset)
          clearNA(n->neigh[0]->include);
        return;
      }
    }
    n = n->neigh[0];
    pathchild = n;
  }
}

/*
Follow the nodes on the path from a leaf to the root, updating d_min and
d_max on the way up.
*/
void update_dminmax_on_path(Node** path, int pathlength)
{
  path[0]->d_min = path[0]->d_lazy;  //The leaf values
  path[0]->d_max = path[0]->d_lazy;
  for(int i = 1; i < pathlength; i++)
  {
    path[i]->d_min = path[i]->d_lazy;
    path[i]->d_max = path[i]->d_lazy;

      //Check values of the children:
    int startind = 1;
    if(path[i]->depth == 0)          //The root
      startind = 0;                  //Not the root
    for(int j = startind; j < path[i]->nneigh; j++)
    {
      path[i]->d_min = min(path[i]->d_min,
                           path[i]->neigh[j]->d_min + path[i]->neigh[j]->diff);
      path[i]->d_max = max(path[i]->d_max,
                           path[i]->neigh[j]->d_max + path[i]->neigh[j]->diff);
    }

    DB_TRACE(0, "up: dmin %d ", path[i]->d_min);DB_CALL(0, print_node(path[i]));
    DB_TRACE(0, "up: dmax %d\n", path[i]->d_max);
  }
}

/*
Die if the given node is not a leaf.
*/
void assert_is_leaf(Node* leaf)
{
  if(leaf->nneigh != 1)   //not a leaf
  {
	  fprintf(stderr, "Error: leaf not given to add_leaf().\n");
	  Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }
}


/*
Return a path (array of Node*) from this node to the root.

@warning  user responsible for the memory.
*/
Node** path_to_root(Node *n)
{
  int pathlen = n->depth;
  Node **path = calloc(n->depth+1, sizeof(Node*));
  for(int i=0; i <= pathlen; i++)
  {
    path[i] = n;
    n = n->neigh[0];
  }

  return path;
}

/*
Return an array mapping the index of a leaf Node in leaves1, to a pointer to a
leaf Node from leaves2.

** modifies (sorts) leaves1 and leaves2
*/
Node** leaf_to_leaf(Node **leaves1, Node **leaves2, int n)
{
    //Sort the leaves by their string name:
  qsort(leaves1, n, sizeof(Node*), compare_nodes);
  qsort(leaves2, n, sizeof(Node*), compare_nodes);

    //Compute the mapping:
  Node **indexTOleaf2 = calloc(n, sizeof(Node*));
  for(int i=0; i < n; i++)
    indexTOleaf2[i] = leaves2[i];

  return indexTOleaf2;
}

/*
Compute the edge Transfer Index from the child node transfer index.
*/
void nodeTI_to_edgeTI(Tree *tree)
{
  for(int i=0; i < tree->nb_nodes; i++)
    set_edge_TI(tree->a_nodes[i], tree->nb_taxa);
}

/*
Set the Transfer Index of the edge above this node (in ref_tree).

@warning  assume that ti_min and ti_max are already set.
*/
void set_edge_TI(Node *u, int n)
{
  if(u->depth != 0)              //Not the root
  {
    DB_TRACE(0, "min %i max %i ", u->ti_min, n - u->ti_max);
    DB_CALL(0, print_node(u));
    u->br[0]->transfer_index = min(u->ti_min, n - u->ti_max);
  }
}
