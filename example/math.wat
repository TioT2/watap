(module
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
)
