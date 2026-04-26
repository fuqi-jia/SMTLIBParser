% SEND + MORE = MONEY
% Each letter represents a distinct digit.

var 0..9: S; var 0..9: E; var 0..9: N; var 0..9: D;
var 0..9: M; var 0..9: O; var 0..9: R; var 0..9: Y;

constraint all_different([S,E,N,D,M,O,R,Y]);

constraint S > 0 /\ M > 0;

constraint 1000*S + 100*E + 10*N + D
         + 1000*M + 100*O + 10*R + E
         = 10000*M + 1000*O + 100*N + 10*E + Y;

solve satisfy;

output ["SEND = ", show(S), show(E), show(N), show(D), "\n",
        "MORE = ", show(M), show(O), show(R), show(E), "\n",
        "MONEY= ", show(M), show(O), show(N), show(E), show(Y), "\n"];
