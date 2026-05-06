; Parse regression: declare-datatypes (Either: left / right, Int selectors only)
(set-logic ALL)

(declare-datatypes ((Either 0)) (((left (lv Int)) (right (rv Int)))))

(assert (is-left (left 7)))
(assert (not (is-right (left 7))))
(assert (is-right (right 3)))
(assert (not (is-left (right 3))))
(assert (= (lv (left 42)) 42))
(assert (= (rv (right 99)) 99))

(check-sat)
(exit)
