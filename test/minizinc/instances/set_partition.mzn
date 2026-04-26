% Set Partition Problem

int: n = 3;

var set of 1..n: S;
var set of 1..n: T;

constraint S union T = {1, 2, 3};
constraint S intersect T = {};

solve satisfy;

output ["S = ", show(S), "\nT = ", show(T), "\n"];
