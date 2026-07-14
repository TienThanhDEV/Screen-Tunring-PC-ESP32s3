const MAX_REPORT_BYTES = 8192;
const REPORT_INTERVAL_MS = 240000;

function corsHeaders(origin) {
  return {
    "access-control-allow-origin": origin,
    "access-control-allow-headers": "authorization, content-type",
    "access-control-allow-methods": "GET,POST,OPTIONS",
    "access-control-max-age": "86400",
    vary: "Origin"
  };
}

function json(value, status = 200, origin = "*") {
  return new Response(JSON.stringify(value), {
    status,
    headers: {
      ...corsHeaders(origin),
      "content-type": "application/json; charset=utf-8",
      "cache-control": "no-store",
      "x-content-type-options": "nosniff"
    }
  });
}

function allowedOrigin(request, env) {
  const origin = request.headers.get("origin") || "";
  const allowed = String(env.ADMIN_ORIGIN || "https://tienthanhdev.github.io")
    .split(",").map(value => value.trim()).filter(Boolean);
  return allowed.includes(origin) ? origin : "*";
}

function isAdmin(request, env) {
  const header = request.headers.get("authorization") || "";
  return Boolean(env.ADMIN_KEY) && header === `Bearer ${env.ADMIN_KEY}`;
}

function safeNumber(value, minimum, maximum) {
  const number = Number(value);
  return Number.isFinite(number) && number >= minimum && number <= maximum
    ? number : null;
}

function sanitizeReport(body, id, mac, now) {
  const telemetry = body.telemetry && typeof body.telemetry === "object"
    ? body.telemetry : {};
  const values = {};
  const fields = {
    cpuTemp: [-100, 200], cpuLoad: [0, 100], gpuTemp: [-100, 250],
    gpuLoad: [0, 100], ramLoad: [0, 100], fps: [0, 10000],
    diskLoad: [0, 100], networkDownMbps: [0, 100000],
    networkUpMbps: [0, 100000], loadAverage: [0, 10000],
    uptimeSeconds: [0, 4294967295], processCount: [0, 65535]
  };
  for (const [name, range] of Object.entries(fields)) {
    const value = safeNumber(telemetry[name], range[0], range[1]);
    if (value !== null) values[name] = value;
  }
  return {
    deviceId: id,
    mac,
    firmware: String(body.firmware || "").slice(0, 24),
    board: String(body.board || "").slice(0, 40),
    flashMB: safeNumber(body.flashMB, 1, 64),
    uptimeSeconds: safeNumber(body.uptimeSeconds, 0, 4294967295),
    freeHeap: safeNumber(body.freeHeap, 0, 16777216),
    pcConnected: Boolean(body.pcConnected),
    rotation: safeNumber(body.rotation, 0, 3),
    language: body.language === "en" ? "en" : "vi",
    telemetry: values,
    receivedAt: new Date(now).toISOString(),
    receivedAtMs: now
  };
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const origin = allowedOrigin(request, env);
    if (request.method === "OPTIONS") {
      return new Response(null, { status: 204, headers: corsHeaders(origin) });
    }

    if (request.method === "GET" && url.pathname === "/api/v1/health") {
      return json({ ok: true, service: "PCScreen Device API", version: 2 },
        200, origin);
    }

    if (request.method === "POST" && url.pathname === "/api/v1/report") {
      if (env.ALLOW_ANONYMOUS_REPORTS !== "true") {
        return json({ error: "reporting_disabled" }, 403, origin);
      }
      const declaredLength = Number(request.headers.get("content-length") || 0);
      if (declaredLength > MAX_REPORT_BYTES) {
        return json({ error: "payload_too_large" }, 413, origin);
      }
      const raw = await request.text();
      if (new TextEncoder().encode(raw).byteLength > MAX_REPORT_BYTES) {
        return json({ error: "payload_too_large" }, 413, origin);
      }
      let body;
      try { body = JSON.parse(raw); }
      catch { return json({ error: "invalid_json" }, 400, origin); }

      const id = String(body.deviceId || "").toUpperCase();
      const mac = String(body.mac || "").toUpperCase();
      const validId = /^PCSCREEN-[0-9A-F]{12}$/.test(id);
      const validMac = /^([0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(mac);
      if (!validId || !validMac || id.slice(9) !== mac.replaceAll(":", "")) {
        return json({ error: "invalid_identity" }, 422, origin);
      }

      const key = `device:${id}`;
      const previous = await env.DEVICES.get(key, "json");
      const now = Date.now();
      if (previous?.receivedAtMs && now - previous.receivedAtMs < REPORT_INTERVAL_MS) {
        return json({ ok: true, throttled: true, nextReportSeconds: 300 },
          202, origin);
      }
      await env.DEVICES.put(key,
        JSON.stringify(sanitizeReport(body, id, mac, now)));
      return json({ ok: true, nextReportSeconds: 300 }, 202, origin);
    }

    if (request.method === "GET" && url.pathname === "/api/v1/devices") {
      if (!isAdmin(request, env)) {
        return json({ error: "unauthorized" }, 401, origin);
      }
      const limit = Math.min(100, Math.max(1, Number(url.searchParams.get("limit")) || 100));
      const cursor = url.searchParams.get("cursor") || undefined;
      const listed = await env.DEVICES.list({ prefix: "device:", limit, cursor });
      const devices = (await Promise.all(listed.keys.map(key =>
        env.DEVICES.get(key.name, "json")))).filter(Boolean);
      devices.sort((a, b) => (b.receivedAtMs || 0) - (a.receivedAtMs || 0));
      return json({ devices, cursor: listed.list_complete ? null : listed.cursor },
        200, origin);
    }

    return json({ error: "not_found" }, 404, origin);
  }
};
