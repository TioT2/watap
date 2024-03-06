(module
  (func $sqr (param $x f32) (result f64)
    ref.null extern
    drop
    local.get $x
    f64.promote_f32
    return
  )

  (export "sqr" (func $sqr))
)