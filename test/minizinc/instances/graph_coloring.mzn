% Graph Coloring Problem

int: nc;  % number of colors
int: n;   % number of nodes
int: m;   % number of edges

array[1..m, 1..2] of int: edges;
array[1..n] of var 1..nc: color;

constraint forall(i in 1..m)(color[edges[i,1]] != color[edges[i,2]]);

solve satisfy;

output ["Node colors = ", show(color), "\n"];
