% Warehouse Location Problem

int: n_customers;
int: n_warehouses;

array[1..n_customers] of int: demand;
array[1..n_warehouses] of int: capacity;
array[1..n_warehouses] of int: fixed_cost;
array[1..n_customers, 1..n_warehouses] of int: transport_cost;

array[1..n_warehouses] of var 0..1: open;
array[1..n_customers, 1..n_warehouses] of var 0..1: supply;

% Each customer's demand is met
constraint forall(c in 1..n_customers)(
    sum(w in 1..n_warehouses)(supply[c,w]) == 1
);

% Warehouse capacity
constraint forall(w in 1..n_warehouses)(
    sum(c in 1..n_customers)(demand[c] * supply[c,w]) <= capacity[w] * open[w]
);

solve minimize sum(w in 1..n_warehouses)(fixed_cost[w] * open[w])
             + sum(c in 1..n_customers, w in 1..n_warehouses)(
                 transport_cost[c,w] * demand[c] * supply[c,w]
               );

output ["Open warehouses = ", show(open), "\n"];
