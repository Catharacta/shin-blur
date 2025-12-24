import { invoke } from "@tauri-apps/api/core";

let statusEl: HTMLElement | null;
let versionEl: HTMLElement | null;
let capsEl: HTMLElement | null;
let intensitySlider: HTMLInputElement | null;
let colorInput: HTMLInputElement | null;

async function initBlur() {
  try {
    const caps = await invoke<number>("blur_init_lib");
    if (capsEl) {
      capsEl.textContent = `Capabilities: 0x${caps.toString(16).padStart(4, '0')}`;
    }
    updateStatus("Library initialized");
    
    // Get version
    const version = await invoke<string>("get_blur_version");
    if (versionEl) {
      versionEl.textContent = `blur_lib v${version}`;
    }
  } catch (e) {
    updateStatus(`Error: ${e}`);
  }
}

async function applyBlur() {
  const intensity = intensitySlider ? parseFloat(intensitySlider.value) / 100 : 1.0;
  const colorHex = colorInput?.value || "80000000";
  const color = parseInt(colorHex, 16);
  
  try {
    await invoke("apply_blur", { intensity, color });
    updateStatus("Blur applied!");
  } catch (e) {
    updateStatus(`Error: ${e}`);
  }
}

async function clearBlur() {
  try {
    await invoke("clear_blur");
    updateStatus("Blur cleared");
  } catch (e) {
    updateStatus(`Error: ${e}`);
  }
}

function updateStatus(msg: string) {
  if (statusEl) {
    statusEl.textContent = msg;
  }
}

window.addEventListener("DOMContentLoaded", () => {
  statusEl = document.querySelector("#status");
  versionEl = document.querySelector("#version");
  capsEl = document.querySelector("#caps");
  intensitySlider = document.querySelector("#intensity");
  colorInput = document.querySelector("#color");
  
  document.querySelector("#init-btn")?.addEventListener("click", initBlur);
  document.querySelector("#apply-btn")?.addEventListener("click", applyBlur);
  document.querySelector("#clear-btn")?.addEventListener("click", clearBlur);
  
  // Update intensity label
  intensitySlider?.addEventListener("input", () => {
    const label = document.querySelector("#intensity-label");
    if (label && intensitySlider) {
      label.textContent = `${intensitySlider.value}%`;
    }
  });
});
