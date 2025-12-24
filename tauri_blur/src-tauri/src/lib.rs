// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/

mod blur_ffi;

use std::sync::atomic::{AtomicBool, Ordering};
use tauri::Window;

static BLUR_INITIALIZED: AtomicBool = AtomicBool::new(false);

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

/// Initialize blur library
#[tauri::command]
fn blur_init_lib() -> Result<u32, String> {
    #[cfg(windows)]
    {
        if BLUR_INITIALIZED.load(Ordering::SeqCst) {
            return Err("Already initialized".to_string());
        }

        let caps = blur_ffi::init()?;
        BLUR_INITIALIZED.store(true, Ordering::SeqCst);
        Ok(caps)
    }

    #[cfg(not(windows))]
    Err("Only supported on Windows".to_string())
}

/// Apply blur to the window
#[tauri::command]
fn apply_blur(window: Window, intensity: f32, color: u32) -> Result<(), String> {
    #[cfg(windows)]
    {
        if !BLUR_INITIALIZED.load(Ordering::SeqCst) {
            return Err("Library not initialized".to_string());
        }

        let hwnd = window.hwnd().map_err(|e| e.to_string())?;
        blur_ffi::apply_blur(hwnd.0 as usize, intensity, color)
    }

    #[cfg(not(windows))]
    Err("Only supported on Windows".to_string())
}

/// Clear blur from the window
#[tauri::command]
fn clear_blur(window: Window) -> Result<(), String> {
    #[cfg(windows)]
    {
        if !BLUR_INITIALIZED.load(Ordering::SeqCst) {
            return Err("Library not initialized".to_string());
        }

        let hwnd = window.hwnd().map_err(|e| e.to_string())?;
        blur_ffi::clear_blur(hwnd.0 as usize)
    }

    #[cfg(not(windows))]
    Err("Only supported on Windows".to_string())
}

/// Get blur library version
#[tauri::command]
fn get_blur_version() -> String {
    #[cfg(windows)]
    {
        blur_ffi::get_version()
    }

    #[cfg(not(windows))]
    "N/A".to_string()
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            greet,
            blur_init_lib,
            apply_blur,
            clear_blur,
            get_blur_version
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
