% 0-1 Knapsack Problem

int: n;
array[1..n] of int: w;
array[1..n] of int: v;
int: W;

array[1..n] of var 0..1: x;

constraint sum(i in 1..n)(w[i] * x[i]) <= W;

solve maximize sum(i in 1..n)(v[i] * x[i]);

output ["Total value = ", show(sum(i in 1..n)(v[i] * fix(x[i]))), "\n"];
