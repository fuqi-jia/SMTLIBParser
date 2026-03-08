; Test: numerals must be parsed as integer constants when the same token
; could also resolve to a declared symbol (e.g. |486| or |313|). The parser
; should treat 486 in (>= 486 x) as the Int constant 486, not as symbol |486|.
(set-logic QF_LIA)

; Declare symbols whose names are decimal numerals (as in real benchmarks)
(declare-fun |313| () Bool)
(declare-fun |486| () Bool)

; Int variables used in constraints with numeral literals
(declare-fun |nWeight(3)| () Int)
(declare-fun |lr316| () Int)

; Numeral 486 must be parsed as Int, not as symbol |486| (Bool)
(assert (>= 486 |nWeight(3)|))
(assert (<= 0 |nWeight(3)|))

; Numeral 313 must be parsed as Int, not as symbol |313| (Bool)
(assert (>= 313 |lr316|))
(assert (<= 1 |lr316|))

; Use the Bool symbols explicitly so they remain declared and in scope
(assert (or (not |313|) (not |486|)))

(check-sat)
(exit)
