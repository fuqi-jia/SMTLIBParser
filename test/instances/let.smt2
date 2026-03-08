(set-info :smt-lib-version 2.6)
(set-logic QF_BV)
(set-info :category "crafted")
(set-info :status sat)
;; let.smt2: examples of (let ((var expr)+) body) from storeinv, ax, QF_BV_bv_eq, simplemean

(declare-fun x () (_ BitVec 8))
(declare-fun y () (_ BitVec 8))
(declare-fun z () (_ BitVec 8))
(declare-fun a () (_ BitVec 1))
(declare-fun b () (_ BitVec 1))
(declare-fun c () (_ BitVec 1))
(declare-fun p () (_ BitVec 1))
(declare-fun q () (_ BitVec 1))

;; 1. Single binding, body uses the bound var
(assert
 (let ((?v (bvadd x (_ bv1 8))))
 (= ?v (bvadd y (_ bv2 8)))))

;; 2. Nested let (style from storeinv_t2 / array benchmarks)
(assert
 (let ((?a (bvand x y)))
  (let ((?b (bvadd ?a z)))
   (= ?b (bvor x (bvnot (_ bv0 8)))))))

;; 3. Let with ite (style from QF_BV_bv_eq_sdp_v2_cc_reg_max.smt2)
(assert
 (let ((?x4343 (ite (and (= a (_ bv1 1)) (= b (_ bv1 1)) (= c (_ bv1 1))) (_ bv1 1) (_ bv0 1))))
 (= p ?x4343)))

;; 4. Deeply nested let with and/ite (style from QF_BV_bv_eq)
(assert
 (let (($x303 (= p (_ bv1 1))))
  (let (($x96 (= q (_ bv1 1))))
   (let (($x2798 (and $x96 $x303 (= a (_ bv1 1)) (= b (_ bv1 1)))))
    (= c (ite $x2798 (_ bv1 1) (_ bv0 1)))))))

;; 5. BV let with extract and bvadd/bvand/bvlshr (style from simplemean_buckets)
(assert
 (let ((?x793 (bvadd (bvand x y) (bvlshr (bvxor x y) (_ bv1 8)))))
  (let ((?x756 ((_ extract 7 4) ?x793)))
   (let ((?x352 ((_ extract 7 4) x)))
    (= (_ bv0 4) (ite (bvuge ?x756 ?x352) (bvsub ?x756 ?x352) (bvsub ?x352 ?x756)))))))

(check-sat)
(exit)
