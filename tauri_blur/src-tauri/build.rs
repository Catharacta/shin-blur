fn main() {
    // Add library search path for blur_lib
    println!(
        "cargo:rustc-link-search=native={}",
        std::env::current_dir().unwrap().display()
    );

    tauri_build::build()
}
