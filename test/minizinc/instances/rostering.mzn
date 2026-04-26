% Nurse Rostering Problem

int: n_nurses = 3;
int: n_days = 7;
int: n_shifts = 3;

array[1..n_days, 1..n_shifts] of int: demand;

array[1..n_nurses, 1..n_days, 1..n_shifts] of var 0..1: roster;

% Demand satisfaction
constraint forall(d in 1..n_days, s in 1..n_shifts)(
    sum(n in 1..n_nurses)(roster[n,d,s]) >= demand[d,s]
);

% One shift per day per nurse
constraint forall(n in 1..n_nurses, d in 1..n_days)(
    sum(s in 1..n_shifts)(roster[n,d,s]) <= 1
);

solve satisfy;

output [ if s == n_shifts then show(roster[nu,d,s]) ++ "\n"
         else show(roster[nu,d,s]) ++ " " endif
       | nu in 1..n_nurses, d in 1..n_days, s in 1..n_shifts ];
