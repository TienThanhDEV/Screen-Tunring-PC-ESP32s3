"use strict";

const state = {
  token: "",
  connected: false,
  demo: true,
  dirty: new Set(),
  repo: { owner: "TienThanhDEV", name: "Screen-Tunring-PC-ESP32s3", branch: "main", folder: "docs/data" },
  effects: { schemaVersion: 1, updatedAt: new Date().toISOString(), effects: [] },
  firmware: {},
  devices: { devices: [] },
  docs: { documents: [] },
  selectedEffect: null
};

const $ = (selector, root = document) => root.querySelector(selector);
const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
const titles = {
  overview: ["CONTROL CENTER", "Tổng quan hệ thống"],
  effects: ["VISUAL SYSTEM", "Thư viện hiệu ứng"],
  firmware: ["RELEASE CENTER", "Firmware & OTA"],
  devices: ["FLEET", "Quản lý thiết bị"],
  docs: ["KNOWLEDGE BASE", "Tài liệu kỹ thuật"],
  settings: ["SECURITY", "Kết nối GitHub"]
};

function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>'"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", "'": "&#39;", '"': "&quot;" }[c]));
}

function showToast(message, type = "success") {
  const node = document.createElement("div");
  node.className = `toast ${type}`;
  node.textContent = message;
  $("#toastStack").append(node);
  setTimeout(() => node.remove(), 3600);
}

function setTab(name) {
  $$(".nav-item").forEach(node => node.classList.toggle("active", node.dataset.tab === name));
  $$(".tab-panel").forEach(node => node.classList.toggle("active", node.id === `tab-${name}`));
  $("#sectionEyebrow").textContent = titles[name][0];
  $("#sectionTitle").textContent = titles[name][1];
  if (window.innerWidth < 600) window.scrollTo({ top: 0, behavior: "smooth" });
}

async function fetchJson(path, fallback) {
  try {
    const response = await fetch(path, { cache: "no-store" });
    if (!response.ok) throw new Error(`${response.status}`);
    return await response.json();
  } catch (error) {
    console.warn(`Không tải được ${path}`, error);
    return fallback;
  }
}

async function loadData() {
  const [effects, firmware, devices, docs] = await Promise.all([
    fetchJson("data/effects.json", state.effects),
    fetchJson("data/firmware-manifest.json", state.firmware),
    fetchJson("data/devices.json", state.devices),
    fetchJson("data/docs.json", state.docs)
  ]);
  state.effects = effects;
  state.firmware = firmware;
  state.devices = devices;
  state.docs = docs;
  renderAll();
}

function renderAll() {
  renderEffects();
  renderFirmware();
  renderDevices();
  renderDocs();
  $("#effectCount").textContent = state.effects.effects.length;
  $("#deviceCount").textContent = state.devices.devices.length;
  $("#currentVersion").textContent = `v${state.firmware.latestVersion || "—"}`;
  $("#firmwareSize").textContent = formatBytes(state.firmware.size || 0);
  $("#manifestTime").textContent = `cập nhật ${new Date().toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit" })}`;
}

function renderEffects() {
  const list = $("#effectList");
  list.innerHTML = state.effects.effects.map(effect => {
    const colors = effect.palette?.length ? effect.palette : ["#47e3ad", "#70a5ff"];
    return `<button class="effect-card ${state.selectedEffect === effect.id ? "active" : ""}" data-effect="${escapeHtml(effect.id)}"><span class="effect-swatch" style="--c1:${escapeHtml(colors[0])};--c2:${escapeHtml(colors[1] || colors[0])};--c3:${escapeHtml(colors[2] || colors[0])}"></span><div><strong>${escapeHtml(effect.name)}</strong><small>${escapeHtml(effect.type)} · ${effect.brightness}%</small></div><i class="effect-state ${effect.enabled ? "on" : ""}"></i></button>`;
  }).join("");
  $$('[data-effect]', list).forEach(button => button.addEventListener("click", () => selectEffect(button.dataset.effect)));
  if (!state.selectedEffect && state.effects.effects[0]) selectEffect(state.effects.effects[0].id);
}

function selectEffect(id) {
  const effect = state.effects.effects.find(item => item.id === id);
  if (!effect) return;
  state.selectedEffect = id;
  $("#effectId").value = effect.id;
  $("#effectName").value = effect.name;
  $("#effectType").value = effect.type;
  $("#effectMinFirmware").value = effect.minimumFirmware || "0.6.0";
  $("#effectSpeed").value = effect.speed;
  $("#effectBrightness").value = effect.brightness;
  $("#effectPalette").value = effect.palette.join(", ");
  $("#effectEnabled").checked = effect.enabled;
  $("#effectEditorTitle").textContent = effect.name;
  $("#speedValue").textContent = effect.speed;
  $("#brightnessValue").textContent = `${effect.brightness}%`;
  updateOrb(effect.palette);
  $$('[data-effect]').forEach(button => button.classList.toggle("active", button.dataset.effect === id));
}

function updateOrb(colors) {
  const palette = colors?.length ? colors : ["#33D6FF", "#FF4FD8"];
  $("#effectOrb").style.setProperty("--orb-1", palette[0]);
  $("#effectOrb").style.setProperty("--orb-2", palette[1] || palette[0]);
}

function renderFirmware() {
  const fw = state.firmware;
  $("#fwVersion").value = fw.latestVersion || "";
  $("#fwMinVersion").value = fw.minimumVersion || "";
  $("#fwChannel").value = fw.channel || "stable";
  $("#fwUrl").value = fw.downloadUrl || "";
  $("#fwSha").value = fw.sha256 || "";
  $("#fwSize").value = fw.size || "";
  $("#fwNotes").value = (fw.releaseNotes || []).join("\n");
  $("#fwMandatory").checked = Boolean(fw.mandatory);
}

function renderDevices() {
  $("#deviceTable").innerHTML = state.devices.devices.map(device => `<tr><td><div class="device-name"><span class="device-avatar">▣</span><div><strong>${escapeHtml(device.name)}</strong><small>${escapeHtml(device.id)}</small></div></div></td><td><strong>${escapeHtml(device.board)}</strong><small>${device.flashMB} MB flash</small></td><td>v${escapeHtml(device.firmware)}</td><td><span class="pill">${escapeHtml(device.channel)}</span></td><td><span class="pill ${device.enabled ? "success" : ""}">${device.enabled ? "Được phép" : "Tạm khóa"}</span></td></tr>`).join("");
}

function renderDocs() {
  const icons = ["⌁", "↟", "{ }"];
  $("#docsGrid").innerHTML = state.docs.documents.map((doc, index) => `<a class="doc-card" href="${escapeHtml(doc.url)}" target="_blank" rel="noopener"><span class="doc-icon">${icons[index % icons.length]}</span><h3>${escapeHtml(doc.title)}</h3><p>${escapeHtml(doc.summary)}</p><span>Mở tài liệu →</span></a>`).join("");
}

function formatBytes(bytes) {
  if (!bytes) return "0 B";
  const units = ["B", "KB", "MB"];
  const unit = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  return `${(bytes / 1024 ** unit).toFixed(unit ? 1 : 0)} ${units[unit]}`;
}

function markDirty(file) {
  state.dirty.add(file);
  $("#unsavedDot").classList.add("visible");
}

function slugify(value) {
  return value.toLowerCase().normalize("NFD").replace(/[\u0300-\u036f]/g, "").replace(/[^a-z0-9]+/g, "-").replace(/(^-|-$)/g, "").slice(0, 40) || `effect-${Date.now()}`;
}

function collectEffectForm() {
  const colors = $("#effectPalette").value.split(",").map(value => value.trim()).filter(value => /^#[0-9a-f]{6}$/i.test(value)).slice(0, 6);
  return {
    id: $("#effectId").value || slugify($("#effectName").value),
    name: $("#effectName").value.trim(),
    type: $("#effectType").value,
    enabled: $("#effectEnabled").checked,
    speed: Number($("#effectSpeed").value),
    brightness: Number($("#effectBrightness").value),
    palette: colors.length ? colors : ["#47E3AD"],
    minimumFirmware: $("#effectMinFirmware").value.trim()
  };
}

function collectFirmwareForm() {
  state.firmware = {
    ...state.firmware,
    schemaVersion: 1,
    channel: $("#fwChannel").value,
    latestVersion: $("#fwVersion").value.trim(),
    minimumVersion: $("#fwMinVersion").value.trim(),
    publishedAt: new Date().toISOString(),
    mandatory: $("#fwMandatory").checked,
    board: "esp32-s3-super-mini",
    flashMB: 4,
    format: "pcota",
    size: Number($("#fwSize").value),
    sha256: $("#fwSha").value.trim().toLowerCase(),
    downloadUrl: $("#fwUrl").value.trim(),
    releaseNotes: $("#fwNotes").value.split("\n").map(line => line.trim()).filter(Boolean)
  };
}

function validateFirmware() {
  collectFirmwareForm();
  if (!/^\d+\.\d+\.\d+$/.test(state.firmware.latestVersion)) throw new Error("Phiên bản phải có dạng 0.6.0");
  if (!/^https:\/\//.test(state.firmware.downloadUrl)) throw new Error("URL firmware phải dùng HTTPS");
  if (!/^[0-9a-f]{64}$/.test(state.firmware.sha256)) throw new Error("SHA-256 phải có đúng 64 ký tự hex");
  if (!Number.isInteger(state.firmware.size) || state.firmware.size < 1000) throw new Error("Kích thước firmware không hợp lệ");
}

function utf8ToBase64(text) {
  const bytes = new TextEncoder().encode(text);
  let binary = "";
  bytes.forEach(byte => { binary += String.fromCharCode(byte); });
  return btoa(binary);
}

function jsonFor(file) {
  const map = {
    "effects.json": state.effects,
    "firmware-manifest.json": state.firmware,
    "devices.json": state.devices,
    "docs.json": state.docs
  };
  return JSON.stringify(map[file], null, 2) + "\n";
}

function downloadText(filename, text) {
  const link = document.createElement("a");
  link.href = URL.createObjectURL(new Blob([text], { type: "application/json" }));
  link.download = filename;
  link.click();
  setTimeout(() => URL.revokeObjectURL(link.href), 500);
}

async function githubRequest(path, options = {}) {
  const response = await fetch(`https://api.github.com${path}`, {
    ...options,
    headers: {
      "Accept": "application/vnd.github+json",
      "X-GitHub-Api-Version": "2022-11-28",
      ...(state.token ? { "Authorization": `Bearer ${state.token}` } : {}),
      ...(options.headers || {})
    }
  });
  const payload = response.status === 204 ? null : await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload?.message || `GitHub API ${response.status}`);
  return payload;
}

async function testConnection(token) {
  state.token = token;
  const repo = await githubRequest(`/repos/${encodeURIComponent(state.repo.owner)}/${encodeURIComponent(state.repo.name)}`);
  if (!repo.permissions?.push) throw new Error("Token đọc được repo nhưng không có quyền ghi");
  state.connected = true;
  state.demo = false;
  updateConnectionUi();
  return repo;
}

function updateConnectionUi() {
  $("#modeText").textContent = state.connected ? "GitHub đã kết nối" : "Chế độ demo";
  $("#repoText").textContent = `${state.repo.branch} / ${state.repo.folder}`;
  $("#demoNotice").classList.toggle("is-hidden", state.connected);
  $("#connectionPill").textContent = state.connected ? "Đã xác thực" : "Chưa kết nối";
  $("#connectionPill").classList.toggle("success", state.connected);
}

async function commitFile(file, message) {
  const path = `${state.repo.folder.replace(/^\/+|\/+$/g, "")}/${file}`;
  const endpoint = `/repos/${encodeURIComponent(state.repo.owner)}/${encodeURIComponent(state.repo.name)}/contents/${path.split("/").map(encodeURIComponent).join("/")}`;
  let sha;
  try {
    const current = await githubRequest(`${endpoint}?ref=${encodeURIComponent(state.repo.branch)}`);
    sha = current.sha;
  } catch (error) {
    if (!/Not Found/i.test(error.message)) throw error;
  }
  return githubRequest(endpoint, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ message, content: utf8ToBase64(jsonFor(file)), branch: state.repo.branch, ...(sha ? { sha } : {}) })
  });
}

async function publishChanges() {
  const files = state.dirty.size ? [...state.dirty] : ["effects.json", "firmware-manifest.json", "devices.json"];
  if (!state.connected) {
    files.forEach(file => downloadText(file, jsonFor(file)));
    showToast("Đã tải các file JSON. Kết nối token để commit trực tiếp.");
    return;
  }
  const button = $("#confirmPublishButton");
  button.disabled = true;
  button.textContent = "Đang commit…";
  try {
    for (const file of files) await commitFile(file, $("#commitMessage").value.trim() || "Update PCScreen cloud data");
    state.dirty.clear();
    $("#unsavedDot").classList.remove("visible");
    closeModal();
    showToast(`Đã xuất bản ${files.length} file lên GitHub.`);
  } catch (error) {
    showToast(`Không thể xuất bản: ${error.message}`, "error");
  } finally {
    button.disabled = false;
    button.textContent = "Commit lên GitHub";
  }
}

function openPublishModal() {
  const files = state.dirty.size ? [...state.dirty] : ["effects.json", "firmware-manifest.json", "devices.json"];
  $("#publishFiles").innerHTML = files.map(file => `<div class="publish-file"><strong>${escapeHtml(file)}</strong><span>${state.connected ? "COMMIT" : "DOWNLOAD"}</span></div>`).join("");
  $("#publishSummary").textContent = state.connected ? `Các file sẽ được ghi vào ${state.repo.owner}/${state.repo.name}, nhánh ${state.repo.branch}.` : "Chế độ demo sẽ tải các file JSON về máy để bạn kiểm tra.";
  $("#confirmPublishButton").textContent = state.connected ? "Commit lên GitHub" : "Tải các file JSON";
  $("#modalBackdrop").classList.remove("is-hidden");
}

function closeModal() { $("#modalBackdrop").classList.add("is-hidden"); }

function bindEvents() {
  $$(".nav-item").forEach(button => button.addEventListener("click", () => setTab(button.dataset.tab)));
  $$('[data-go]').forEach(button => button.addEventListener("click", () => setTab(button.dataset.go)));
  $("#demoButton").addEventListener("click", async () => { $("#loginScreen").classList.add("is-hidden"); $("#appShell").classList.remove("is-hidden"); await loadData(); });
  $("#connectButton").addEventListener("click", async () => {
    const button = $("#connectButton");
    button.disabled = true; button.textContent = "Đang xác thực…";
    try {
      await testConnection($("#tokenInput").value.trim());
      $("#settingsToken").value = $("#tokenInput").value.trim();
      $("#tokenInput").value = "";
      $("#loginScreen").classList.add("is-hidden"); $("#appShell").classList.remove("is-hidden");
      await loadData(); showToast("Đã kết nối GitHub an toàn.");
    } catch (error) { state.token = ""; $("#loginMessage").textContent = error.message; $("#loginMessage").style.color = "var(--danger)"; }
    finally { button.disabled = false; button.textContent = "Kết nối quản trị"; }
  });
  $("#themeButton").addEventListener("click", () => document.body.classList.toggle("light"));
  $("#refreshButton").addEventListener("click", async () => { await loadData(); showToast("Đã đồng bộ lại dữ liệu công khai."); });
  $("#publishButton").addEventListener("click", openPublishModal);
  $("#openSettingsButton").addEventListener("click", () => setTab("settings"));
  $("#modalClose").addEventListener("click", closeModal);
  $("#modalBackdrop").addEventListener("click", event => { if (event.target === $("#modalBackdrop")) closeModal(); });
  $("#confirmPublishButton").addEventListener("click", publishChanges);
  $("#downloadAllButton").addEventListener("click", () => ["effects.json", "firmware-manifest.json", "devices.json", "docs.json"].forEach(file => downloadText(file, jsonFor(file))));

  $("#effectForm").addEventListener("submit", event => {
    event.preventDefault();
    const effect = collectEffectForm();
    const index = state.effects.effects.findIndex(item => item.id === state.selectedEffect);
    if (index >= 0) state.effects.effects[index] = effect; else state.effects.effects.push(effect);
    state.effects.updatedAt = new Date().toISOString(); state.selectedEffect = effect.id;
    markDirty("effects.json"); renderEffects(); selectEffect(effect.id); showToast("Đã lưu hiệu ứng vào bản nháp.");
  });
  $("#newEffectButton").addEventListener("click", () => {
    state.selectedEffect = null; $("#effectForm").reset(); $("#effectId").value = ""; $("#effectName").value = "Hiệu ứng mới"; $("#effectMinFirmware").value = "0.6.0"; $("#effectSpeed").value = 40; $("#effectBrightness").value = 70; $("#effectPalette").value = "#47E3AD, #70A5FF"; $("#effectEditorTitle").textContent = "Hiệu ứng mới"; updateOrb(["#47E3AD", "#70A5FF"]); $("#effectName").focus();
  });
  $("#deleteEffectButton").addEventListener("click", () => {
    if (!state.selectedEffect || !confirm("Xóa hiệu ứng đang chọn khỏi bản nháp?")) return;
    state.effects.effects = state.effects.effects.filter(item => item.id !== state.selectedEffect); state.selectedEffect = null; markDirty("effects.json"); renderEffects(); showToast("Đã xóa hiệu ứng khỏi bản nháp.");
  });
  $("#effectSpeed").addEventListener("input", event => { $("#speedValue").textContent = event.target.value; $("#unsavedDot").classList.add("visible"); });
  $("#effectBrightness").addEventListener("input", event => { $("#brightnessValue").textContent = `${event.target.value}%`; $("#unsavedDot").classList.add("visible"); });
  $("#effectPalette").addEventListener("input", event => updateOrb(event.target.value.split(",").map(value => value.trim())));

  $("#firmwareForm").addEventListener("submit", event => {
    event.preventDefault();
    try { validateFirmware(); markDirty("firmware-manifest.json"); renderAll(); showToast("Manifest firmware đã được kiểm tra và lưu."); }
    catch (error) { showToast(error.message, "error"); }
  });
  $("#downloadFirmwareJson").addEventListener("click", () => { try { validateFirmware(); downloadText("firmware-manifest.json", jsonFor("firmware-manifest.json")); } catch (error) { showToast(error.message, "error"); } });
  $("#addDeviceButton").addEventListener("click", () => {
    const id = prompt("Device ID, ví dụ PCSCREEN-S3-02:"); if (!id) return;
    state.devices.devices.push({ id: id.trim(), name: `PC Screen ${state.devices.devices.length + 1}`, board: "ESP32-S3 Super Mini", flashMB: 4, firmware: state.firmware.latestVersion || "0.6.0", channel: "stable", enabled: true });
    markDirty("devices.json"); renderDevices(); $("#deviceCount").textContent = state.devices.devices.length;
  });

  $("#settingsForm").addEventListener("submit", async event => {
    event.preventDefault();
    state.repo = { owner: $("#repoOwner").value.trim(), name: $("#repoName").value.trim(), branch: $("#repoBranch").value.trim(), folder: $("#repoFolder").value.trim() };
    try { await testConnection($("#settingsToken").value.trim() || state.token); $("#settingsToken").value = ""; showToast("Repository và quyền ghi hợp lệ."); }
    catch (error) { state.connected = false; updateConnectionUi(); showToast(error.message, "error"); }
  });
  $("#disconnectButton").addEventListener("click", () => { state.token = ""; state.connected = false; state.demo = true; $("#settingsToken").value = ""; updateConnectionUi(); showToast("Đã xóa token khỏi bộ nhớ tab."); });
}

bindEvents();
