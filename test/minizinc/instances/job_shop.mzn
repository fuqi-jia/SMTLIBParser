% Job Shop Scheduling

int: n;   % number of jobs
int: m;   % number of machines

array[1..n, 1..m] of int: d;   % durations
array[1..n, 1..m] of int: mc;  % machine for each task

int: max_t = sum(i in 1..n, j in 1..m)(d[i,j]);

array[1..n, 1..m] of var 0..max_t: s;  % start times

% Tasks within a job do not overlap
constraint forall(i in 1..n, j in 1..m-1)(
    s[i,j] + d[i,j] <= s[i,j+1]
);

% Tasks on same machine do not overlap
constraint forall(ma in 1..m)(
    disjunctive(
        [s[i,j] | i in 1..n, j in 1..m where mc[i,j] == ma],
        [d[i,j] | i in 1..n, j in 1..m where mc[i,j] == ma]
    )
);

solve minimize max(i in 1..n)(s[i,m] + d[i,m]);

output ["Makespan = ", show(max(i in 1..n)(s[i,m] + d[i,m])), "\n"];
