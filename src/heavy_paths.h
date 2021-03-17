#ifndef __HEAVY_PATHS_H__
#define __HEAVY_PATHS_H__
/*
  This is the code that uses heavy paths to help when the alternate tree
  (T_o in the paper) is not balanced. We use bookkeeping, instead of a
  balanced datastructure, to maintain the minimum transfer distance over the
  whole tree. This shaves a logarithmic factor off of the running time
  when traveling up the alternate tree from leaf to root.
 
  In the alternate tree, a heavy path is split into a tree ("Path Tree" or
  PT) where each node of the tree represents a subpath of the heavypath. Each
  subpath knows its min/max value on the subpath, and its min/max value for
  all the subtrees hanging off the subpath. This way, when a heavy path is
  updated the min value on the path down to the "exit" node will decrease by 1,
  but the min value off the path will increase by 1, along with the rest of
  the heavy path below the exit node.

  The PTs are joined together into a "HeavyPath Tree" (HPT), where a root of
  a PT is connected to a leaf of another PT using the parent_heavypath and
  child_heavypath pointers.
*/

#include "tree.h"
#include "limits.h"
#include "debug.h"
#include "stdbool.h"


typedef struct __Tree Tree;
typedef struct __Node Node;
typedef struct __NodeArray NodeArray;

/*
  We use the following conventions:
  - the original alt_tree is decomposed into heavypaths
  - a heavypath is represented by a binary tree of Path objects (called the
    PathTree PT) where:
    * the root of the HP tree contains information for the entire heavypath
    * the leaves of the HP tree contain node information for the corresponding
      node in alt_tree, along with pointers (child_heavypaths) to the
      pendant heavypaths (in the case of a binary tree there is only one
      pendant heavypath).

  In other words, a tree of Paths (PT) represents a heavypath, where the Path
  object can be an internal subpath node, the root (which contains the
  summary bookkeeping for the entire heavypath), or a leaf.
  In the case of a PT leaf, the Path represents the node in alt_tree on the
  current heavypath, pointed to by ->node, and the pendant heavypath is pointed
  to by ->child_heavypath.
 
  The entire group of PTs, that are glued together by child_heavypath pointers,
  is called the HeavyPathTree (HPT).

  If the PT leaf is also a leaf of alt_tree, then n->left and n->right and
  n->child_heavypaths are NULL and the path_to_root is an array of Path objects
  that represent the path in the HPT from the leaf to the root of the HPT.
*/
typedef struct __Path Path;
typedef struct __Path {
  int id;          //A unique identifier.

    //The structure of the heavypath tree:
  Path* left;      //Left child. This leads to higher nodes on the heavypath.
  Path* right;     //Right child. This leads to lower nodes on the heavypath.
  Path* parent;
  Path* sibling;

  Node* node;               //The node of alt_tree corresponding to this Path.
                            // (this applies only to leaves of the PT)

  Path** child_heavypaths;  //The array of Path tree roots pendant to this Path.
  int num_child_paths;      //The size of the child_heavypaths array.
  Path* parent_heavypath;   //The Path that this PT hangs on.

  int total_depth;          //# of Path structs to root through all PTs
                            // (i.e. # nodes in path to HPT root).

  Path*** path_to_root_p;   //Pointer to array of Path objects to root.
                            // (this points to memory shared between all HPT
                            //  leaves -- it is set once when used)

    //The transfer index (TI) values:
  int diff_path;      //Diff to add to subtree rooted on path.
  int diff_subtree;   //Diff to add to pendant subtrees. This is necessary since
                      //the alt-tree nodes to the left of the path in the PT
                      //must get -1 (diff_path -= 1), whereas the subtrees of
                      //those nodes must get +1 (diff_subtree += 1).

  int d_min_path;     //Min value for the subpath.
  int d_min_subtree;  //Min value over all pendant subtrees for this (sub)Path.

  int d_max_path;     //Max value for the subpath.
  int d_max_subtree;  //Max value over all pendant subtrees for this (sub)Path.
    //The transfer set (TS) values:
  NodeArray* include_path;    //Included leaves in TS for nodes on the (sub)path.
  NodeArray* include_subtree; //Included leaves in the transfer set for the subtrees.
  NodeArray* exclude;         //Excluded leaves from TS for this node (not pushed down).
  NodeArray* exclude_path;    //Excluded leaves from TS for this (sub)path (pushed down).
} Path;

/*
Allocate a new Path, setting all the default values.
*/
Path* new_Path();

/* Recursiveley decompose the alternate tree into heavy paths according to
the scheme described in the definition of the Path struct. Return the root
Path of the Path tree.

@note   each heavypath corresponds to a tree of Paths we call the PathTree (PT)
        leaves of the PTs are glued the roots of other PTs (using the
        child_heavypath pointers).
        We call the entire tree the HeavyPathTree (HPT)

@note   Allocate a pointer to an array that will hold a path to the root for
        a leaf in the HPT, for use when calculating add_leaf_HPT(). Each leaf
        of the HPT will have path_to_root_p set to this value, as well as
        the root of the HPT.
*/
Path* do_heavy_decomposition(Node* root);
Path* heavy_decomposition(Node* root, int depth, int* maxdepth,
                          Path*** path_to_root_pointer);

/* Free the memory for the HeavyPathTree (allocated in heavy_decomposition).
*/
void free_HPT(Path* root);
void free_HPT_rec(Path* node);

/* For the given heavypath, create a Path structure that represents the path.
Split the path in half and create a Path for each half.  If a half is a single
node, then hang the next heavy path off of it. If it's a leaf of alt_tree, then
link the Path to the corresponding leaf in alt_tree.
*/
Path* partition_heavypath(Node** n, int length, int depth, int* maxdepth,
                          Path*** path_to_root_pointer);

/*
Return a Path for the given node of alt_tree.  The Path will be a leaf
node of the path tree.
Either 1) the leaf will point to a node (internal or leaf) of alt_tree,
or     2) child_heavypath will point to a heavypath representing the
          descendant of the alt_tree node.
*/
Path* heavypath_leaf(Node* node, int depth, int* maxdepth,
                     Path*** path_to_root_pointer);

/* Return the min transfer index in the subtree rooted at path.
*/
int get_ti_min(Path* hptroot);

/* Return the transfer index in the subtree rooted at path, given the
modifying values. Return the max if usemax is true, other return the min.
*/
int get_ti_x_mod(Path* path, int accum_path, int accum_subtree, bool usemax);

/* Return the min transfer index in the subtree rooted at path, given the
modifying values.
*/
int get_ti_min_mod(Path* path, int accum_path, int accum_subtree);

/* Return the max transfer index in the subtree rooted at path.
*/
int get_ti_max(Path* hptroot);

/* Return the max transfer index in the subtree rooted at path, given the
modifying values.
*/
int get_ti_max_mod(Path* path, int accum_path, int accum_subtree);

/* Return the heavypath rooted at the node.
   ** user responsible for memory of returned heavypath.
*/
Node** get_heavypath(Node* root, int* length);

/* Return the length of the heavy path rooted at this node.
*/
int get_heavypath_length(Node* n);

/* Return True if the (sub)Path corresponds to a leaf of alt_tree (i.e. it is a
leaf of the HPT).
*/
bool is_HPT_leaf(Path* n);

/*
Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.
*/
void add_leaf_HPT(Node* leaf);

/* Print the given heavypath.
*/
void print_heavypath(Node** heavypath, int length);

/* Return the transfer set for the best node in the Heavy Path Tree to which
the given root Path belongs.

@note  user responsible for the memory

Descend from the root of the HPT as long as the best value in the child's
subtree is equal to the best of the whole tree. On the way down, add the
included leaves. Once to the bottom, mark the excluded leaves for that node.
Visit all of the leaves in the subtree keeping only those that are not marked
to be excluded.
*/
NodeArray* get_transfer_set_HPT(Path* hptroot, Tree* alt_tree);


/* Visit leaves of the subHPT rooted at this node and add them to transfer_set
if where exclude_this == false.
*/
//void add_subtree_to_TS(Path* n, NodeArray* transfer_set);

/* Return the Path corresponding to the node in alt_tree with the minimum
transfer distance.
*/
Path* get_min_path(Path* hptroot);

/* Return the Path corresponding to the node in alt_tree with the maximum
transfer distance.
*/
Path* get_max_path(Path* hptroot);

/* Return the Path corresponding to the node in alt_tree with the minimum
transfer distance if usemax is true, otherwise return the min.
*/
Path* get_x_path(Path* hptroot, bool usemax);

/* Return the transfer set for the given node. If usemax is true, then
get the leaf set corresponding to the max value.
node).

@note  you must free any memory associated to t->transfer_set before calling
       this function
*/
NodeArray* get_transfer_set_for_node_HPT(Path* n, Tree* alt_tree, bool usemax);

/* Return the min transfer set for the given node.
*/
NodeArray* get_min_transfer_set_for_node_HPT(Path* n, Tree* alt_tree);

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
NodeArray* get_max_transfer_set_for_node_HPT(Path* n, Tree* alt_tree);

/* Traverse to the root of alt_tree while including all the leaves from the
sibling subrees that do not have exclude_this == true.
*/
void include_leaves_from_ancestral_subtrees(Tree* t, Node* n);

/* Visit leaves of the subHPT rooted at this node and add them to transfer_set
if path->node->exclude_this == false.
*/
void add_subtree_to_TS(Path* n, NodeArray* transfer_set);

/*
Descend to the leaves of the HPT. Once there, create the path_to_root array
for that Path object.
*/
//void set_paths_to_root(Path* node);

/* Build a path from this node up to the root of the HPT, following each
PT to it's root in turn. The length of the path is node->total_depth+1.

@warning    user reponsible for memory
*/
Path** get_path_to_root_HPT(Path* node);

/* Build a path from this Path leaf up to the root of the HPT, following each
PT to its root in turn.
*/
void set_path_to_root_HPT(Path* leaf, Path** path_to_root);


/* Return the root of the HPT with the given alt_tree leaf.
*/
Path* get_HPT_root(Node* leaf);

/* Reset the path and subtree min and max, along with the diff values for the
path from the given leaf to the root of the HPT.
*/
void reset_leaf_HPT(Node* leaf);

/*--------------------- OUTPUT FUNCTIONS -------------------------*/


/* Print the Heavy Path Tree (HPT) in dot format.
*/
void print_HPT_dot(Path* hproot, Node* altroot, int repid);

/* Print the given Path node.
*/
void print_HPT_node(const Path* n);

/* Recursively print the subPath.
*/
void print_HPT_subpath_dot(Path* n, FILE* f);

/* Recursively print the subtree.
*/
void print_HPT_subtree_dot(Node* n, FILE* f);



/* Print a string representing this Path node formatted for dot output.
*/
void print_HPT_node_dot(Path* n, FILE* f);

/* Print a string that formats a heavypath node in a PT.
*/
void print_HPT_hpnode_dot(Path* n, FILE* f);

/* Print a string that formats a PT node.
*/
void print_HPT_ptnode_dot(Path* n, FILE* f);

/* Print a descriptive node.
*/
void print_HPT_keynode_dot(FILE* f);



/* Recursively print the include and exclude sets for the nodes that descend
from the given one.
*/
void print_HPT_sets(Path* node);

#endif
