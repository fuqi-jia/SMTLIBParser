; Parser smoke test: const array + store/select (evaluates via NT_CONST_ARRAY / array simplification)
(set-logic ALL)

(declare-const i Int)
(declare-const a (Array Int Int))

(assert (= a (store ((as const (Array Int Int)) 0) i 1)))
(assert (= (select ((as const (Array Int Int)) 7) 42) 7))

(check-sat)
(exit)
