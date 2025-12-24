import { invoke } from "@tauri-apps/api/core";
import { getCurrentWindow } from "@tauri-apps/api/window";

const appWindow = getCurrentWindow();

window.addEventListener("DOMContentLoaded", () => {
  const versionEl = document.getElementById("version");
  const capsEl = document.getElementById("caps");
  const statusEl = document.getElementById("status");
  const intensityInput = document.getElementById("intensity") as HTMLInputElement;
  const intensityLabel = document.getElementById("intensity-label");
  const colorInput = document.getElementById("color") as HTMLInputElement;
  const initBtn = document.getElementById("init-btn");
  const applyBtn = document.getElementById("apply-btn");
  const clearBtn = document.getElementById("clear-btn");
  const closeBtn = document.getElementById("close-btn");

  if (!versionEl || !capsEl || !statusEl || !intensityInput || !intensityLabel || !colorInput || !initBtn || !applyBtn || !clearBtn || !closeBtn) {
    return;
  }

  // Close app
  closeBtn.addEventListener("click", () => {
    appWindow.close();
  });

  // Update labels
  intensityInput.addEventListener("input", () => {
    intensityLabel.textContent = `${intensityInput.value}%`;
  });

  // Init Library
  initBtn.addEventListener("click", async () => {
    try {
      statusEl.textContent = "Initializing...";
      const caps = await invoke<number>("blur_init_lib");
      const version = await invoke<string>("get_blur_version");

      versionEl.textContent = version;
      capsEl.textContent = `0x${caps.toString(16).toUpperCase()}`;
      statusEl.textContent = "Library Initialized";
      initBtn.setAttribute("disabled", "true");
    } catch (e) {
      statusEl.textContent = `Error: ${e}`;
    }
  });

  // Apply Blur
  applyBtn.addEventListener("click", async () => {
    try {
      const intensity = parseFloat(intensityInput.value) / 100.0;
      const colorText = colorInput.value;
      const color = parseInt(colorText, 16);

      statusEl.textContent = "Applying Blur...";
      await invoke("apply_blur", { intensity, color });
      statusEl.textContent = "Blur Applied";
    } catch (e) {
      statusEl.textContent = `Error: ${e}`;
    }
  });

  // Clear Blur
  clearBtn.addEventListener("click", async () => {
    try {
      statusEl.textContent = "Clearing Blur...";
      await invoke("clear_blur");
      statusEl.textContent = "Blur Cleared";
    } catch (e) {
      statusEl.textContent = `Error: ${e}`;
    }
  });
});
