(set-logic QF_S)

(declare-fun var_abc () String)

(assert (= var_abc (indexOf /abc/ "b")))

(check-sat)

