; Parse regression: uninterpreted function applications in assertions
(set-logic ALL)

(declare-fun f (Int) Int)
(declare-fun h (Int Int) Int)

(assert (= (f 0) (f 0)))
(assert (> (+ (f 1) (f 2)) 0))
(assert (= (h 2 3) (h 2 3)))
(assert (not (= (h 1 1) (h 2 2))))

(check-sat)
(exit)
