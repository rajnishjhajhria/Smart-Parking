// ─────────────────────────────────────────────
//  api.js  —  All HTTP calls to the C++ backend
// ─────────────────────────────────────────────
const BASE = 'http://127.0.0.1:3001/api';
const ROOT = 'http://127.0.0.1:3001';
const REQUEST_TIMEOUT_MS = 2500;

async function req(path, opts = {}) {
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);
  try {
    const res = await fetch(BASE + path, {
      headers: { 'Content-Type': 'application/json' },
      signal: controller.signal,
      ...opts,
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Request failed');
    return data;
  } catch (error) {
    if (error.name === 'AbortError') {
      throw new Error('Backend request timed out');
    }
    throw error;
  } finally {
    clearTimeout(timeoutId);
  }
}

export const getSlots    = (level) => req(`/slots?level=${level}`);
export const getSlotById = (id)    => req(`/slots/${id}`);
export const getStats    = (level) => req(`/stats?level=${level}`);
export const getVehicles = (search = '') => req(`/vehicles${search ? '?search=' + encodeURIComponent(search) : ''}`);
export const getTickets  = ()      => req('/tickets');

export const parkVehicle      = (body) => req('/park',    { method: 'POST',   body: JSON.stringify(body) });
export const unparkVehicle    = (body) => req('/unpark',  { method: 'POST',   body: JSON.stringify(body) });
export const reserveSlot      = (body) => req('/reserve', { method: 'POST',   body: JSON.stringify(body) });
export const cancelReservation = (id)  => req(`/reserve/${id}`, { method: 'DELETE' });

export async function checkHealth() {
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);
  try {
    const res = await fetch(ROOT + '/health', { signal: controller.signal });
    return res.ok;
  } catch {
    return false;
  } finally {
    clearTimeout(timeoutId);
  }
}
