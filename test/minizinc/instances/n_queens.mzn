% N-Queens Problem

int: n;
array[1..n] of var 1..n: q;

constraint all_different(q);
constraint all_different([q[i] + i | i in 1..n]);
constraint all_different([q[i] - i | i in 1..n]);

solve satisfy;

output [ if fix(q[i]) == j then "Q " else ". " endif ++
         if j == n then "\n" else "" endif
       | i in 1..n, j in 1..n ];
