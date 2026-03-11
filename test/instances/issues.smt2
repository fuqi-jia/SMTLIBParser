; ========================================
; QF_S filename split (SMTParser was ERROR, Z3 solves SAT) -> issue #29
; ========================================
(set-logic QF_S)
(set-option :produce-models true)

(declare-fun s  () String)
(declare-fun filename_0  () String)
(declare-fun filename_1  () String)
(declare-fun filename_2  () String)
(declare-fun i1 () Int)
(declare-fun i2 () Int)
(declare-fun i3 () Int)
(declare-fun tmpStr0 () String)
(declare-fun tmpStr1 () String)
(declare-fun tmpStr2 () String)

(assert (= filename_0 s) )

; i1 = LastIndexof(filename_0, "/")
(assert (ite (str.contains filename_0 "/")
             (and (= filename_0 (str.++ tmpStr0 (str.++ "/" tmpStr1) ) )
                  (not (str.contains tmpStr1 "/") )
                  (= i1 (str.len tmpStr0) )             
             )
             (= i1 (- 0 1) )
        )
)

(assert (ite (not (= i1 (- 0 1) ) )
             (and (= i2 (- (str.len filename_0) i1) )
                  (= filename_1 (str.substr filename_0 i1 i2) )                  
             )
             (= filename_1 filename_0)
        ) 
)

(assert (= i3 (str.indexof filename_1 "." 0) ) )

(assert (ite (not (= i3 (- 0 1) ) )
             (= filename_2 (str.substr filename_1 0 i3) )
             (= filename_2 filename_1)
        ) 
)

(check-sat)
(get-model)
(reset)

; ========================================
; declare-sort S (SMTParser parsing failure, Z3 solving success) -> issue #31
; ========================================
(declare-sort S 1)
(define-sort SB () (S Bool))
(declare-fun A () (S Bool))
(declare-fun B () SB)
(assert (= A B)) 


; ========================================
; string handling (SMTParser parsing success, Z3 solving failed?) -> issue #32
; ========================================

(declare-fun ss () String)
(declare-fun var () String)
(declare-fun ret () String)



(assert (ite (or (str.contains ss "<") (str.contains ss ">") )
             (= ret "x" )
             (= ret ss)
        )
)

(assert (= var (str.++ "<scr" "ipt") ) )

(assert (str.contains ss var) )

(assert (not (= ret "x") ) )

(check-sat)
(exit)