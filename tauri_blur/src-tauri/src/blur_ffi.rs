/*
 * blur_ffi.rs - FFI bindings for blur_lib.dll
 */

use std::ffi::{c_char, CStr};
use std::ptr;

/// Error codes from blur_lib
pub const BLUR_SUCCESS: i32 = 0;
pub const BLUR_NOT_INITIALIZED: i32 = 1;
pub const BLUR_INVALID_HANDLE: i32 = 2;
pub const BLUR_ALREADY_APPLIED: i32 = 9;

/// Capability bits
pub const BLUR_CAP_SETWINDOWCOMPOSITION: u32 = 0x0001;
pub const BLUR_CAP_DWM_BLUR: u32 = 0x0002;
pub const BLUR_CAP_COLOR_CONTROL: u32 = 0x0008;

/// EffectParams structure matching C definition
#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct EffectParams {
    pub struct_version: u32,
    pub intensity: f32,
    pub color_argb: u32,
    pub animate: u8,
    pub animation_ms: u32,
    pub reserved_flags: u32,
    pub reserved_padding: [u8; 4],
}

impl Default for EffectParams {
    fn default() -> Self {
        Self {
            struct_version: 1,
            intensity: 1.0,
            color_argb: 0x80000000,
            animate: 0,
            animation_ms: 0,
            reserved_flags: 0,
            reserved_padding: [0; 4],
        }
    }
}

#[cfg(windows)]
#[link(name = "blur_lib")]
extern "C" {
    pub fn blur_init(capabilities: *mut u32) -> i32;
    pub fn blur_shutdown();
    pub fn blur_apply_to_window(
        window_handle: usize,
        params: *const EffectParams,
        timeout_ms: u32,
    ) -> i32;
    pub fn blur_clear_from_window(window_handle: usize, timeout_ms: u32) -> i32;
    pub fn blur_restore_all() -> i32;
    pub fn blur_get_version(out_utf8: *mut *mut c_char) -> i32;
    pub fn blur_free_string(ptr: *mut c_char);
    pub fn blur_get_last_error(out_utf8: *mut *mut c_char) -> i32;
}

/// Safe wrapper for blur_lib initialization
#[cfg(windows)]
pub fn init() -> Result<u32, String> {
    unsafe {
        let mut caps: u32 = 0;
        let result = blur_init(&mut caps);
        if result == BLUR_SUCCESS {
            Ok(caps)
        } else {
            Err(format!("blur_init failed with code {}", result))
        }
    }
}

/// Safe wrapper for shutdown
#[cfg(windows)]
pub fn shutdown() {
    unsafe {
        blur_shutdown();
    }
}

/// Safe wrapper for apply_blur_to_window
#[cfg(windows)]
pub fn apply_blur(hwnd: usize, intensity: f32, color: u32) -> Result<(), String> {
    let params = EffectParams {
        struct_version: 1,
        intensity,
        color_argb: color,
        ..Default::default()
    };
    
    unsafe {
        let result = blur_apply_to_window(hwnd, &params, 0);
        if result == BLUR_SUCCESS {
            Ok(())
        } else if result == BLUR_ALREADY_APPLIED {
            Ok(()) // Already applied is not an error
        } else {
            Err(get_last_error_message())
        }
    }
}

/// Safe wrapper for clear_blur_from_window
#[cfg(windows)]
pub fn clear_blur(hwnd: usize) -> Result<(), String> {
    unsafe {
        let result = blur_clear_from_window(hwnd, 0);
        if result == BLUR_SUCCESS {
            Ok(())
        } else {
            Err(get_last_error_message())
        }
    }
}

/// Get last error message
#[cfg(windows)]
fn get_last_error_message() -> String {
    unsafe {
        let mut ptr: *mut c_char = ptr::null_mut();
        if blur_get_last_error(&mut ptr) == BLUR_SUCCESS && !ptr.is_null() {
            let msg = CStr::from_ptr(ptr).to_string_lossy().into_owned();
            blur_free_string(ptr);
            msg
        } else {
            "Unknown error".to_string()
        }
    }
}

/// Get library version
#[cfg(windows)]
pub fn get_version() -> String {
    unsafe {
        let mut ptr: *mut c_char = ptr::null_mut();
        if blur_get_version(&mut ptr) == BLUR_SUCCESS && !ptr.is_null() {
            let version = CStr::from_ptr(ptr).to_string_lossy().into_owned();
            blur_free_string(ptr);
            version
        } else {
            "unknown".to_string()
        }
    }
}
