#include "somtparser/frontend/parser.h"
#include <iostream>
#include <string>
#include "test_helpers.h"

int main() {
    // Create parser
    auto parser = SOMTParser::newParser();
    
    // SMT-LIB model strings for testing
    std::string model_str1 = R"(
(model
  (define-fun v17 () Real
    0.0)
  (define-fun v15 () Real
    (- 1.0))
  (define-fun v7 () Real
    1.0)
  (define-fun v12 () Real
    (- 1.0))
  (define-fun v1 () Real
    1.0)
  (define-fun v13 () Real
    (- 1.0))
  (define-fun v6 () Real
    (- (/ 1.0 4.0)))
  (define-fun v2 () Real
    1.0)
  (define-fun v16 () Real
    1.0)
  (define-fun v18 () Real
    (- 1.0))
  (define-fun v10 () Real
    (/ 1.0 2.0))
  (define-fun v3 () Real
    0.0)
  (define-fun v14 () Real
    (- 8.0))
  (define-fun v9 () Real
    1.0)
  (define-fun v11 () Real
    1.0)
  (define-fun v4 () Real
    (- (/ 1.0 4.0)))
  (define-fun v5 () Real
    0.0)
  (define-fun v8 () Real
    0.0)
  (define-fun hypothesis () Bool
    (let ((a!1 (+ (* v10 (* v2 v2) v5) (* (- 1.0) v1 v10 v2 v7) (* (* v2 v2) v7 v9))))
  (and (> v4 (- 1.0))
       (> v7 0.0)
       (> a!1 0.0)
       (< v4 0.0)
       (< v6 0.0)
       (< a!1 (* v2 v2)))))
  (define-fun assumptions () Bool
    (let ((a!1 (= (+ (* v14 v18 (* v2 v2 v2) v4) (* v14 v17 (* v2 v2 v2) v7))
              (+ (* v16 v17 (* v2 v2 v2) v4)
                 (* (- 1.0) v1 v12 (* v16 v16) v2 v5)
                 (* (* v1 v1) v12 (* v16 v16) v7)
                 (* v15 v16 (* v2 v2 v2) v7))))
      (a!2 (= (+ v2
                 (* v2 v4)
                 (* (- 1.0) v2 v5)
                 (* (- 1.0) v10 v2 v5)
                 (* v2 v6)
                 (* v1 v10 v7)
                 (* (- 1.0) v2 v7 v9))
              0.0))
      (a!3 (= (+ (* v18 v4)
                 (* 2.0 v18 v3 v4)
                 (* v18 (* v3 v3) v4)
                 (* v17 v7)
                 (* 2.0 v17 v3 v7)
                 (* v17 (* v3 v3) v7)
                 (* v11 v8))
              (+ (* v13 v6) (* v13 v3 v6)))))
  (and (> (* v14 v18) (* v16 v17))
       (> (* v14 v17) (* v15 v16))
       (> (* v2 v9) 0.0)
       (> (* v2 v2 v9) (* v1 v10 v2))
       (< (* (* v1 v1) v12 v2) 0.0)
       (> v10 0.0)
       (< (* v12 v2) 0.0)
       (> v1 0.0)
       (> v2 0.0)
       (> v11 0.0)
       (< v13 0.0)
       (> v3 (- 1.0))
       (> v16 0.0)
       (< v14 0.0)
       (< v18 0.0)
       (< v15 0.0)
       (> (* v15 v18) (* v17 v17))
       a!1
       a!2
       a!3
       (= v8 0.0)
       (= v5 0.0))))
  (define-fun /0 ((x!0 Real) (x!1 Real)) Real
    (ite (and (= x!0 (- (/ 1.0 4.0))) (= x!1 1.0)) (- (/ 1.0 4.0))
      0.0))
)
)";

    std::string model_str2 = R"(
    (
  (define-fun | | () (_ BitVec 4)
    #xc)
  (define-fun v1 () (_ BitVec 4)
    #x0)
  (define-fun ?v0 () (_ BitVec 4)
    #x1)
  (define-fun notes () (_ BitVec 4)
    #x7)
  (define-fun V0 () (_ BitVec 4)
    #x5)
  (define-fun ~!@$%^&*_-+=><.?/ () (_ BitVec 4)
    #xa)
  (define-fun |~!@$%^&*_-+=<>.?/()| () (_ BitVec 4)
    #xe)
  (define-fun v0 () (_ BitVec 4)
    #xf)
  (define-fun  () (_ BitVec 4)
    #x3)
  (define-fun ~!@$%^&*_-+=<>.?/ () (_ BitVec 4)
    #xf)
)
    )";

  // Simplified version to reproduce the as-array issue
  std::string model_str3_simple = R"(
  (
  (define-fun testVar () (Array Int Int)
    (_ as-array k!52))
  (define-fun k!52 ((x!0 Int)) Int
    (ite (= x!0 1) 10 20))
  )
  )";

  std::string model_str3_original = R"(
  (
  (define-fun |c_#memory_$Pointer$.offset_primed| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!54))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!49))))
  (define-fun |c_aux_v_#memory_$Pointer$.offset_79| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!54))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!49))))
  (define-fun |c_aux_v_#memory_int_85| () (Array Int (Array Int Int))
    (store (store ((as const (Array Int (Array Int Int)))
                ((as const (Array Int Int)) 164))
              42
              (_ as-array k!24))
       46
       (_ as-array k!25)))
  (define-fun |c_aux_v_#memory_$Pointer$.base_83| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!30))))
  (store (store a!1 54 (_ as-array k!36)) 52 (_ as-array k!8))))
  (define-fun |c_~#__CS_thread_allocated~0.offset| () Int
    4094277)
  (define-fun |c_aux_v_#memory_$Pointer$.offset_80| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!19))
                  46
                  (_ as-array k!22))))
  (store (store a!1 54 (_ as-array k!17)) 52 (_ as-array k!16))))
  (define-fun |c_aux_v_#memory_$Pointer$.offset_77| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!19))
                  46
                  (_ as-array k!57))))
  (store (store a!1 54 (_ as-array k!17)) 52 (_ as-array k!16))))
  (define-fun |c_aux_v_#memory_$Pointer$.base_76| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!63))))
  (store (store a!1 54 (_ as-array k!36)) 52 (_ as-array k!47))))
  (define-fun |c_~#full~0.base| () Int
    46)
  (define-fun |c_aux_v_#memory_$Pointer$.base_78| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!11))
                  46
                  (_ as-array k!10))))
  (store (store a!1 54 (_ as-array k!9)) 52 (_ as-array k!8))))
  (define-fun |c_~#__CS_thread_born_round~0.base| () Int
    46)
  (define-fun |c_~#__CS_thread_lockedon~0.offset| () Int
    4094267)
  (define-fun |c_aux_v_#memory_$Pointer$.offset_78| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!54))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!49))))
  (define-fun |c_aux_v_#memory_$Pointer$.offset_81| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!57))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!49))))
  (define-fun |c_aux_v_#memory_$Pointer$.offset_82| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!51))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!49))))
  (define-fun |c_~#m~0.base| () Int
    52)
  (define-fun |c_~#__CS_thread_born_round~0.offset| () Int
    4094289)
  (define-fun |c_aux_v_#memory_int_84| () (Array Int (Array Int Int))
    (store (store ((as const (Array Int (Array Int Int)))
                ((as const (Array Int Int)) 164))
              42
              (_ as-array k!26))
       46
       (_ as-array k!25)))
  (define-fun |c_~#empty~0.base| () Int
    52)
  (define-fun |c_~#__CS_thread_status~0.base| () Int
    46)
  (define-fun |c_aux_v_#memory_$Pointer$.base_77| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!63))))
  (store (store a!1 54 (_ as-array k!36)) 52 (_ as-array k!47))))
  (define-fun |c_aux_v_#memory_$Pointer$.offset_84| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!57))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!66))))
  (define-fun |c_aux_v_#memory_$Pointer$.offset_76| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!19))
                  46
                  (_ as-array k!57))))
  (store (store a!1 54 (_ as-array k!17)) 52 (_ as-array k!16))))
  (define-fun |c_#memory_$Pointer$.base_primed| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!63))))
  (store (store a!1 54 (_ as-array k!71)) 52 (_ as-array k!47))))
  (define-fun |c_#memory_int| () (Array Int (Array Int Int))
    (let ((a!1 (store (store (store ((as const (Array Int Int)) 165) 4094293 250)
                         4091003
                         280)
                  156
                  263))
      (a!7 (store (store (store ((as const (Array Int Int)) 168) 4094293 130)
                         4094310
                         67)
                  4094284
                  298)))
(let ((a!2 (store (store (store (store a!1 4094299 132) 141 195) 4094308 227)
                  4094282
                  45))
      (a!8 (store (store (store (store a!7 4094299 140) 141 183) 4094277 316)
                  4094278
                  210)))
(let ((a!3 (store (store (store (store a!2 4094277 287) 159 317) 268 269)
                  242
                  243))
      (a!9 (store (store (store (store a!8 4094309 214) 4094290 186)
                         4094307
                         224)
                  4094285
                  229)))
(let ((a!4 (store (store (store (store a!3 4094307 131) 4094288 235) 162 286)
                  4094280
                  43)))
(let ((a!5 (store (store (store (store a!4 4094290 299) 4094278 275)
                         4094309
                         240)
                  92
                  290)))
(let ((a!6 (store (store (store (store a!5 4094289 309) 292 294) 4094283 178)
                  4094281
                  44)))
  (store (store ((as const (Array Int (Array Int Int)))
                  ((as const (Array Int Int)) 164))
                42
                (store a!6 320 321))
         46
         (store (store (store a!9 4094291 79) 4094283 133) 260 262)))))))))
  (define-fun |c_aux_v_#memory_$Pointer$.base_79| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!30))))
  (store (store a!1 54 (_ as-array k!36)) 52 (_ as-array k!47))))
  (define-fun |c_#memory_$Pointer$.offset| () (Array Int (Array Int Int))
    (let ((a!1 (store (store (store ((as const (Array Int Int)) 170) 4094311 302)
                         260
                         261)
                  272
                  273)))
(let ((a!2 (store (store (store (store a!1 4094275 223) 4094284 288)
                         4094308
                         193)
                  4094299
                  281)))
(let ((a!3 (store (store (store (store a!2 4094277 230) 4094278 188)
                         4094309
                         189)
                  4094290
                  276)))
(let ((a!4 (store (store (store (store a!3 4094307 126) 295 297) 4094285 90)
                  4094291
                  58)))
(let ((a!5 (store (store (store (store a!4 4094283 216) 4094286 57) 4094276 209)
                  323
                  324)))
(let ((a!6 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!19))
                  46
                  (store a!5 231 232))))
  (store (store a!6 54 (_ as-array k!17)) 52 (_ as-array k!16)))))))))
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp___CS_thread_lockedon~0#1.base| () Int
    54)
  (define-fun |c_~#num~0.offset| () Int
    4094273)
  (define-fun |c_~#__CS_thread_lockedon~0.base| () Int
    42)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp___CS_thread_status~0#1.offset| () Int
    4094277)
  (define-fun |c_~#num~0.base| () Int
    54)
  (define-fun |c_~#empty~0.offset| () Int
    4094277)
  (define-fun |c_old(~__CS_round~0)| () Int
    1)
  (define-fun |c_aux_v_#memory_$Pointer$.offset_83| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!57))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!16))))
  (define-fun |c_~#__CS_thread_allocated~0.base| () Int
    54)
  (define-fun |c_aux_v_#memory_$Pointer$.base_80| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!77))))
  (store (store a!1 54 (_ as-array k!36)) 52 (_ as-array k!47))))
  (define-fun |c_aux_v_#memory_$Pointer$.base_81| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!11))
                  46
                  (_ as-array k!14))))
  (store (store a!1 54 (_ as-array k!9)) 52 (_ as-array k!8))))
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp___CS_thread_status~0#1.base| () Int
    42)
  (define-fun |c_aux_v_#memory_$Pointer$.base_84| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!34))
                  46
                  (_ as-array k!30))))
  (store (store a!1 54 (_ as-array k!36)) 52 (_ as-array k!73))))
  (define-fun |c_~#m~0.offset| () Int
    4094283)
  (define-fun |c_aux_v_#memory_$Pointer$.base_82| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!11))
                  46
                  (_ as-array k!30))))
  (store (store a!1 54 (_ as-array k!9)) 52 (_ as-array k!8))))
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp___CS_thread_lockedon~0#1.offset| () Int
    4094266)
  (define-fun |c_#memory_$Pointer$.base| () (Array Int (Array Int Int))
    (let ((a!1 (store (store (store ((as const (Array Int Int)) 174) 4094307 217)
                         4094291
                         182)
                  4094311
                  301)))
(let ((a!2 (store (store (store (store a!1 4094310 305) 160 282) 4094284 236)
                  4094299
                  205)))
(let ((a!3 (store (store (store (store a!2 4094278 259) 4094309 194) 4094276 78)
                  4094286
                  75)))
(let ((a!4 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 163))
                         42
                         (_ as-array k!11))
                  46
                  (store (store a!3 4094290 74) 4094288 180))))
  (store (store a!4 54 (_ as-array k!9)) 52 (_ as-array k!8)))))))
  (define-fun |c_aux_v_#memory_int_83| () (Array Int (Array Int Int))
    (store (store ((as const (Array Int (Array Int Int)))
                ((as const (Array Int Int)) 164))
              42
              (_ as-array k!69))
       46
       (_ as-array k!25)))
  (define-fun |c_~#__CS_thread_status~0.offset| () Int
    4094286)
  (define-fun |c_~#full~0.offset| () Int
    4094308)
  (define-fun |c_old(~__CS_ret~0)| () Int
    0)
  (define-fun c_~__CS_ret~0 () Int
    0)
  (define-fun c_~__CS_thread~0.offset () (Array Int Int)
    ((as const (Array Int Int)) 0))
  (define-fun c_~__CS_thread~0.offset_primed () (Array Int Int)
    (store ((as const (Array Int Int)) 0) 0 2))
  (define-fun c_~__CS_thread_index~0_primed () Int
    0)
  (define-fun |#funAddr~thread1.base| () Int
    (- 1))
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_num~0#1.base| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_num~0#1.offset| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_m~0#1.base| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_m~0#1.offset| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_empty~0#1.base| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_empty~0#1.offset| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_full~0#1.base| () Int
    0)
  (define-fun |c_ULTIMATE.start_main_~#__CS_cp_full~0#1.offset| () Int
    0)
  (define-fun c_~__THREAD_RUNNING~0 () Int
    0)
  (define-fun |c_#memory_int_primed| () (Array Int (Array Int Int))
    (let ((a!1 (store (store (store ((as const (Array Int Int)) 165) 4094293 250)
                         4091003
                         280)
                  156
                  263))
      (a!8 (store (store (store ((as const (Array Int Int)) 168) 4094310 67)
                         4094293
                         130)
                  4094284
                  298)))
(let ((a!2 (store (store (store (store a!1 4094299 37) 4094308 227) 4094289 309)
                  4094278
                  275))
      (a!9 (store (store (store (store a!8 4094299 140) 4094277 316) 141 183)
                  4094278
                  210)))
(let ((a!3 (store (store (store (store a!2 4094309 240) 159 317) 4094290 299)
                  162
                  286))
      (a!10 (store (store (store (store a!9 4094290 44) 4094307 224)
                          4094285
                          229)
                   4094291
                   45)))
(let ((a!4 (store (store (store (store a!3 4094307 176) 4094288 235)
                         4094277
                         287)
                  4094291
                  129))
      (a!11 (store (store (store (store a!10 4094283 133) 260 262) 4094309 164)
                   4094289
                   0)))
(let ((a!5 (store (store (store (store a!4 292 294) 268 269) 141 195) 92 290)))
(let ((a!6 (store (store (store (store a!5 242 243) 4094282 45) 4094280 43)
                  4094283
                  178)))
(let ((a!7 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 164))
                         42
                         (store (store a!6 4094281 44) 320 321))
                  52
                  ((as const (Array Int Int)) 164))))
  (store (store a!7 46 (store a!11 4094286 0))
         54
         (store ((as const (Array Int Int)) 164) 4094277 1))))))))))
  (define-fun ~unnamed0~0~P_PID () Int
    1)
  (define-fun ~unnamed0~0~P_ALL () Int
    0)
  (define-fun ~__codecvt_result~0~__codecvt_noconv () Int
    3)
  (define-fun |#funAddr~main_thread.base| () Int
    (- 1))
  (define-fun ~__codecvt_result~0~__codecvt_ok () Int
    0)
  (define-fun |#funAddr~thread2.base| () Int
    (- 1))
  (define-fun |#funAddr~main_thread.offset| () Int
    2)
  (define-fun ~__codecvt_result~0~__codecvt_partial () Int
    1)
  (define-fun |#funAddr~thread1.offset| () Int
    0)
  (define-fun |#funAddr~thread2.offset| () Int
    1)
  (define-fun c_~__CS_thread~0.base () (Array Int Int)
    ((as const (Array Int Int)) 0))
  (define-fun c_~__CS_thread~0.base_primed () (Array Int Int)
    (store ((as const (Array Int Int)) 0) 0 (- 1)))
  (define-fun ~__codecvt_result~0~__codecvt_error () Int
    2)
  (define-fun ~unnamed0~0~P_PGID () Int
    2)
  (define-fun |c_ULTIMATE.start_main_#t~mem94#1_primed| () Int
    1)
  (define-fun c_~__CS_round~0_primed () Int
    0)
  (define-fun k!47 ((x!0 Int)) Int
    (ite (= x!0 100) 255
    (ite (= x!0 4094284) 56
    (ite (= x!0 4094308) 184
    (ite (= x!0 4094277) 291
    (ite (= x!0 4094289) 211
    (ite (= x!0 4094278) 105
    (ite (= x!0 4094299) 306
    (ite (= x!0 4094309) 264
    (ite (= x!0 4094290) 300
    (ite (= x!0 4094307) 218
    (ite (= x!0 295) 296
    (ite (= x!0 4094291) 303
    (ite (= x!0 98) 99
    (ite (= x!0 63) 199
    (ite (= x!0 4094276) 201
    (ite (= x!0 231) 233
      169)))))))))))))))))
  (define-fun k!26 ((x!0 Int)) Int
    (ite (= x!0 4094293) 250
    (ite (= x!0 4091003) 280
    (ite (= x!0 156) 263
    (ite (= x!0 4094299) 37
    (ite (= x!0 141) 195
    (ite (= x!0 4094289) 309
    (ite (= x!0 4094278) 275
    (ite (= x!0 4094309) 240
    (ite (= x!0 159) 317
    (ite (= x!0 4094290) 299
    (ite (= x!0 162) 286
    (ite (= x!0 4094288) 235
    (ite (= x!0 4094307) 131
    (ite (= x!0 4094308) 227
    (ite (= x!0 4094291) 129
    (ite (= x!0 292) 294
    (ite (= x!0 4094277) 287
    (ite (= x!0 242) 243
    (ite (= x!0 92) 290
    (ite (= x!0 268) 269
    (ite (= x!0 4094280) 43
    (ite (= x!0 4094282) 45
    (ite (= x!0 4094283) 178
    (ite (= x!0 4094281) 44
    (ite (= x!0 320) 321
      165))))))))))))))))))))))))))
  (define-fun k!19 ((x!0 Int)) Int
    (ite (= x!0 82) 312
    (ite (= x!0 4094299) 48
    (ite (= x!0 4094277) 249
    (ite (= x!0 4094289) 127
    (ite (= x!0 4094282) 81
    (ite (= x!0 4094309) 149
    (ite (= x!0 4094290) 71
    (ite (= x!0 4094288) 234
    (ite (= x!0 4094307) 121
    (ite (= x!0 138) 139
    (ite (= x!0 98) 207
    (ite (= x!0 4094292) 220
    (ite (= x!0 160) 161
    (ite (= x!0 4094279) 197
    (ite (= x!0 4094280) 248
    (ite (= x!0 4094332) 308
    (ite (= x!0 292) 293
    (ite (= x!0 150) 151
    (ite (= x!0 320) 322
      172))))))))))))))))))))
  (define-fun k!69 ((x!0 Int)) Int
    (ite (= x!0 4094293) 250
    (ite (= x!0 4091003) 280
    (ite (= x!0 156) 263
    (ite (= x!0 4094299) 37
    (ite (= x!0 4094308) 227
    (ite (= x!0 4094289) 309
    (ite (= x!0 4094278) 275
    (ite (= x!0 4094309) 240
    (ite (= x!0 159) 317
    (ite (= x!0 4094290) 299
    (ite (= x!0 162) 286
    (ite (= x!0 4094307) 176
    (ite (= x!0 4094288) 235
    (ite (= x!0 4094277) 287
    (ite (= x!0 4094291) 129
    (ite (= x!0 292) 294
    (ite (= x!0 268) 269
    (ite (= x!0 141) 195
    (ite (= x!0 92) 290
    (ite (= x!0 242) 243
    (ite (= x!0 4094282) 45
    (ite (= x!0 4094280) 43
    (ite (= x!0 4094283) 178
    (ite (= x!0 4094281) 44
    (ite (= x!0 320) 321
      165))))))))))))))))))))))))))
  (define-fun k!41 ((x!0 Int)) Int
    (ite (= x!0 4094293) 130
    (ite (= x!0 4094310) 67
    (ite (= x!0 4094284) 298
    (ite (= x!0 4094299) 140
    (ite (= x!0 141) 183
    (ite (= x!0 4094289) 43
    (ite (= x!0 4094278) 210
    (ite (= x!0 4094309) 214
    (ite (= x!0 4094277) 316
    (ite (= x!0 4094290) 186
    (ite (= x!0 4094307) 224
    (ite (= x!0 4094285) 229
    (ite (= x!0 4094291) 79
    (ite (= x!0 4094283) 133
    (ite (= x!0 260) 262
      168))))))))))))))))
  (define-fun k!34 ((x!0 Int)) Int
    (ite (= x!0 156) 157
    (ite (= x!0 4094284) 238
    (ite (= x!0 4094299) 38
    (ite (= x!0 4094308) 314
    (ite (= x!0 4094289) 148
    (ite (= x!0 4094278) 228
    (ite (= x!0 4094309) 158
    (ite (= x!0 159) 318
    (ite (= x!0 4094290) 72
    (ite (= x!0 162) 285
    (ite (= x!0 4094307) 39
    (ite (= x!0 4094291) 315
    (ite (= x!0 63) 200
    (ite (= x!0 4094283) 215
    (ite (= x!0 150) 219
    (ite (= x!0 4094281) 245
      166)))))))))))))))))
  (define-fun k!77 ((x!0 Int)) Int
    (ite (= x!0 4094311) 301
    (ite (= x!0 4094310) 305
    (ite (= x!0 4094284) 236
    (ite (= x!0 4094299) 205
    (ite (= x!0 4094289) 112
    (ite (= x!0 4094278) 259
    (ite (= x!0 4094309) 267
    (ite (= x!0 4094290) 35
    (ite (= x!0 4094307) 217
    (ite (= x!0 4094288) 180
    (ite (= x!0 4094291) 119
    (ite (= x!0 160) 282
    (ite (= x!0 4094276) 78
    (ite (= x!0 4094286) 75
      174)))))))))))))))
  (define-fun k!63 ((x!0 Int)) Int
    (ite (= x!0 4094311) 301
    (ite (= x!0 4094310) 305
    (ite (= x!0 4094284) 236
    (ite (= x!0 4094299) 205
    (ite (= x!0 4094289) 64
    (ite (= x!0 4094278) 259
    (ite (= x!0 4094309) 267
    (ite (= x!0 4094290) 35
    (ite (= x!0 4094307) 217
    (ite (= x!0 4094288) 180
    (ite (= x!0 4094291) 119
    (ite (= x!0 160) 282
    (ite (= x!0 4094276) 78
    (ite (= x!0 4094286) 75
      174)))))))))))))))
  (define-fun k!56 ((x!0 Int)) Int
    (ite (= x!0 82) 312
    (ite (= x!0 4094299) 48
    (ite (= x!0 4094277) 249
    (ite (= x!0 4094289) 127
    (ite (= x!0 4094282) 81
    (ite (= x!0 4094309) 149
    (ite (= x!0 4094290) 71
    (ite (= x!0 4094307) 121
    (ite (= x!0 4094279) 197
    (ite (= x!0 138) 139
    (ite (= x!0 4094291) 265
    (ite (= x!0 4094280) 248
    (ite (= x!0 160) 161
    (ite (= x!0 98) 207
    (ite (= x!0 4094292) 220
    (ite (= x!0 4094288) 234
    (ite (= x!0 292) 293
    (ite (= x!0 4094332) 308
    (ite (= x!0 150) 151
    (ite (= x!0 320) 322
      172)))))))))))))))))))))
  (define-fun k!49 ((x!0 Int)) Int
    (ite (= x!0 4091003) 279
    (ite (= x!0 82) 313
    (ite (= x!0 4094284) 53
    (ite (= x!0 4094308) 192
    (ite (= x!0 4094299) 241
    (ite (= x!0 4094289) 284
    (ite (= x!0 4094278) 51
    (ite (= x!0 4094282) 96
    (ite (= x!0 4094309) 310
    (ite (= x!0 4094290) 251
    (ite (= x!0 268) 270
    (ite (= x!0 4094307) 278
    (ite (= x!0 4094332) 307
    (ite (= x!0 4094285) 203
    (ite (= x!0 4094291) 311
    (ite (= x!0 4094280) 247
    (ite (= x!0 144) 226
      175))))))))))))))))))
  (define-fun k!14 ((x!0 Int)) Int
    (ite (= x!0 4094311) 301
    (ite (= x!0 4094310) 305
    (ite (= x!0 4094284) 236
    (ite (= x!0 4094299) 205
    (ite (= x!0 4094289) 112
    (ite (= x!0 4094278) 259
    (ite (= x!0 4094309) 194
    (ite (= x!0 4094290) 35
    (ite (= x!0 4094307) 217
    (ite (= x!0 4094288) 180
    (ite (= x!0 4094291) 182
    (ite (= x!0 160) 282
    (ite (= x!0 4094286) 75
    (ite (= x!0 4094276) 78
      174)))))))))))))))
  (define-fun k!71 ((x!0 Int)) Int
    (ite (= x!0 272) 274
    (ite (= x!0 256) 257
    (ite (= x!0 4094284) 237
    (ite (= x!0 4094299) 271
    (ite (= x!0 4094277) 55
    (ite (= x!0 4094289) 206
    (ite (= x!0 4094278) 179
    (ite (= x!0 4094309) 253
    (ite (= x!0 4094308) 185
    (ite (= x!0 4094290) 315
    (ite (= x!0 4094288) 181
    (ite (= x!0 4094307) 213
    (ite (= x!0 138) 283
    (ite (= x!0 4094291) 252
    (ite (= x!0 4094298) 38
    (ite (= x!0 144) 225
    (ite (= x!0 92) 289
    (ite (= x!0 4094306) 39
    (ite (= x!0 4094276) 202
    (ite (= x!0 4094281) 246
    (ite (= x!0 323) 325
      167))))))))))))))))))))))
  (define-fun k!57 ((x!0 Int)) Int
    (ite (= x!0 272) 273
    (ite (= x!0 4094311) 302
    (ite (= x!0 323) 324
    (ite (= x!0 4094275) 223
    (ite (= x!0 4094284) 288
    (ite (= x!0 4094308) 193
    (ite (= x!0 4094277) 230
    (ite (= x!0 4094289) 36
    (ite (= x!0 4094278) 188
    (ite (= x!0 4094309) 189
    (ite (= x!0 4094299) 281
    (ite (= x!0 4094290) 277
    (ite (= x!0 4094307) 126
    (ite (= x!0 295) 297
    (ite (= x!0 4094285) 90
    (ite (= x!0 4094291) 58
    (ite (= x!0 4094283) 216
    (ite (= x!0 4094286) 57
    (ite (= x!0 4094276) 209
    (ite (= x!0 231) 232
    (ite (= x!0 260) 261
      170))))))))))))))))))))))
  (define-fun k!50 ((x!0 Int)) Int
    (ite (= x!0 100) 254
    (ite (= x!0 4094275) 222
    (ite (= x!0 256) 258
    (ite (= x!0 4094299) 208
    (ite (= x!0 4094277) 50
    (ite (= x!0 4094289) 191
    (ite (= x!0 4094284) 304
    (ite (= x!0 4094309) 266
    (ite (= x!0 4094278) 319
    (ite (= x!0 242) 244
    (ite (= x!0 4094290) 265
    (ite (= x!0 4094307) 212
    (ite (= x!0 4094279) 198
    (ite (= x!0 4094285) 204
    (ite (= x!0 4094292) 221
    (ite (= x!0 4094291) 239
    (ite (= x!0 4094298) 48
    (ite (= x!0 4094306) 49
      173)))))))))))))))))))
  (define-fun k!43 ((x!0 Int)) Int
    (ite (= x!0 4094310) 67
    (ite (= x!0 4094293) 130
    (ite (= x!0 4094284) 298
    (ite (= x!0 4094299) 140
    (ite (= x!0 141) 183
    (ite (= x!0 4094289) 43
    (ite (= x!0 4094278) 210
    (ite (= x!0 4094309) 214
    (ite (= x!0 4094277) 316
    (ite (= x!0 4094290) 44
    (ite (= x!0 4094307) 224
    (ite (= x!0 4094285) 229
    (ite (= x!0 4094291) 79
    (ite (= x!0 4094283) 133
    (ite (= x!0 260) 262
      168))))))))))))))))
  (define-fun k!36 ((x!0 Int)) Int
    (ite (= x!0 272) 274
    (ite (= x!0 256) 257
    (ite (= x!0 4094284) 237
    (ite (= x!0 4094299) 271
    (ite (= x!0 4094277) 41
    (ite (= x!0 4094289) 206
    (ite (= x!0 4094278) 179
    (ite (= x!0 4094309) 253
    (ite (= x!0 4094308) 185
    (ite (= x!0 4094290) 315
    (ite (= x!0 4094288) 181
    (ite (= x!0 4094307) 213
    (ite (= x!0 138) 283
    (ite (= x!0 4094291) 252
    (ite (= x!0 4094298) 38
    (ite (= x!0 144) 225
    (ite (= x!0 92) 289
    (ite (= x!0 4094306) 39
    (ite (= x!0 4094276) 202
    (ite (= x!0 4094281) 246
    (ite (= x!0 323) 325
      167))))))))))))))))))))))
  (define-fun k!29 ((x!0 Int)) Int
    (ite (= x!0 156) 157
    (ite (= x!0 4094284) 238
    (ite (= x!0 4094299) 153
    (ite (= x!0 4094308) 314
    (ite (= x!0 4094289) 148
    (ite (= x!0 4094278) 228
    (ite (= x!0 4094309) 158
    (ite (= x!0 159) 318
    (ite (= x!0 4094290) 72
    (ite (= x!0 162) 285
    (ite (= x!0 4094307) 177
    (ite (= x!0 4094291) 315
    (ite (= x!0 63) 200
    (ite (= x!0 4094283) 215
    (ite (= x!0 150) 219
    (ite (= x!0 4094281) 245
      166)))))))))))))))))
  (define-fun k!22 ((x!0 Int)) Int
    (ite (= x!0 4094311) 302
    (ite (= x!0 272) 273
    (ite (= x!0 260) 261
    (ite (= x!0 4094275) 223
    (ite (= x!0 4094284) 288
    (ite (= x!0 4094308) 193
    (ite (= x!0 4094299) 281
    (ite (= x!0 4094289) 36
    (ite (= x!0 4094278) 188
    (ite (= x!0 4094309) 189
    (ite (= x!0 4094277) 230
    (ite (= x!0 4094290) 276
    (ite (= x!0 4094307) 126
    (ite (= x!0 295) 297
    (ite (= x!0 4094285) 90
    (ite (= x!0 4094291) 58
    (ite (= x!0 4094283) 216
    (ite (= x!0 4094286) 57
    (ite (= x!0 4094276) 209
    (ite (= x!0 231) 232
    (ite (= x!0 323) 324
      170))))))))))))))))))))))
  (define-fun k!8 ((x!0 Int)) Int
    (ite (= x!0 100) 255
    (ite (= x!0 4094308) 184
    (ite (= x!0 4094277) 291
    (ite (= x!0 4094289) 211
    (ite (= x!0 4094278) 196
    (ite (= x!0 4094309) 264
    (ite (= x!0 4094299) 306
    (ite (= x!0 4094290) 300
    (ite (= x!0 295) 296
    (ite (= x!0 4094307) 218
    (ite (= x!0 98) 99
    (ite (= x!0 4094291) 303
    (ite (= x!0 63) 199
    (ite (= x!0 4094276) 201
    (ite (= x!0 231) 233
      169))))))))))))))))
  (define-fun k!51 ((x!0 Int)) Int
    (ite (= x!0 272) 273
    (ite (= x!0 4094311) 302
    (ite (= x!0 323) 324
    (ite (= x!0 4094275) 223
    (ite (= x!0 4094284) 288
    (ite (= x!0 4094299) 281
    (ite (= x!0 4094277) 230
    (ite (= x!0 4094289) 36
    (ite (= x!0 4094278) 188
    (ite (= x!0 4094309) 190
    (ite (= x!0 4094308) 193
    (ite (= x!0 4094290) 277
    (ite (= x!0 295) 297
    (ite (= x!0 4094307) 126
    (ite (= x!0 4094285) 90
    (ite (= x!0 4094291) 58
    (ite (= x!0 4094283) 216
    (ite (= x!0 4094286) 57
    (ite (= x!0 260) 261
    (ite (= x!0 4094276) 209
    (ite (= x!0 231) 232
      170))))))))))))))))))))))
  (define-fun k!30 ((x!0 Int)) Int
    (ite (= x!0 4094311) 301
    (ite (= x!0 4094310) 305
    (ite (= x!0 4094284) 236
    (ite (= x!0 4094299) 205
    (ite (= x!0 4094289) 112
    (ite (= x!0 4094278) 259
    (ite (= x!0 4094309) 194
    (ite (= x!0 4094290) 35
    (ite (= x!0 4094307) 217
    (ite (= x!0 4094288) 180
    (ite (= x!0 4094291) 119
    (ite (= x!0 160) 282
    (ite (= x!0 4094276) 78
    (ite (= x!0 4094286) 75
      174)))))))))))))))
  (define-fun k!16 ((x!0 Int)) Int
    (ite (= x!0 4091003) 279
    (ite (= x!0 82) 313
    (ite (= x!0 4094299) 241
    (ite (= x!0 4094308) 192
    (ite (= x!0 4094289) 284
    (ite (= x!0 4094278) 187
    (ite (= x!0 4094282) 96
    (ite (= x!0 4094309) 310
    (ite (= x!0 268) 270
    (ite (= x!0 4094290) 251
    (ite (= x!0 4094307) 278
    (ite (= x!0 4094332) 307
    (ite (= x!0 4094285) 203
    (ite (= x!0 4094280) 247
    (ite (= x!0 4094291) 311
    (ite (= x!0 144) 226
      175)))))))))))))))))
  (define-fun k!9 ((x!0 Int)) Int
    (ite (= x!0 272) 274
    (ite (= x!0 256) 257
    (ite (= x!0 4094284) 237
    (ite (= x!0 4094299) 271
    (ite (= x!0 4094308) 185
    (ite (= x!0 4094289) 206
    (ite (= x!0 4094278) 179
    (ite (= x!0 4094309) 253
    (ite (= x!0 4094290) 315
    (ite (= x!0 4094307) 213
    (ite (= x!0 4094288) 181
    (ite (= x!0 138) 283
    (ite (= x!0 4094291) 252
    (ite (= x!0 4094298) 38
    (ite (= x!0 144) 225
    (ite (= x!0 92) 289
    (ite (= x!0 4094306) 39
    (ite (= x!0 4094276) 202
    (ite (= x!0 4094281) 246
    (ite (= x!0 323) 325
      167)))))))))))))))))))))
  (define-fun k!66 ((x!0 Int)) Int
    (ite (= x!0 4091003) 279
    (ite (= x!0 82) 313
    (ite (= x!0 4094284) 53
    (ite (= x!0 4094308) 192
    (ite (= x!0 4094299) 241
    (ite (= x!0 4094289) 284
    (ite (= x!0 4094282) 96
    (ite (= x!0 4094278) 187
    (ite (= x!0 4094309) 310
    (ite (= x!0 268) 270
    (ite (= x!0 4094290) 251
    (ite (= x!0 4094332) 307
    (ite (= x!0 4094307) 278
    (ite (= x!0 4094285) 203
    (ite (= x!0 4094280) 247
    (ite (= x!0 4094291) 311
    (ite (= x!0 144) 226
      175))))))))))))))))))
  (define-fun k!73 ((x!0 Int)) Int
    (ite (= x!0 100) 255
    (ite (= x!0 4094284) 56
    (ite (= x!0 4094308) 184
    (ite (= x!0 4094277) 291
    (ite (= x!0 4094289) 211
    (ite (= x!0 4094278) 196
    (ite (= x!0 4094309) 264
    (ite (= x!0 4094299) 306
    (ite (= x!0 4094290) 300
    (ite (= x!0 295) 296
    (ite (= x!0 4094307) 218
    (ite (= x!0 98) 99
    (ite (= x!0 4094291) 303
    (ite (= x!0 63) 199
    (ite (= x!0 4094276) 201
    (ite (= x!0 231) 233
      169)))))))))))))))))
  (define-fun k!52 ((x!0 Int)) Int
    (ite (= x!0 82) 312
    (ite (= x!0 4094299) 48
    (ite (= x!0 4094277) 249
    (ite (= x!0 4094289) 127
    (ite (= x!0 4094282) 81
    (ite (= x!0 4094309) 149
    (ite (= x!0 4094290) 71
    (ite (= x!0 4094307) 49
    (ite (= x!0 4094332) 308
    (ite (= x!0 138) 139
    (ite (= x!0 4094292) 220
    (ite (= x!0 292) 293
    (ite (= x!0 4094291) 265
    (ite (= x!0 4094279) 197
    (ite (= x!0 98) 207
    (ite (= x!0 4094288) 234
    (ite (= x!0 4094280) 248
    (ite (= x!0 160) 161
    (ite (= x!0 150) 151
    (ite (= x!0 320) 322
      172)))))))))))))))))))))
  (define-fun k!24 ((x!0 Int)) Int
    (ite (= x!0 4094293) 250
    (ite (= x!0 4091003) 280
    (ite (= x!0 156) 263
    (ite (= x!0 4094308) 227
    (ite (= x!0 141) 195
    (ite (= x!0 4094299) 132
    (ite (= x!0 4094277) 287
    (ite (= x!0 4094289) 309
    (ite (= x!0 159) 317
    (ite (= x!0 242) 243
    (ite (= x!0 162) 286
    (ite (= x!0 268) 269
    (ite (= x!0 4094288) 235
    (ite (= x!0 4094307) 131
    (ite (= x!0 4094291) 129
    (ite (= x!0 292) 294
    (ite (= x!0 4094290) 299
    (ite (= x!0 4094278) 275
    (ite (= x!0 92) 290
    (ite (= x!0 4094309) 240
    (ite (= x!0 4094280) 43
    (ite (= x!0 4094282) 45
    (ite (= x!0 4094283) 178
    (ite (= x!0 4094281) 44
    (ite (= x!0 320) 321
      165))))))))))))))))))))))))))
  (define-fun k!17 ((x!0 Int)) Int
    (ite (= x!0 100) 254
    (ite (= x!0 4094275) 222
    (ite (= x!0 256) 258
    (ite (= x!0 4094299) 208
    (ite (= x!0 4094284) 304
    (ite (= x!0 4094289) 191
    (ite (= x!0 4094278) 319
    (ite (= x!0 4094309) 266
    (ite (= x!0 242) 244
    (ite (= x!0 4094290) 265
    (ite (= x!0 4094307) 212
    (ite (= x!0 4094279) 198
    (ite (= x!0 4094285) 204
    (ite (= x!0 4094292) 221
    (ite (= x!0 4094291) 239
    (ite (= x!0 4094298) 48
    (ite (= x!0 4094306) 49
      173))))))))))))))))))
  (define-fun k!10 ((x!0 Int)) Int
    (ite (= x!0 4094311) 301
    (ite (= x!0 4094310) 305
    (ite (= x!0 4094284) 236
    (ite (= x!0 4094299) 205
    (ite (= x!0 4094289) 112
    (ite (= x!0 4094278) 259
    (ite (= x!0 4094309) 194
    (ite (= x!0 4094290) 74
    (ite (= x!0 4094307) 217
    (ite (= x!0 4094288) 180
    (ite (= x!0 4094291) 182
    (ite (= x!0 160) 282
    (ite (= x!0 4094286) 75
    (ite (= x!0 4094276) 78
      174)))))))))))))))
  (define-fun k!32 ((x!0 Int)) Int
    (ite (= x!0 156) 157
    (ite (= x!0 4094284) 238
    (ite (= x!0 4094299) 38
    (ite (= x!0 4094308) 314
    (ite (= x!0 4094289) 148
    (ite (= x!0 4094278) 228
    (ite (= x!0 4094309) 158
    (ite (= x!0 159) 318
    (ite (= x!0 4094290) 72
    (ite (= x!0 162) 285
    (ite (= x!0 4094307) 177
    (ite (= x!0 4094291) 315
    (ite (= x!0 63) 200
    (ite (= x!0 4094283) 215
    (ite (= x!0 150) 219
    (ite (= x!0 4094281) 245
      166)))))))))))))))))
  (define-fun k!25 ((x!0 Int)) Int
    (ite (= x!0 4094310) 67
    (ite (= x!0 4094293) 130
    (ite (= x!0 4094284) 298
    (ite (= x!0 4094299) 140
    (ite (= x!0 4094277) 316
    (ite (= x!0 4094289) 43
    (ite (= x!0 141) 183
    (ite (= x!0 4094309) 214
    (ite (= x!0 4094278) 210
    (ite (= x!0 4094290) 44
    (ite (= x!0 4094307) 224
    (ite (= x!0 4094285) 229
    (ite (= x!0 4094291) 45
    (ite (= x!0 4094283) 133
    (ite (= x!0 260) 262
      168))))))))))))))))
  (define-fun k!11 ((x!0 Int)) Int
    (ite (= x!0 156) 157
    (ite (= x!0 4094284) 238
    (ite (= x!0 4094308) 314
    (ite (= x!0 4094299) 153
    (ite (= x!0 4094289) 148
    (ite (= x!0 4094278) 228
    (ite (= x!0 4094309) 158
    (ite (= x!0 159) 318
    (ite (= x!0 4094290) 72
    (ite (= x!0 162) 285
    (ite (= x!0 4094307) 177
    (ite (= x!0 63) 200
    (ite (= x!0 4094283) 215
    (ite (= x!0 150) 219
    (ite (= x!0 4094281) 245
      166))))))))))))))))
  (define-fun k!54 ((x!0 Int)) Int
    (ite (= x!0 272) 273
    (ite (= x!0 4094311) 302
    (ite (= x!0 323) 324
    (ite (= x!0 4094275) 223
    (ite (= x!0 4094284) 288
    (ite (= x!0 4094299) 281
    (ite (= x!0 4094277) 230
    (ite (= x!0 4094289) 47
    (ite (= x!0 4094278) 188
    (ite (= x!0 4094309) 190
    (ite (= x!0 4094308) 193
    (ite (= x!0 4094290) 277
    (ite (= x!0 4094307) 126
    (ite (= x!0 295) 297
    (ite (= x!0 4094285) 90
    (ite (= x!0 4094291) 58
    (ite (= x!0 4094283) 216
    (ite (= x!0 4094286) 57
    (ite (= x!0 260) 261
    (ite (= x!0 4094276) 209
    (ite (= x!0 231) 232
      170))))))))))))))))))))))
)
  )";

    // Test case from smtrat: symbols starting with invalid characters like '('
    // These symbols need to be wrapped with |...| in define-fun/declare-fun declarations
    std::string model_str4_smtrat = R"(
(model 
        (define-fun (38,19)!131 () Real
                1
        )
        (define-fun (72,19)!72 () Real
                1
        )
        (define-fun (70,19)!73 () Real
                1
        )
        (define-fun (63,19)!117 () Real
                0
        )
)
)";

    // Basic interface: parse model from string then evaluate(phi, M)
    {
        parser->mkVarInt("x");
        parser->mkVarInt("y");
        auto phi = parser->mkExpr("(and (> x 0) (> y 0))");
        VERIFY(phi);
        std::string modelStr = R"(
(model
  (define-fun x () Int 1)
  (define-fun y () Int 2)
))";
        auto M = parser->parseModel(modelStr);
        VERIFY(M && M->size() >= 2);
        auto psi = parser->evaluate(phi, M);
        VERIFY(psi && psi->isTrue() && "evaluate((and (> x 0) (> y 0)), M) with x=1,y=2 should be true");
        std::cout << "Basic parseModel + evaluate: phi=(and (> x 0) (> y 0)), M={x->1,y->2} => " << parser->toString(psi) << " OK" << std::endl;
    }

    try {
        auto model = parser->parseModel(model_str1);
        VERIFY(model && "model1 should parse successfully");
        std::cout << "Model 1 parsed successfully!" << std::endl;
        std::cout << "Model 1 size: " << model->size() << std::endl;
        VERIFY(model->size() > 0);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
        VERIFY(pairs.size() == model->size());
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        VERIFY(false && "model1 should not throw");
    }

    try {
        auto model = parser->parseModel(model_str2);
        VERIFY(model && "model2 should parse successfully");
        std::cout << "Model 2 parsed successfully!" << std::endl;
        std::cout << "Model 2 size: " << model->size() << std::endl;
        VERIFY(model->size() > 0);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        VERIFY(false && "model2 should not throw");
    }

    try {
        auto model = parser->parseModel(model_str3_simple);
        VERIFY(model && "model3 (simple as-array) should parse successfully");
        std::cout << "Model 3 (simplified) parsed successfully!" << std::endl;
        std::cout << "Model 3 size: " << model->size() << std::endl;
        VERIFY(model->size() >= 1);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        VERIFY(false && "model3 should not throw");
    }

    try {
        auto model = parser->parseModel(model_str4_smtrat);
        VERIFY(model && "model4 (smtrat) should parse successfully");
        std::cout << "Model 4 (smtrat) parsed successfully!" << std::endl;
        std::cout << "Model 4 size: " << model->size() << std::endl;
        VERIFY(model->size() >= 3);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        VERIFY(false && "model4 should not throw");
    }

    try {
        // Parse model 5 - CVC5 real_algebraic_number format test
        std::string model_str5_cvc5 = R"(
(model
  (define-fun x () Real
    (_ real_algebraic_number <(+ (* 1 (^ x 2)) (- 3)), ((/ 3 2), (/ 7 4))>))
)";
        
        auto model = parser->parseModel(model_str5_cvc5);
        
        VERIFY(model && "model5 (CVC5 real_algebraic_number) should parse successfully");
        std::cout << "Model 5 (CVC5 real_algebraic_number) parsed successfully!" << std::endl;
        std::cout << "Model 5 size: " << model->size() << std::endl;
        VERIFY(model->size() == 1);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
            if (pair.second->isCRealAlgebraicNumber()) {
                std::cout << "  -> This is a real_algebraic_number node" << std::endl;
                std::cout << "  -> Polynomial: " << parser->toString(pair.second->getChild(0)) << std::endl;
                std::cout << "  -> Lower bound: " << parser->toString(pair.second->getChild(1)) << std::endl;
                std::cout << "  -> Upper bound: " << parser->toString(pair.second->getChild(2)) << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        VERIFY(false && "model5 should not throw");
    }
    return 0;
}
