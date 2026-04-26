% Sudoku Solver

int: S;
int: N = S * S;

array[1..N,1..N] of var 1..N: x;

constraint forall(i in 1..N)(all_different([x[i,j] | j in 1..N]));
constraint forall(j in 1..N)(all_different([x[i,j] | i in 1..N]));
constraint forall(i,j in 1..S)(
    all_different([x[(i-1)*S + ii, (j-1)*S + jj] | ii,jj in 1..S])
);

solve satisfy;

output [ if j == N then show(x[i,j]) ++ "\n" else show(x[i,j]) ++ " " endif
       | i in 1..N, j in 1..N ];
