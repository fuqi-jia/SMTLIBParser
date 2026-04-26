% Optional Delivery Problem (uses opt int)

int: n = 3;

array[1..n] of opt int: delivery_time;
array[1..n] of int: deadline = [10, 20, 15];

constraint forall(i in 1..n)(
    occurs(delivery_time[i]) -> deopt(delivery_time[i]) <= deadline[i]
);

solve satisfy;

output ["Deliveries = ", show(delivery_time), "\n"];
