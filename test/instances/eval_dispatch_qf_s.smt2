; Parser smoke test: string/regex ops used by evaluateSimpleOp dispatch (Phase B)
(set-logic ALL)

(declare-const s String)
(declare-const t String)
(assert (= s "aba"))
(assert (= t "aaa"))

; Ternary: str.indexof_re takes a start index, like str.indexof.
; This fixture said "Binary" and used two arguments, which is what the
; parser accepted and not what SMT-LIB declares.
(assert (>= (str.indexof_re s (str.to_re "b") 0) 0))

; Ternary: str.replace_re / str.replace_re_all
(assert (= (str.replace_re s (str.to_re "a") "x") "xbx"))
(assert (= (str.replace_re_all t (str.to_re "a") "b") "bbb"))

; Ternary: ((_ re.loop m n) Reg) — Reg, then loop bounds as Int children in internal DAG
(assert (str.in_re "xx" ((_ re.loop 1 2) (str.to_re "x"))))

(check-sat)
(exit)
