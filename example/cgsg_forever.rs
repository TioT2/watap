#[repr(C)]
struct OptI32 {
  pub value: i32,
  pub is_some: u8,
}

extern "C" {
  fn get_right_value() -> OptI32;
}

pub static mut RESERVE_RIGHT_VALUE: i32 = 30;

fn main() {
  let right_value_opt = unsafe { get_right_value() };

  println!("CGSG FOREVER!!! {}!!!", if right_value_opt.is_some == 1 {
    right_value_opt.value
  } else {
    unsafe { RESERVE_RIGHT_VALUE }
  });
}
