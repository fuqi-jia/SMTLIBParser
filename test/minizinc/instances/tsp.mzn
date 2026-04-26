% Traveling Salesman Problem

int: n;
array[1..n, 1..n] of int: d;

array[1..n] of var 1..n: next;

constraint circuit(next);

solve minimize sum(i in 1..n)(d[i, next[i]]);

output ["next = ", show(next), "\n"];
