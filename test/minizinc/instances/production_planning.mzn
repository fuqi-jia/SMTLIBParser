% Production Planning Problem

int: n = 2;
array[1..n] of int: profit = [10, 20];
array[1..n] of int: resource = [3, 4];
int: max_res = 10;

array[1..n] of var int: prod;

constraint forall(i in 1..n)(prod[i] >= 0);
constraint sum(i in 1..n)(resource[i] * prod[i]) <= max_res;

solve maximize sum(i in 1..n)(profit[i] * prod[i]);

output ["Production = ", show(prod), "\n"];
