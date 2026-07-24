; Real number theory edge cases
(set-logic QF_LRA)

; ========================================
; Section 1: Special Real Number Formats
; ========================================

; Zero in different forms
(declare-const zero1 Real)
(declare-const zero2 Real)
(declare-const zero3 Real)
(assert (= zero1 0.0))
(assert (= zero2 0.00000))
(assert (= zero3 (/ 0 1)))

; Very small numbers
(declare-const tiny Real)
(assert (= tiny 0.00000000000000000001))

; Very large numbers
(declare-const huge Real)
(assert (= huge 999999999999999999999.999999999999999))

; Negative numbers
(declare-const neg Real)
(assert (= neg (- 123.456)))

; Fractions
(declare-const frac1 Real)
(declare-const frac2 Real)
(assert (= frac1 (/ 1 3)))
(assert (= frac2 (/ 22 7))) ; approximation of pi

; ========================================
; Section 2: Division Edge Cases
; ========================================

; Division by very small number
(declare-const small_div Real)
(assert (= small_div (/ 1.0 0.00001)))

; Multiple divisions
(declare-const multi_div Real)
(assert (= multi_div (/ (/ (/ 8.0 2.0) 2.0) 1.0)))

; ========================================
; Section 3: Comparison Edge Cases
; ========================================

; Very close numbers
(declare-const close1 Real)
(declare-const close2 Real)
(declare-const x Real)
(assert (= close1 1.000000000000001))
(assert (= close2 1.000000000000002))
(assert (not (= close1 close2)))

; Numbers differing only in sign
(declare-const pos Real)
(declare-const neg_same Real)
(assert (= pos 5.5))
(assert (= neg_same (- 5.5)))
(assert (< neg_same pos))

(assert (< close1 (root-obj (+ (^ x 2) (- 3)) 1)))
(assert (< close2 (root-of-with-interval (coeffs 1 (- 2)) 1 2)))

(check-sat)
(exit)
