(module
  (func $f_sign (param f32) (result f32)
    local.get 0
    f32.const 0x0p+0 (;=0;)
    f32.gt
    if
      f32.const 0x1p+0 (;=1;)
      return
    end
    f32.const 0x0p+0 (;=0;)
    return
  )

  (export "f_sign" (func $f_sign )))
