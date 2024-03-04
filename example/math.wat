(module
  (func $f_inv_sqrt (param $x f32) (result f32)
    (;
    (local $conv f32)

    i32.const 0x5F3759DF

    local.get $x
    i32.reinterpret_f32
    i32.const 1
    i32.shr_u
    i32.sub

    f32.reinterpret_i32
    local.set $conv

    f32.const 1.5

    local.get $conv
    local.get $conv
    f32.mul

    local.get $x
    f32.const 0.5
    f32.mul

    f32.mul

    f32.sub
    ;)

    f32.const 1
    local.get $x
    f32.sqrt
    f32.div
    return
  )

  (func $vec3f_len2 (param $x f32) (param $y f32) (param $z f32) (result f32)
    local.get $x
    local.get $x
    f32.mul

    local.get $y
    local.get $y
    f32.mul
    f32.add

    local.get $z
    local.get $z
    f32.mul
    f32.add
    return
  )

  (func $vec3f_len (param $x f32) (param $y f32) (param $z f32) (result f32)
    local.get $x
    local.get $y
    local.get $z
    call $vec3f_len2
    f32.sqrt
    return
  )

  (export "vec3f_len2" (func $vec3f_len2))
  (export "vec3f_len" (func $vec3f_len))
  (export "f_inv_sqrt" (func $f_inv_sqrt))
)
