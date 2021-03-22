/*
Here we implement the heavy path decomposition of the alt_tree.
*/
#include "heavy_paths.h"

/*
Allocate a new Path, setting all the default values.
*/
int id_counter = 0;          //Global id counter for Path structs.
Path* new_Path()
{
  Path *newpath = malloc(sizeof(Path));

  newpath->id = id_counter++;

  newpath->left = NULL;
  newpath->right = NULL;
  newpath->parent = NULL;
  newpath->sibling = NULL;

  newpath->node = NULL;

  newpath->child_heavypaths = NULL;
  newpath->num_child_paths = 0;
  newpath->parent_heavypath = NULL;
  newpath->path_to_root_p = NULL;

  newpath->total_depth = 0;

  newpath->diff_path = newpath->diff_subtree = 0;

  newpath->d_min_path = 1;
  newpath->d_min_subtree = 1;

  newpath->d_max_path = 0;
  newpath->d_max_subtree = 1;

  newpath->include_subtree = allocateNA(INCLUDE_EXCLUDE_SIZE);
  newpath->include_path = allocateNA(INCLUDE_EXCLUDE_SIZE);
  newpath->exclude = allocateNA(INCLUDE_EXCLUDE_SIZE);
  newpath->exclude_path = allocateNA(INCLUDE_EXCLUDE_SIZE);

  return newpath;
}


/*
Recursively decompose the alternative tree into heavy paths according to
the scheme described in the definition of the Path struct. Return the root
Path of the Path tree.

@note   Each heavypath corresponds to a tree of Paths we call the PathTree (PT).
        Leaves of the PTs are glued to the roots of other PTs (using the
        child_heavypath pointers).
        We call the entire tree the HeavyPathTree (HPT)

@note   Allocate a pointer to an array that will hold a path to the root for
        a leaf in the HPT, for use when calculating add_leaf_HPT(). Each leaf
        of the HPT will have path_to_root_p set to this value, as well as
        the root of the HPT.
*/
Path* do_heavy_decomposition(Node *root)
{
  int maxdepth = 0;
  Path*** path_to_root_pointer = malloc(sizeof(Path**));
  Path* heavy_path_tree_root = heavy_decomposition(root, 0, &maxdepth,
                                                   path_to_root_pointer);

  *path_to_root_pointer = calloc(maxdepth+1, sizeof(Path*));
  heavy_path_tree_root->path_to_root_p = path_to_root_pointer;
  //set_paths_to_root(heavy_path_tree_root);

  return heavy_path_tree_root;
}

Path* heavy_decomposition(Node *root, int depth, int *maxdepth,
                          Path*** path_to_root_pointer)
{
  int length;
  Node** heavypath = get_heavypath(root, &length);

  Path *path_root;
  if(length == 1)
    path_root = heavypath_leaf(heavypath[0], depth, maxdepth,
                               path_to_root_pointer);
  else
    path_root = partition_heavypath(heavypath, length, depth, maxdepth,
                                    path_to_root_pointer);

  free(heavypath);

  return path_root;
}

/*
Free the memory for the HeavyPathTree (allocated in heavy_decomposition).
*/
void free_HPT(Path* root)
{
  free(*root->path_to_root_p);
  free(root->path_to_root_p);
  free_HPT_rec(root);
}
void free_HPT_rec(Path* node)
{
  freeNA(node->include_path);
  freeNA(node->include_subtree);
  freeNA(node->exclude);
  freeNA(node->exclude_path);

  if(node->child_heavypaths)                    //PT leaf with descendents
  {
    for(int i=0; i < node->num_child_paths; i++)
      free_HPT_rec(node->child_heavypaths[i]);  //decend to next PT

    free(node->child_heavypaths);
  }

  else if(!node->node)                      //not leaf of HPT (internal PT node)
  {
    free_HPT_rec(node->left);
    free_HPT_rec(node->right);
  }

  //else                                    //a leaf of the HPT
  //{
  //  free(node->path_to_root);
  //}

  free(node);
}


/*
For the given heavypath, create a Path structure that represents the path.
Split the path in half and create a Path for each half.  If a half is a single
node, then hang the next heavy path off of it. If it's a leaf of alt_tree, then
link the Path to the corresponding leaf in alt_tree.
*/
Path* partition_heavypath(Node **heavypath, int length, int depth,
                          int *maxdepth, Path ***path_to_root_pointer)
{
  Path* newpath = new_Path();
  newpath->total_depth = depth;

    //Split the heavy path into two equal-length subpaths:
  int l1 = ceil(length/2);
  if(l1 == 1)
    newpath->left = heavypath_leaf(heavypath[0], depth+1, maxdepth,
                                   path_to_root_pointer);
  else
    newpath->left = partition_heavypath(heavypath, l1, depth+1, maxdepth,
                                        path_to_root_pointer);
  newpath->left->parent = newpath;

  int l2 = length - l1;
  if(l2 == 1)
    newpath->right = heavypath_leaf(heavypath[l1], depth+1, maxdepth,
                                    path_to_root_pointer);
  else
    newpath->right = partition_heavypath(&heavypath[l1], l2, depth+1, maxdepth,
                                         path_to_root_pointer);
  newpath->right->parent = newpath;

  newpath->right->sibling = newpath->left;
  newpath->left->sibling = newpath->right;

  newpath->d_min_path = min(newpath->left->d_min_path,
                            newpath->right->d_min_path);
  newpath->d_max_path = max(newpath->left->d_max_path,
                            newpath->right->d_max_path);
  newpath->d_max_subtree = max(newpath->left->d_max_subtree,
                               newpath->right->d_max_subtree);

  newpath->num_hpt_leaves = newpath->left->num_hpt_leaves +
                            newpath->right->num_hpt_leaves;

  return newpath;
}

/*
Return a Path for the given node of alt_tree.  The Path will be a leaf
node of the path tree.
Either 1) the leaf will point to a node (internal or leaf) of alt_tree,
or     2) child_heavypath will point to a heavypath representing the
          descendant of the alt_tree node.
*/
Path* heavypath_leaf(Node *node, int depth, int *maxdepth,
                     Path ***path_to_root_pointer)
{
  Path* newpath = new_Path();

  newpath->total_depth = depth;
  newpath->node = node;           //attach the path to the node
  node->path = newpath;           //attach the node to the path

  newpath->d_max_path = node->subtreesize;

    //Handle an internal alt_tree node with pendant heavypath:
  if(node->nneigh > 1)
  {
    newpath->num_child_paths = node->nneigh - 2;
    int i_neigh = 1;              //don't look at the parent
    if(node->depth == 0)          //If we are the root of alt_tree
    {
      i_neigh = 0;                //then we have no parent.
      newpath->num_child_paths += 1;
    }

    newpath->child_heavypaths = calloc(newpath->num_child_paths, sizeof(Path*));
    newpath->d_min_subtree = INT_MAX;
    newpath->d_max_subtree = INT_MIN;
    newpath->num_hpt_leaves = 0;

    int j = 0;                    //index the child heavypaths
    while(i_neigh < node->nneigh)
    {
      if(node->neigh[i_neigh] != node->heavychild)
      {
        newpath->child_heavypaths[j] = heavy_decomposition(node->neigh[i_neigh],
                                                           depth+1, maxdepth,
                                                           path_to_root_pointer);
        newpath->child_heavypaths[j]->parent_heavypath = newpath;

        newpath->num_hpt_leaves += newpath->child_heavypaths[j]->num_hpt_leaves;

        newpath->d_min_subtree = min3(newpath->d_min_subtree,
                                      newpath->child_heavypaths[j]->d_min_path,
                                      newpath->child_heavypaths[j]->d_min_subtree);
        newpath->d_max_subtree = max3(newpath->d_max_subtree,
                                      newpath->child_heavypaths[j]->d_max_path,
                                      newpath->child_heavypaths[j]->d_max_subtree);
        j++;
      }

      i_neigh++;
    }

    newpath->d_min_path = newpath->d_max_path = node->subtreesize;
  }
  else                            //HPT leaf (corresponds to alt_tree leaf)
  {
    *maxdepth = max(depth, *maxdepth);
    newpath->path_to_root_p = path_to_root_pointer;
    newpath->num_hpt_leaves = 1;
  }

  return newpath;
}


/*
Descend to the leaves of the HPT. Once there, create the path_to_root vector
for that Path object.
*/
//void set_paths_to_root(Path* node)
//{
//  if(node->child_heavypaths)
//    for(int i=0; i < node->num_child_paths; i++)
//      set_paths_to_root(node->child_heavypaths[i]);
//
//  else if(node->left)
//  {
//    set_paths_to_root(node->left);
//    set_paths_to_root(node->right);
//  }
//
//  else                                //a leaf of the HPT tree
//    node->path_to_root = get_path_to_root_HPT(node);
//}

/* Return the min transfer index in the subtree rooted at path.
*/
int get_ti_min(Path* path)
{
  return min(path->d_min_path + path->diff_path,
             path->d_min_subtree + path->diff_subtree);
}

/* Return the transfer index in the subtree rooted at path, given the
modifying values. Return the max if usemax is true, other return the min.
*/
int get_ti_x_mod(Path* path, int accum_path, int accum_subtree, bool usemax)
{
  if(usemax)
    return get_ti_max_mod(path, accum_path, accum_subtree);
  else
    return get_ti_min_mod(path, accum_path, accum_subtree);
}

/* Return the min transfer index in the subtree rooted at path, given the
modifying values.
*/
int get_ti_min_mod(Path* path, int accum_path, int accum_subtree)
{
  if(path->node && path->num_child_paths == 0)    //leaf of HPT
    return path->d_min_path + path->diff_path + accum_path;
  else
    return min(path->d_min_path + path->diff_path + accum_path,
               path->d_min_subtree + path->diff_subtree + accum_subtree);
}

/* Return the max transfer index in the subtree rooted at path.
*/
int get_ti_max(Path* path)
{
  if(path->node && path->num_child_paths == 0)    //leaf of HPT
    return path->d_max_path + path->diff_path;
  else
    return max(path->d_max_path + path->diff_path,
               path->d_max_subtree + path->diff_subtree);
}

/* Return the max transfer index in the subtree rooted at path, given the
modifying values.
*/
int get_ti_max_mod(Path* path, int accum_path, int accum_subtree)
{
  return max(path->d_max_path + path->diff_path + accum_path,
             path->d_max_subtree + path->diff_subtree + accum_subtree);
}

/* Return the transfer set for the best node in the Heavy Path Tree to which
the given root Path belongs.

@note  user responsible for the memory

Descend from the root of the HPT as long as the best value in the child's
subtree is equal to the best of the whole tree. On the way down, add the
included leaves. Once to the bottom, mark the excluded leaves for that node.
Visit all of the leaves in the subtree keeping only those that are not marked
to be excluded.
*/
NodeArray* get_transfer_set_HPT(Path* hptroot, Tree* alt_tree)
{
  //print_HPT_sets(hptroot);
  int min = get_ti_min(hptroot);
  int max = alt_tree->nb_taxa - get_ti_max(hptroot);
  NodeArray *set;
  if(min <= max)
  {
    Path* min_node = get_min_path(hptroot);
    //fprintf(stderr, "  min node HPT: %i\n", min_node->id);
    //fprintf(stderr, "  min node: "); print_node(min_node->node);
    set = get_transfer_set_for_node_HPT(min_node, alt_tree, false);
    //fprintf(stderr, "TRANSFER SET min (%i): ", set->i);
    //printNA(set);
    assert(set->i == min);
  }
  else
  {
    Path* max_node = get_max_path(hptroot);
    //fprintf(stderr, "  max node HPT: %i\n", max_node->id);
    //fprintf(stderr, "  max node: "); print_node(max_node->node);
    set = get_transfer_set_for_node_HPT(max_node, alt_tree, true);
    //fprintf(stderr, "TRANSFER SET max (%i == %i): ", set->i, max);
    //printNA(set);
    assert(set->i == max);
  }

  return set;
}


/* Return the Path corresponding to the node in alt_tree with the minimum
transfer distance.
*/
Path* get_min_path(Path* hptroot)
{
  return get_x_path(hptroot, false);
}

/* Return the Path corresponding to the node in alt_tree with the maximum
transfer distance.
*/
Path* get_max_path(Path* hptroot)
{
  return get_x_path(hptroot, true);
}

/* Return the Path corresponding to the node in alt_tree with the minimum
transfer distance if usemax is true, otherwise return the min.
*/
Path* get_x_path(Path* hptroot, bool usemax)
{
  int target;
  if(usemax)
    target = get_ti_max(hptroot);
  else
    target = get_ti_min(hptroot);

  Path* currentnode = hptroot;
  int accum_path = hptroot->diff_path;
  int accum_subtree = hptroot->diff_subtree;
  while(true)
  {
    if(currentnode->right &&
       get_ti_x_mod(currentnode->right, accum_path, accum_subtree, usemax) == target)
    {
      currentnode = currentnode->right;
      accum_path += currentnode->diff_path;
      accum_subtree += currentnode->diff_subtree;
    }
    else if(currentnode->left &&
            get_ti_x_mod(currentnode->left, accum_path, accum_subtree, usemax) == target)
    {
      currentnode = currentnode->left;
      accum_path += currentnode->diff_path;
      accum_subtree += currentnode->diff_subtree;
    }
    else if(currentnode->child_heavypaths) //PT leaf (internal node of alt_tree)
    {
      bool notfound = true;
      for(int i=0; i < currentnode->num_child_paths; i++)
      {
        if(get_ti_x_mod(currentnode->child_heavypaths[i],
                        accum_subtree, accum_subtree, usemax) == target)
        {
          currentnode = currentnode->child_heavypaths[i];
          notfound = false;
          accum_path = currentnode->diff_path + accum_subtree;
          accum_subtree = currentnode->diff_subtree + accum_subtree;
        }

      }

      if(notfound)
        return currentnode;
    }
    else
      return currentnode;                  //HPT leaf (leaf of alt_tree)
  }
}

/* Return the transfer set for the given node. If usemax is true, then
get the leaf set corresponding to the max value.

@note  you must free any memory associated to t->transfer_set before calling
       this function
*/
NodeArray* get_transfer_set_for_node_HPT(Path* n, Tree* alt_tree, bool usemax)
{
  if(usemax)
    return get_max_transfer_set_for_node_HPT(n, alt_tree);
  else
    return get_min_transfer_set_for_node_HPT(n, alt_tree);
}

/* Return the min transfer set for the given node.
*/
NodeArray* get_min_transfer_set_for_node_HPT(Path* n, Tree* alt_tree)
{
  NodeArray* transfer_set = allocateNA(INCLUDE_EXCLUDE_SIZE);

  Path** path = get_path_to_root_HPT(n);
    //Ascent to root while collecting include_subree leaves from ancestral
    //PTs, and the include_path and exclude_path leaves from this PT:
  bool in_PT = true;    //True until we pass from this PT to the parent PT
  for(int i=0; i <= n->total_depth; i++)
  {
    if(i && path[i]->num_child_paths)
      in_PT = false;
    if(in_PT)
      appendNA(transfer_set, path[i]->include_path);
    else
      appendNA(transfer_set, path[i]->include_subtree);
  }
  free(path);

  add_nonexcluded_from_HPT_subtree(transfer_set, n);
    //Find the sibling node that roots the subtree for the HP to the right:
  while(n->sibling)           //not root of PT
  {
    if(n == n->parent->left)  //we are the left child
      add_nonexcluded_from_HPT_subtree(transfer_set, n->sibling);
    n = n->parent;
  }

  return transfer_set;
}


/* Return the max transfer set for the given node.

For the maximum case we have to:
- include all exclude nodes for this path,
- include all exclude_path lists from ancestors in this PT,
- inside this TP we
  - exclude all include_subtree lists from siblings to the left, ignoring
    those to the right.
- outside this TP we
  - exclude all include_subtree lists from siblings of all ancestors
    (at a TP leaf, we exclude all include_subtrees from sibling TPs)

All leaves from sibling subtrees to the ancestors are then added to the TS,
unless they have been excluded.
*/
NodeArray* get_max_transfer_set_for_node_HPT(Path* n, Tree* alt_tree)
{
  NodeArray* transfer_set = allocateNA(INCLUDE_EXCLUDE_SIZE);

  Path** path = get_path_to_root_HPT(n);
    //Ascent to root while collecting include_subree leaves from ancestral
    //PTs, and the include_path and exclude_path leaves from this PT:
  bool in_PT = true;   //True until we pass from this PT to the parent PT
  for(int i=0; i <= n->total_depth; i++)
  {
    if(i && path[i]->num_child_paths)
      in_PT = false;

    if(in_PT)          //in this PT
      appendNA(transfer_set, path[i]->exclude_path);
  }

    //Inclue the exclude list from this node:
  appendNA(transfer_set, n->exclude);

    //Traverse the alt_tree subtree, including only the alt_tree leaves that
    //have exclude_this == false:
  include_leaves_from_ancestral_subtrees(transfer_set, n);

  free(path);
  return transfer_set;
}


/* Traverse the tree only to the HPT leaves that are not excluded. Add these
leaves to the transfer_set. This is done by comparing the excluded array
of a node to the number of HPT leaves descending from that node.
*/
void add_nonexcluded_from_HPT_subtree(NodeArray* transfer_set, Path* n)
{
  if(n->exclude->i == n->num_hpt_leaves)  //all leaves in subtree are excluded
    return;

  if(!n->left && !n->right && !n->child_heavypaths)  //leaf of HPT
  {
    addNodeNA(transfer_set, n->node);
    return;
  }

  if(n->left)
    add_nonexcluded_from_HPT_subtree(transfer_set, n->left);
  if(n->right)
    add_nonexcluded_from_HPT_subtree(transfer_set, n->right);
  for(int i=0; i < n->num_child_paths; i++)
    add_nonexcluded_from_HPT_subtree(transfer_set, n->child_heavypaths[i]);
}

/* Traverse to the root of alt_tree while including all the leaves from the
sibling subrees that do not have exclude_this == true.
*/
void include_leaves_from_ancestral_subtrees(NodeArray* transfer_set, Path* n)
{
  bool in_subree = true;
	while(n->total_depth)			//the root of the HPT has no siblings.
	{
    if(n->sibling)
    {                       //n is right child or we're out of the first PT:
      if(!in_subree || n->parent->right == n)
        add_nonexcluded_from_HPT_subtree(transfer_set, n->sibling);

      n = n->parent;
    }
    else              //the root of a PT
    {
      in_subree = false;
      for(int i=0; i < n->parent_heavypath->num_child_paths; i++)
        if(n->parent_heavypath->child_heavypaths[i] != n)
          add_nonexcluded_from_HPT_subtree(transfer_set,
                                           n->parent_heavypath->child_heavypaths[i]);
      n = n->parent_heavypath;
    }
	}
}

/* Visit leaves of the subHPT rooted at this node and add them to transfer_set
if path->node->exclude_this == false.
*/
//void add_subtree_to_TS(Path* n, NodeArray* transfer_set)
//{
//  if(n->left)
//    add_subtree_to_TS(n->left, transfer_set);
//  if(n->right)
//    add_subtree_to_TS(n->right, transfer_set);
//  if(n->node)                         //n represents a node of alt_tree
//  {
//    if(n->child_heavypaths)           //n not a leaf of alt_tree
//      for(int i=0; i < n->num_child_paths; i++)
//        add_subtree_to_TS(n->child_heavypaths[i], transfer_set);
//    else if(!n->node->exclude_this)   //n a leaf of alt_tree and not excluded
//      addNodeNA(transfer_set, n->node);
//  }
//}

/*
Build a path (vector of Path*) from this Path leaf up to the root of the HPT,
following each PT to its root in turn.
*/
void set_path_to_root_HPT(Path* leaf, Path** path_to_root)
{
  int i_path = 0;
  Path* w = leaf;
  while(w != NULL)                    //traverse up between PTs
  {
    while(1)                          //traverse up each PT
    {
      path_to_root[i_path++] = w;
      if(w->parent == NULL)
        break;

      w = w->parent;
    }

    w = w->parent_heavypath;
  }
}

/*
Build a path (vector of Path*) from this node up to the root of the HPT,
following each PT to it's root in turn. The length of the path is
node->total_depth+1.

@warning    user reponsible for memory
*/
Path** get_path_to_root_HPT(Path* node)
{
  int pathlen = node->total_depth+1;
  Path** path_to_root = calloc(pathlen, sizeof(Path*));
  int i_path = 0;

  Path* w = node;
  while(w != NULL)                    //traverse up between PTs
  {
    while(1)                          //traverse up each PT
    {
      path_to_root[i_path++] = w;
      if(w->parent == NULL)
        break;

      w = w->parent;
    }

    w = w->parent_heavypath;
  }
  assert(i_path == pathlen);

  return path_to_root;
}



/*
Return the heavypath rooted at the node root.
   - user responsible for memory of returned heavypath.
*/
Node** get_heavypath(Node* root, int* length)
{
  *length = get_heavypath_length(root);

  Node** heavypath = calloc(*length, sizeof(Node*));
  Node* current = root;
  for(int i=0; i < *length; i++)
  {
    heavypath[i] = current;
    current = current->heavychild;
  }

  return heavypath;
}


/*
Return the number of nodes in the heavy path rooted at this node.
*/
int get_heavypath_length(Node *n)
{
    //Get the heavypath length:
  int length = 1;
  while(n->nneigh != 1)      //not a leaf
  {
    length++;
    n = n->heavychild;
  }
  return length;
}

/*
Return True if the (sub)Path corresponds to a leaf of alt_tree (i.e. it is a
leaf of the HPT).
*/
bool is_HPT_leaf(Path *n)
{
  return n->node && !n->child_heavypaths; //or (n->node && n->node->nneigh == 1)
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - Update Heavypath Tree - - - - - - - - - - - - - -


/*
Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.
*/
void add_leaf_HPT(Node* leaf)
{
  DB_TRACE(0, "alt_tree leaf - "); DB_CALL(0, print_node(leaf));
    //Go down the path from the root to leaf, pushing down and modifying diff
    //values as we go. Subtract 1 for nodes on the path, and nodes above (to
    //the left) in the heavypath, and add 1 for nodes to the right in the
    //heavypath:
  set_path_to_root_HPT(leaf->path, *leaf->path->path_to_root_p);
  Path** path = *leaf->path->path_to_root_p;
  //Path** path = leaf->path->path_to_root;
  int pathlen = path[0]->total_depth + 1; //length (in number of nodes)
  for(int i = pathlen-1; i > 0; i--)      //go from root to parent of leaf.
  {
    if(path[i]->node)                 //leaf of PT (points to node in alt_tree)
    {                                 //(child is root of heavypath)
      addNodeNA(path[i]->exclude, leaf); //exclude leaf contained in a subtree
      for(int j=0; j < path[i]->num_child_paths; j++)
      {
        path[i]->child_heavypaths[j]->diff_path += path[i]->diff_subtree;
        path[i]->child_heavypaths[j]->diff_subtree += path[i]->diff_subtree;

        if(path[i]->child_heavypaths[j] != path[i-1])
        {                                //include leaf for sibling subtrees
          addNodeNA(path[i]->child_heavypaths[j]->include_subtree, leaf);
          addNodeNA(path[i]->child_heavypaths[j]->include_path, leaf);
          path[i]->child_heavypaths[j]->diff_path += 1;
          path[i]->child_heavypaths[j]->diff_subtree += 1;
        }
      }

      path[i]->d_min_path += path[i]->diff_path - 1;
      path[i]->d_max_path = path[i]->d_min_path;
    }
    else                              //internal node of PT
    {
      path[i-1]->diff_path += path[i]->diff_path;
      path[i-1]->diff_subtree += path[i]->diff_subtree;

      if(path[i-1] == path[i]->right) //right child of i is in the path
      {
        addNodeNA(path[i]->left->include_subtree, leaf);
        addNodeNA(path[i]->left->exclude_path, leaf);
        path[i]->left->diff_path += path[i]->diff_path - 1;
        path[i]->left->diff_subtree += path[i]->diff_subtree + 1;
      }
      else                            //left child of i is in the path
      {
        assert(path[i-1] == path[i]->left);
        addNodeNA(path[i]->right->include_path, leaf);
        addNodeNA(path[i]->right->include_subtree, leaf);
        path[i]->right->diff_path += path[i]->diff_path + 1;
        path[i]->right->diff_subtree += path[i]->diff_subtree + 1;
      }
    }

    path[i]->diff_path = path[i]->diff_subtree = 0;
  }
  assert(path[0]->node && path[0]->child_heavypaths == NULL &&
         path[0]->left == NULL && path[0]->right == NULL);       //HPT leaf
  //fprintf(stderr, "HPT leaf exclude:\n\t"); print_HPT_node(path[0]);
  //fprintf(stderr, "\t"); print_node(leaf);
  addNodeNA(path[0]->exclude, leaf);
  //fprintf(stderr, "   exclude address: %p\n", path[0]->exclude);
  path[0]->d_min_path += path[0]->diff_path - 1;
  path[0]->d_max_path = path[0]->d_min_path;
  path[0]->diff_path = path[0]->diff_subtree = 0;

    //Go up the path, updating the min and max values along the way:
  for(int i=1; i < pathlen; i++)
  {
    if(path[i]->child_heavypaths)               //leaf of a PT
    {                                           //d_min_path already set
      path[i]->d_min_subtree = path[i]->child_heavypaths[0]->d_min_path +
                               path[i]->child_heavypaths[0]->diff_path;
      path[i]->d_max_subtree = path[i]->child_heavypaths[0]->d_max_path +
                               path[i]->child_heavypaths[0]->diff_path;

      for(int j=0; j < path[i]->num_child_paths; j++)
      {
        if(is_HPT_leaf(path[i]->child_heavypaths[j]))
        {
          path[i]->d_min_subtree = min(path[i]->d_min_subtree,
                                       path[i]->child_heavypaths[j]->d_min_path+
                                       path[i]->child_heavypaths[j]->diff_path);
          path[i]->d_max_subtree = max(path[i]->d_max_subtree,
                                       path[i]->child_heavypaths[j]->d_max_path+
                                       path[i]->child_heavypaths[j]->diff_path);
        }
        else
        {
          path[i]->d_min_subtree = min3(path[i]->d_min_subtree,
                                        path[i]->child_heavypaths[j]->d_min_path+
                                        path[i]->child_heavypaths[j]->diff_path,
                                        path[i]->child_heavypaths[j]->d_min_subtree+
                                        path[i]->child_heavypaths[j]->diff_path);
          path[i]->d_max_subtree = max3(path[i]->d_max_subtree,
                                        path[i]->child_heavypaths[j]->d_max_path+
                                        path[i]->child_heavypaths[j]->diff_path,
                                        path[i]->child_heavypaths[j]->d_max_subtree+
                                        path[i]->child_heavypaths[j]->diff_path);
        }
      }
    }
    else
    {
      assert(path[i]->left && path[i]->right);  //internal PT node
      path[i]->d_min_path = min(path[i]->left->d_min_path +
                                path[i]->left->diff_path,
                                path[i]->right->d_min_path +
                                path[i]->right->diff_path);
      path[i]->d_max_path = max(path[i]->left->d_max_path +
                                path[i]->left->diff_path,
                                path[i]->right->d_max_path +
                                path[i]->right->diff_path);

      if(is_HPT_leaf(path[i]->left))        //left is leaf of alt_tree
      {
        path[i]->d_min_subtree = path[i]->right->d_min_subtree +
                                 path[i]->right->diff_subtree;
        path[i]->d_max_subtree = path[i]->right->d_max_subtree +
                                 path[i]->right->diff_subtree;
      }
      else if(is_HPT_leaf(path[i]->right))  //right is leaf of alt_tree
      {
        path[i]->d_min_subtree = path[i]->left->d_min_subtree +
                                 path[i]->left->diff_subtree;
        path[i]->d_max_subtree = path[i]->left->d_max_subtree +
                                 path[i]->left->diff_subtree;
      }
      else
      {
        path[i]->d_min_subtree = min(path[i]->left->d_min_subtree +
                                     path[i]->left->diff_subtree,
                                     path[i]->right->d_min_subtree +
                                     path[i]->right->diff_subtree);
        path[i]->d_max_subtree = max(path[i]->left->d_max_subtree +
                                     path[i]->left->diff_subtree,
                                     path[i]->right->d_max_subtree +
                                     path[i]->right->diff_subtree);
      }
    }
  }
}


/* Return the root of the HPT with the given alt_tree leaf.
*/
Path* get_HPT_root(Node* leaf)
{
  assert(leaf->nneigh == 1);
  Path* node = leaf->path;
  Path* w = node;
  while(node != NULL)                 //traverse up between PTs
  {
    while(node->parent != NULL)       //traverse up each PT
      node = node->parent;

    w = node;
    node = node->parent_heavypath;
  }

  return w;
}



/*
Reset the path and subtree min and max, along with the diff values for the
path from the given leaf to the root of the HPT.
*/
void reset_leaf_HPT(Node *leaf)
{
    //Follow the path from the leaf to the root, resetting the values along
    //the way:
  Path* w = leaf->path;
  Path* lastw = w;
  while(w != NULL)                    //traverse up between PTs
  {
    w->diff_path = w->diff_subtree = 0;
    w->d_min_path = w->d_max_path = w->node->subtreesize;
    if(!is_HPT_leaf(w))               //this PT leaf is not a HPT leaf
    {
      w->d_min_subtree = min(lastw->d_min_path, lastw->d_min_subtree);
      w->d_max_subtree = max(lastw->d_max_path, lastw->d_max_subtree);
      for(int i=0; i < w->num_child_paths; i++)
        if(w->child_heavypaths[i] != lastw)
        {
          w->child_heavypaths[i]->diff_path = 0;
          w->child_heavypaths[i]->diff_subtree = 0;
          clearNA(w->child_heavypaths[i]->include_subtree);
          clearNA(w->child_heavypaths[i]->include_path);
        }
    }

    clearNA(w->exclude);
    while(w->parent != NULL)          //traverse up each PT
    {
      w = w->parent;
      clearNA(w->exclude);

      w->diff_path = w->diff_subtree = 0;
      w->d_min_path = min(w->left->d_min_path, w->right->d_min_path);
      w->d_max_path = max(w->left->d_max_path, w->right->d_max_path);
      w->d_min_subtree = 1;
      w->d_max_subtree = max(w->left->d_max_subtree, w->right->d_max_subtree);

      w->left->diff_path = w->left->diff_subtree = 0;
      w->right->diff_path = w->right->diff_subtree = 0;
      clearNA(w->right->exclude_path);
      clearNA(w->right->include_subtree);
      clearNA(w->right->include_path);
      clearNA(w->left->exclude_path);
      clearNA(w->left->include_subtree);
      clearNA(w->left->include_path);
    }

    lastw = w;
    w = w->parent_heavypath;
  }
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - I/O - - - - - - - - - - - - - - - - - - - - -

/*
Print the given heavypath.
*/
void print_heavypath(Node **heavypath, int length)
{
  for(int i=0; i < length; i++)
    printf("%i ", heavypath[i]->id);
  printf("\n");
}

/*
Print the given Path node.
*/
void print_HPT_node(const Path* n)
{
  char *name = "----";
  if(n->node && n->node->nneigh == 1)  //a leaf
    name = n->node->name;
  fprintf(stderr, "path id: %i name: %s\n", n->id, name);
}


/*
Print the Heavy Path Tree (HPT) in dot format. HPT edges will be dashed, while
edges of alt_tree (not in the HPT) will be solid.
*/
void print_HPT_dot(Path* hproot, Node* altroot, int filenameid)
{
  char filename[15];
  sprintf(filename, "hptree_%i.dot", filenameid);
  fprintf(stderr, "Creating DOT: %s\n", filename);
  FILE *f = fopen(filename, "w");
  if(f == NULL)
  {
    fprintf(stderr, "Can't open file for writing!");
    exit(0);
  }

  fprintf(f, "digraph HPT\n  {\n  center=true;\n");
  print_HPT_keynode_dot(f);
  print_HPT_subpath_dot(hproot, f);
  print_HPT_subtree_dot(altroot, f);
  fprintf(f, "  }\n");
  fclose(f);
}

/*
Recursively print the subPath (for the tree structure on the Path).
*/
void print_HPT_subpath_dot(Path* n, FILE *f)
{
  if(n->left)
  {
    print_HPT_ptnode_dot(n, f);       //print node formatting information.

    fprintf(f, "  ");
    print_HPT_node_dot(n, f);
    fprintf(f, " -> ");
    print_HPT_node_dot(n->left, f);
    fprintf(f, " [style=dashed];\n");

    print_HPT_subpath_dot(n->left, f);
  }
  if(n->right)
  {
    //print_HPT_ptnode_dot(n, f);     //already printed this for n->left.

    fprintf(f, "  ");
    print_HPT_node_dot(n, f);
    fprintf(f, " -> ");
    print_HPT_node_dot(n->right, f);
    fprintf(f, " [style=dashed];\n");

    print_HPT_subpath_dot(n->right, f);
  }
  if(n->node)                         //node of alt_tree
  {
    print_HPT_hpnode_dot(n, f);       //print node formatting information.

    if(n->child_heavypaths)           //not a leaf of HPT (and alt_tree)
    {
      for(int i=0; i < n->num_child_paths; i++)
      {
        fprintf(f, "  ");
        print_HPT_node_dot(n, f);
        fprintf(f, " -> ");
        print_HPT_node_dot(n->child_heavypaths[i], f);
        fprintf(f, " [style=dashed color=gray];\n");

        print_HPT_subpath_dot(n->child_heavypaths[i], f);
      }
    }
  }
}


/*
Recursively print the subtree (for alt_tree edges).
*/
void print_HPT_subtree_dot(Node* node, FILE *f)
{
  if(node->nneigh > 1)    //not leaf
  {
    int firstchild = 1;
    if(node->depth == 0)
      firstchild = 0;

    for(int i = firstchild; i < node->nneigh; i++)
    {
      fprintf(f, "  ");
      print_HPT_node_dot(node->path, f);
      fprintf(f, " -> ");
      print_HPT_node_dot(node->neigh[i]->path, f);
      fprintf(f, ";\n");

      print_HPT_subtree_dot(node->neigh[i], f);
    }
  }
}


/*
Print a string representing this Path node formatted for dot output.
*/
void print_HPT_node_dot(Path* n, FILE *f)
{
  fprintf(f, "%i", n->id);
}

/*
Print a string that formats a heavypath (alt_tree) node in a PT (PathTree).

A heavypath node will be circular and have the following format:
  1 (2)                  id (alt_id)
p: 0 1 1    diff_path    d_min_path    d_max_path
s: 0 1 1    diff_subtree d_min_subtree d_max_subtree

if it's an internal node of alt_tree, and:

 3 (2):a              id (alt_id): name
P: 0 1 1    diff_path    d_min_path    d_max_path
s: 0 1 1    diff_subtree d_min_subtree d_max_subtree

if it's a leaf of alt_tree.
*/
void print_HPT_hpnode_dot(Path* n, FILE *f)
{
  if(n->node->nneigh == 1)              //a leaf of alt_tree
    fprintf(f, "  %i [label=\"%i (%i): %s",
            n->id, n->id, n->node->id, n->node->name);
  else
    fprintf(f, "  %i [label=\"%i (%i)", n->id, n->id, n->node->id);

  fprintf(f, "\np: %i %i %i\n", n->diff_path, n->d_min_path, n->d_max_path);

  if(n->node->nneigh == 1)              //a leaf of alt_tree
    fprintf(f, "s: %i x x", n->diff_subtree);
  else
    fprintf(f, "s: %i %i %i", n->diff_subtree, n->d_min_subtree, n->d_max_subtree);

  fprintf(f, "\"];\n");
}

/* Print a string that formats a PT (PathTree) node.

A pathtree node will be rectangular and have the following format:
     1                      id
p: 0 1 1    diff_path    d_min_path    d_max_path
s: 0 1 1    diff_subtree d_min_subtree d_max_subtree
*/
void print_HPT_ptnode_dot(Path* n, FILE *f)
{
  fprintf(f, "  %i [shape=rectangle ", n->id);
  fprintf(f, "label=\"%i", n->id);
  fprintf(f, "\np: %i %i %i\ns: %i %i %i", n->diff_path, n->d_min_path,
          n->d_max_path, n->diff_subtree, n->d_min_subtree, n->d_max_subtree);
  fprintf(f, "\"];\n");
}

/* Print a node that describes the values in the positions.
*/
void print_HPT_keynode_dot(FILE *f)
{
  fprintf(f, "  keynode [shape=record ");
  fprintf(f, "label=\"{path id (node id)|{");
  fprintf(f, "{diff_path|diff_subtree} | ");
  fprintf(f, "{min_path|min_subtree} | ");
  fprintf(f, "{max_path|max_subtree}}}");
  fprintf(f, "\"];\n");
}

/* Recursively print the include and exclude sets for the nodes that descend
from the given one.
*/
void print_HPT_sets(Path* node)
{
  fprintf(stderr, "%i\n", node->id);
  fprintf(stderr, "exclude: ");
  for(int i=0; i < node->exclude->i; i++)
    fprintf(stderr, "%s ", node->exclude->a[i]->name);
  fprintf(stderr, "\nexclude_path: ");
  for(int i=0; i < node->exclude_path->i; i++)
    fprintf(stderr, "%s ", node->exclude_path->a[i]->name);
  fprintf(stderr, "\ninclude_path: ");
  for(int i=0; i < node->include_path->i; i++)
    fprintf(stderr, "%s ", node->include_path->a[i]->name);
  fprintf(stderr, "\ninclude_subtree: ");
  for(int i=0; i < node->include_subtree->i; i++)
    fprintf(stderr, "%s ", node->include_subtree->a[i]->name);
  fprintf(stderr, "\n");

  if(node->left)
    print_HPT_sets(node->left);
  if(node->child_heavypaths)             //leaf of PT but not HPT
    for(int i=0; i < node->num_child_paths; i++)
      print_HPT_sets(node->child_heavypaths[i]);
  if(node->right)
    print_HPT_sets(node->right);
}