extern crate cbindgen;

use std::path::PathBuf;

fn main() {
  let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
  let output_file = PathBuf::from(&crate_dir)
    .join("include")
    .join("dhc")
    .join("dhc.h");

  cbindgen::generate(crate_dir)
    .expect("Unable to generate bindings")
    .write_to_file(output_file);
}