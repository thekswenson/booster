/*

BOOSTER: BOOtstrap Support by TransfER: 
BOOSTER is an alternative method to compute bootstrap branch supports 
in large trees. It uses transfer distance between bipartitions, instead
of perfect match.

Copyright (C) 2017 Frederic Lemoine, Jean-Baka Domelevo Entfellner, Olivier Gascuel

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef _EDGE_STACK_H_
#define _EDGE_STACK_H_

#include "tree.h"

typedef struct __NodeStackElt NodeStackElt;
typedef struct __NodeStackElt {
  Node *node;
  Edge *edge;
  struct __NodeStackElt *prev;
} NodeStackElt;


typedef struct __NodeStack NodeStack;
typedef struct __NodeStack {
  struct __NodeStackElt *head;
} NodeStack;


NodeStack *new_nodestack();
void nodestack_push(NodeStack *ns, Node *n, Edge *e);
NodeStackElt * nodestack_pop(NodeStack *ns);
void nodestack_free(NodeStack *ns);

#endif
