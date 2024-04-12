(module
  (func $sqr (param $x i32)
    i64.const 30
    i32.wrap_i64
    local.set $x
    return
  )

  (export "sqr" (func $sqr))
)