% Magic Square Problem

int: n;
int: s = n * (n*n + 1) div 2;

array[1..n, 1..n] of var 1..n*n: x;

constraint all_different([x[i,j] | i in 1..n, j in 1..n]);

constraint forall(i in 1..n)(sum(j in 1..n)(x[i,j]) == s);
constraint forall(j in 1..n)(sum(i in 1..n)(x[i,j]) == s);
constraint sum(i in 1..n)(x[i,i]) == s;
constraint sum(i in 1..n)(x[i,n+1-i]) == s;

solve satisfy;

output [ if j == n then show(x[i,j]) ++ "\n" else show(x[i,j]) ++ " " endif
       | i in 1..n, j in 1..n ];
