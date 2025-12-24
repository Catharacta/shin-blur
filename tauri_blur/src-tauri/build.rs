fn main() {
    let project_root = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let blur_lib_dir = std::path::Path::new(&project_root)
        .parent()
        .unwrap()
        .join("blur_lib")
        .join("build")
        .join("Release");

    println!("cargo:rustc-link-search=native={}", blur_lib_dir.display());
    println!("cargo:rustc-link-lib=blur_lib");

    // Copy DLL to output directory
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let target_dir = std::path::Path::new(&out_dir)
        .parent()
        .unwrap()
        .parent()
        .unwrap()
        .parent()
        .unwrap();

    let dll_src = blur_lib_dir.join("blur_lib.dll");
    let dll_dst = target_dir.join("blur_lib.dll");

    if dll_src.exists() {
        std::fs::copy(&dll_src, &dll_dst).ok();
        println!("cargo:rerun-if-changed={}", dll_src.display());
    }

    tauri_build::build();
}
