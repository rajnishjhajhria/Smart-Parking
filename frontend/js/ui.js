// ─────────────────────────────────────────────
//  ui.js  —  Pure DOM rendering helpers
// ─────────────────────────────────────────────

// ── SLOTS ─────────────────────────────────────
export function renderSlots(slots, selectedId, onSelect) {
  const grid = document.getElementById('slots-grid');
  grid.innerHTML = slots.map(s => `
    <div class="slot ${s.status} ${s.id === selectedId ? 'selected' : ''}" data-id="${s.id}">
      <div class="slot-status-dot"></div>
      ${carSVG(s.status)}
      <div class="slot-id">${s.id}</div>
      <div class="slot-status-label">${s.status}</div>
    </div>`).join('');
  grid.querySelectorAll('.slot').forEach(el =>
    el.addEventListener('click', () => onSelect(el.dataset.id)));
}

function carSVG(status) {
  if (status === 'occupied') return `
    <svg class="slot-car" viewBox="0 0 40 60" fill="none">
      <rect x="8" y="10" width="24" height="38" rx="6" fill="rgba(255,59,59,.2)" stroke="rgba(255,59,59,.55)" stroke-width="1.5"/>
      <rect x="10" y="14" width="20" height="12" rx="3" fill="rgba(255,255,255,.07)"/>
      <rect x="9" y="42" width="7" height="5" rx="2" fill="rgba(255,59,59,.5)"/>
      <rect x="24" y="42" width="7" height="5" rx="2" fill="rgba(255,59,59,.5)"/>
      <rect x="9" y="10" width="7" height="5" rx="2" fill="rgba(255,59,59,.5)"/>
      <rect x="24" y="10" width="7" height="5" rx="2" fill="rgba(255,59,59,.5)"/>
    </svg>`;
  if (status === 'reserved') return `
    <svg class="slot-car" viewBox="0 0 40 60" fill="none">
      <rect x="8" y="10" width="24" height="38" rx="6" fill="rgba(245,166,35,.08)" stroke="rgba(245,166,35,.45)" stroke-width="1.5" stroke-dasharray="4 2"/>
      <text x="20" y="36" text-anchor="middle" fill="rgba(245,166,35,.65)" font-size="14">⏱</text>
    </svg>`;
  return `
    <svg class="slot-car" viewBox="0 0 40 60" fill="none">
      <rect x="8" y="10" width="24" height="38" rx="6" fill="rgba(198,241,53,.04)" stroke="rgba(198,241,53,.2)" stroke-width="1" stroke-dasharray="3 3"/>
    </svg>`;
}

// ── STATS ─────────────────────────────────────
export function renderStats({ total, occupied, available, reserved }) {
  set('stat-total', total); set('stat-avail', available);
  set('stat-occ', occupied); set('stat-occ2', occupied); set('stat-res', reserved);
  bar('bar-avail', available, total);
  bar('bar-occ', occupied, total);
  bar('bar-res', reserved, total);
  const arc = document.querySelector('circle[data-arc]');
  if (arc && total > 0) arc.style.strokeDashoffset = 263.9 * (1 - occupied / total);
}

// ── SLOT DETAIL ───────────────────────────────
export function renderSlotDetail(slot, selectedTime, onConfirm) {
  const card = document.getElementById('slot-detail-card');
  if (!slot) {
    card.innerHTML = `<div class="detail-slot-id">—</div><div class="detail-status available">Select a slot</div>`;
    return;
  }
  const label = slot.status.charAt(0).toUpperCase() + slot.status.slice(1);
  let rows = '', btnLabel = '', btnClass = '';

  if (slot.status === 'available') {
    rows = `
      <div class="detail-row"><span class="detail-row-label">Floor</span><span class="detail-row-val">Level ${slot.level}</span></div>
      <div class="detail-row"><span class="detail-row-label">Type</span><span class="detail-row-val">Standard</span></div>
      <div class="detail-row"><span class="detail-row-label">Rate</span><span class="detail-row-val">₹40 / hr</span></div>
      <div class="detail-row"><span class="detail-row-label">Time Slot</span><span class="detail-row-val">${selectedTime || '—'}</span></div>`;
    btnLabel = 'Park Vehicle Here'; btnClass = '';
  } else if (slot.status === 'occupied' && slot.vehicle) {
    rows = `
      <div class="detail-row"><span class="detail-row-label">Plate</span><span class="detail-row-val">${slot.vehicle.plate}</span></div>
      <div class="detail-row"><span class="detail-row-label">Type</span><span class="detail-row-val">${slot.vehicle.type}</span></div>
      <div class="detail-row"><span class="detail-row-label">Parked At</span><span class="detail-row-val">${fmtTime(slot.vehicle.parkedAt)}</span></div>
      <div class="detail-row"><span class="detail-row-label">Fee Due</span><span class="detail-row-val">₹${slot.fee ?? 0}</span></div>`;
    btnLabel = 'Release & Generate Ticket'; btnClass = 'danger';
  } else if (slot.status === 'reserved') {
    rows = `
      <div class="detail-row"><span class="detail-row-label">Reserved By</span><span class="detail-row-val">${slot.reservedBy || '—'}</span></div>
      <div class="detail-row"><span class="detail-row-label">Time</span><span class="detail-row-val">${slot.reservedTime || '—'}</span></div>
      <div class="detail-row"><span class="detail-row-label">Floor</span><span class="detail-row-val">Level ${slot.level}</span></div>`;
    btnLabel = 'Cancel Reservation'; btnClass = 'warn';
  }

  card.innerHTML = `
    <div class="detail-slot-id">${slot.id}</div>
    <div class="detail-status ${slot.status}">${label}</div>
    <div>${rows}</div>
    ${btnLabel ? `<button class="confirm-btn ${btnClass}" id="confirm-btn">${btnLabel}</button>` : ''}`;
  document.getElementById('confirm-btn')?.addEventListener('click', () => onConfirm(slot));
}

// ── VEHICLES ──────────────────────────────────
export function renderVehicles(vehicles, onHighlight) {
  const list = document.getElementById('vehicle-list');
  if (!vehicles.length) { list.innerHTML = `<div class="empty-state">No vehicles parked</div>`; return; }
  list.innerHTML = `<div class="section-title" style="margin-bottom:10px">Parked Vehicles</div>` +
    vehicles.map(v => `
      <div class="vehicle-item" data-slot="${v.slotId}">
        <div class="vehicle-icon"><svg viewBox="0 0 24 24"><path d="M5 17H3a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v10a2 2 0 0 1-2 2h-1M9 21a2 2 0 1 0 0-4 2 2 0 0 0 0 4zM20 21a2 2 0 1 0 0-4 2 2 0 0 0 0 4z"/></svg></div>
        <div class="vehicle-info">
          <div class="vehicle-plate">${v.plate}</div>
          <div class="vehicle-slot">Slot ${v.slotId} · Level ${v.level}</div>
        </div>
        <div class="vehicle-time">${fmtTime(v.parkedAt)}<div class="vehicle-fee">₹${v.fee}</div></div>
      </div>`).join('');
  list.querySelectorAll('.vehicle-item').forEach(el =>
    el.addEventListener('click', () => onHighlight(el.dataset.slot)));
}

export function setLoading(id, msg = 'Loading…') {
  const el = document.getElementById(id);
  if (el) el.innerHTML = `<div class="loading-state">${msg}</div>`;
}

// ── TIMESLOTS ─────────────────────────────────
const BOOKED = new Set(['06:00','07:00','08:00']);
const TIMES  = ['06:00','07:00','08:00','09:00','10:00','11:00','12:00','13:00','14:00','15:00','16:00','17:00','18:00'];

export function renderTimeslots(selectedTime, dayOffset, onSelect) {
  const d = new Date(); d.setDate(d.getDate() + dayOffset);
  set('date-label', dayOffset === 0 ? 'Today' : dayOffset === 1 ? 'Tomorrow'
    : d.toLocaleDateString('en-IN', { day:'numeric', month:'short' }));
  const wrap = document.getElementById('timeslots');
  wrap.innerHTML = TIMES.map(t => {
    const booked = dayOffset === 0 && BOOKED.has(t);
    const sel = !booked && selectedTime === t;
    return `<button class="timeslot ${booked?'booked':''} ${sel?'selected':''}" data-time="${t}" ${booked?'disabled':''}>${t}</button>`;
  }).join('');
  wrap.querySelectorAll('.timeslot:not(.booked)').forEach(b =>
    b.addEventListener('click', () => onSelect(b.dataset.time)));
}

// ── TOAST ─────────────────────────────────────
let _tt;
export function toast(msg, isError = false) {
  const t = document.getElementById('toast');
  document.getElementById('toast-msg').textContent = msg;
  document.getElementById('toast-icon').innerHTML = isError
    ? `<svg viewBox="0 0 24 24" fill="none" stroke="var(--red)" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>`
    : `<svg viewBox="0 0 24 24" fill="none" stroke="var(--lime)" stroke-width="2"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>`;
  t.classList.toggle('error', isError);
  t.classList.add('show');
  clearTimeout(_tt);
  _tt = setTimeout(() => t.classList.remove('show'), 3200);
}

// ── API STATUS ────────────────────────────────
export function setApiStatus(online) {
  const el = document.getElementById('api-status');
  if (!el) return;
  el.className = `api-status ${online ? 'online' : 'offline'}`;
  el.querySelector('.api-label').textContent = online ? 'C++ API Online' : 'API Offline';
}

// ── MODAL ─────────────────────────────────────
export const openModal  = id => document.getElementById(id).classList.add('open');
export const closeModal = id => document.getElementById(id).classList.remove('open');

export function renderTicket(ticket) {
  return `<div class="ticket-box">
    <div class="ticket-id"># ${ticket.id}</div>
    <div class="ticket-fee">₹${ticket.fee}</div>
    <div class="ticket-row"><span class="ticket-label">Plate</span><span class="ticket-val">${ticket.plate}</span></div>
    <div class="ticket-row"><span class="ticket-label">Slot</span><span class="ticket-val">${ticket.slotId} · Level ${ticket.level}</span></div>
    <div class="ticket-row"><span class="ticket-label">Parked</span><span class="ticket-val">${fmtTime(ticket.parkedAt)}</span></div>
    <div class="ticket-row"><span class="ticket-label">Exit</span><span class="ticket-val">${fmtTime(ticket.exitAt)}</span></div>
    <div class="ticket-row"><span class="ticket-label">Duration</span><span class="ticket-val">${parseFloat(ticket.duration).toFixed(2)} hrs</span></div>
  </div>`;
}

// ── UTILS ─────────────────────────────────────
function set(id, val) { const e = document.getElementById(id); if (e) e.textContent = val; }
function bar(id, val, total) { const e = document.getElementById(id); if (e) e.style.width = (total > 0 ? val/total*100 : 0) + '%'; }
export function fmtTime(iso) {
  if (!iso) return '—';
  return new Date(iso).toLocaleTimeString('en-IN', { hour:'2-digit', minute:'2-digit' });
}
