// ─────────────────────────────────────────────
//  app.js  —  Main controller
// ─────────────────────────────────────────────
import * as API from './api.js';
import * as UI  from './ui.js';

const state = { level:1, slots:[], selectedSlot:null, selectedTime:null, dayOffset:0, modalType:null };

// ── BOOT ─────────────────────────────────────
async function boot() {
  clock();
  bindLevelTabs();
  bindSidebar();
  bindActions();
  bindSearch();
  bindDateNav();
  bindModalClose();
  UI.renderTimeslots(null, 0, onTimeSelect);

  UI.setApiStatus(false);
  const healthPromise = API.checkHealth()
    .then(online => {
      UI.setApiStatus(online);
      if (!online) UI.toast('C++ server not running. Start it with: make run', true);
    })
    .catch(() => {
      UI.setApiStatus(false);
      UI.toast('C++ server not running. Start it with: make run', true);
    });

  const refreshPromise = refresh();
  await Promise.allSettled([healthPromise, refreshPromise]);
  setInterval(refresh, 15_000);
}

async function refresh() {
  await Promise.all([loadSlots(), loadStats(), loadVehicles()]);
}

// ── SLOTS ─────────────────────────────────────
async function loadSlots() {
  try {
    const { slots } = await API.getSlots(state.level);
    state.slots = slots;
    if (state.selectedSlot)
      state.selectedSlot = slots.find(s => s.id === state.selectedSlot.id) || null;
    UI.renderSlots(slots, state.selectedSlot?.id, onSlotClick);
    UI.renderSlotDetail(state.selectedSlot, state.selectedTime, onConfirmSlot);
  } catch { UI.toast('Failed to load slots', true); }
}

async function onSlotClick(id) {
  try {
    state.selectedSlot = await API.getSlotById(id);
  } catch {
    state.selectedSlot = state.slots.find(s => s.id === id) || null;
  }
  UI.renderSlots(state.slots, id, onSlotClick);
  UI.renderSlotDetail(state.selectedSlot, state.selectedTime, onConfirmSlot);
}

async function onConfirmSlot(slot) {
  if      (slot.status === 'available') openParkModal(slot.id);
  else if (slot.status === 'occupied')  await doUnpark({ slotId: slot.id });
  else if (slot.status === 'reserved')  await doCancelReservation(slot.id);
}

// ── STATS ─────────────────────────────────────
async function loadStats() {
  try { UI.renderStats(await API.getStats(state.level)); } catch {}
}

// ── VEHICLES ──────────────────────────────────
async function loadVehicles(search = '') {
  UI.setLoading('vehicle-list', 'Loading vehicles…');
  try {
    const { vehicles } = await API.getVehicles(search);
    UI.renderVehicles(vehicles, id => onSlotClick(id));
  } catch { UI.renderVehicles([], () => {}); }
}

// ── PARK ──────────────────────────────────────
function openParkModal(preSlot = '') {
  state.modalType = 'park';
  modal('Park Vehicle', 'Assign a vehicle to a parking slot', `
    <div class="form-group">
      <label class="form-label">Plate Number *</label>
      <input class="form-input" id="inp-plate" placeholder="e.g. DL 1AB 2345"/>
      <div class="form-error" id="err-plate">Plate number is required</div>
    </div>
    <div class="form-group">
      <label class="form-label">Vehicle Type</label>
      <input class="form-input" id="inp-type" value="Car" placeholder="Car / SUV / Bike"/>
    </div>
    <div class="form-group">
      <label class="form-label">Slot (blank = auto-assign)</label>
      <input class="form-input" id="inp-slot" value="${preSlot}" placeholder="e.g. P1-03"/>
    </div>`);
  document.getElementById('inp-plate').focus();
}

async function doPark() {
  const plate = val('inp-plate'), type = val('inp-type') || 'Car', slotId = val('inp-slot') || undefined;
  if (!plate) { fieldErr('err-plate'); return; }
  setBusy(true);
  try {
    const res = await API.parkVehicle({ plate, type, slotId });
    UI.toast(`${plate} parked at ${res.slot.id}`);
    UI.closeModal('modal-overlay');
    await refresh();
  } catch (e) { UI.toast(e.message, true); } finally { setBusy(false); }
}

// ── UNPARK ────────────────────────────────────
async function doUnpark(body) {
  try {
    const res = await API.unparkVehicle(body);
    ticketModal(res.ticket);
    await refresh();
  } catch (e) { UI.toast(e.message, true); }
}

function openRemoveModal() {
  state.modalType = 'remove';
  modal('Remove Vehicle', 'Release an occupied slot', `
    <div class="form-group">
      <label class="form-label">Plate Number</label>
      <input class="form-input" id="inp-plate" placeholder="e.g. DL 1AB 2345"/>
    </div>
    <div style="text-align:center;color:var(--muted);font-size:11px;margin:-4px 0 10px">— or —</div>
    <div class="form-group">
      <label class="form-label">Slot ID</label>
      <input class="form-input" id="inp-slot" placeholder="e.g. P1-02"/>
      <div class="form-error" id="err-slot">Enter plate or slot ID</div>
    </div>`);
}

async function doRemove() {
  const plate = val('inp-plate'), slotId = val('inp-slot');
  if (!plate && !slotId) { fieldErr('err-slot'); return; }
  setBusy(true);
  try {
    const res = await API.unparkVehicle({ plate, slotId });
    ticketModal(res.ticket);
    await refresh();
  } catch (e) { UI.toast(e.message, true); } finally { setBusy(false); }
}

function ticketModal(ticket) {
  state.modalType = 'ticket';
  document.getElementById('modal-title').textContent = 'Parking Ticket';
  document.getElementById('modal-sub').textContent   = 'Vehicle released successfully';
  document.getElementById('modal-body').innerHTML    = UI.renderTicket(ticket);
  document.getElementById('modal-btns').innerHTML    = `<button class="btn-confirm" style="grid-column:1/-1" onclick="window.print()">🖨 Print</button>`;
  UI.openModal('modal-overlay');
}

// ── RESERVE ───────────────────────────────────
function openReserveModal(preSlot = '') {
  state.modalType = 'reserve';
  modal('Reserve Slot', 'Pre-book a slot for a future arrival', `
    <div class="form-group">
      <label class="form-label">Plate Number *</label>
      <input class="form-input" id="inp-plate" placeholder="e.g. MH 04 CD 6789"/>
      <div class="form-error" id="err-plate">Required</div>
    </div>
    <div class="form-group">
      <label class="form-label">Slot ID *</label>
      <input class="form-input" id="inp-slot" value="${preSlot}" placeholder="e.g. P1-03"/>
      <div class="form-error" id="err-slot">Required</div>
    </div>
    <div class="form-group">
      <label class="form-label">Time Slot *</label>
      <input class="form-input" id="inp-time" value="${state.selectedTime || ''}" placeholder="e.g. 13:00"/>
      <div class="form-error" id="err-time">Required</div>
    </div>`);
  document.getElementById('inp-plate').focus();
}

async function doReserve() {
  const plate = val('inp-plate'), slotId = val('inp-slot'), time = val('inp-time');
  let ok = true;
  if (!plate)  { fieldErr('err-plate'); ok=false; }
  if (!slotId) { fieldErr('err-slot');  ok=false; }
  if (!time)   { fieldErr('err-time');  ok=false; }
  if (!ok) return;
  setBusy(true);
  try {
    await API.reserveSlot({ plate, slotId, time });
    UI.toast(`Slot ${slotId} reserved for ${time}`);
    UI.closeModal('modal-overlay');
    await refresh();
  } catch (e) { UI.toast(e.message, true); } finally { setBusy(false); }
}

async function doCancelReservation(slotId) {
  try {
    await API.cancelReservation(slotId);
    UI.toast(`Reservation for ${slotId} cancelled`);
    state.selectedSlot = null;
    await refresh();
  } catch (e) { UI.toast(e.message, true); }
}

// ── MODAL SUBMIT ROUTER ───────────────────────
window._submitModal = async () => {
  if      (state.modalType === 'park')    await doPark();
  else if (state.modalType === 'remove')  await doRemove();
  else if (state.modalType === 'reserve') await doReserve();
  else UI.closeModal('modal-overlay');
};

// ── TIMESLOTS ─────────────────────────────────
function onTimeSelect(t) {
  state.selectedTime = state.selectedTime === t ? null : t;
  UI.renderTimeslots(state.selectedTime, state.dayOffset, onTimeSelect);
  UI.renderSlotDetail(state.selectedSlot, state.selectedTime, onConfirmSlot);
}

function bindDateNav() {
  document.getElementById('date-prev')?.addEventListener('click', () => {
    state.dayOffset = Math.max(0, state.dayOffset - 1);
    UI.renderTimeslots(state.selectedTime, state.dayOffset, onTimeSelect);
  });
  document.getElementById('date-next')?.addEventListener('click', () => {
    state.dayOffset = Math.min(6, state.dayOffset + 1);
    UI.renderTimeslots(state.selectedTime, state.dayOffset, onTimeSelect);
  });
}

// ── LEVEL TABS ────────────────────────────────
function bindLevelTabs() {
  document.querySelectorAll('.level-tab').forEach(btn => {
    btn.addEventListener('click', async () => {
      state.level = parseInt(btn.dataset.level);
      state.selectedSlot = null;
      document.querySelectorAll('.level-tab').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      document.getElementById('lvl-label').textContent = state.level;
      await refresh();
    });
  });
}

// ── SIDEBAR & ACTIONS ─────────────────────────
function bindSidebar() {
  document.getElementById('btn-park')?.addEventListener('click',     () => openParkModal(state.selectedSlot?.status === 'available' ? state.selectedSlot.id : ''));
  document.getElementById('btn-remove')?.addEventListener('click',   openRemoveModal);
  document.getElementById('btn-vehicles')?.addEventListener('click', () => document.getElementById('vehicle-list').scrollIntoView({ behavior:'smooth' }));
}

function bindActions() {
  document.getElementById('act-park')?.addEventListener('click',    () => openParkModal(state.selectedSlot?.status === 'available' ? state.selectedSlot.id : ''));
  document.getElementById('act-ticket')?.addEventListener('click',  openRemoveModal);
  document.getElementById('act-search')?.addEventListener('click',  () => document.getElementById('search-input').focus());
  document.getElementById('act-reserve')?.addEventListener('click', () => openReserveModal(state.selectedSlot?.status === 'available' ? state.selectedSlot.id : ''));
}

// ── SEARCH ────────────────────────────────────
function bindSearch() {
  let t;
  document.getElementById('search-input')?.addEventListener('input', e => {
    clearTimeout(t); t = setTimeout(() => loadVehicles(e.target.value), 280);
  });
}

// ── MODAL CLOSE ───────────────────────────────
function bindModalClose() {
  document.getElementById('modal-overlay')?.addEventListener('click', e => {
    if (e.target === e.currentTarget) UI.closeModal('modal-overlay');
  });
  document.getElementById('btn-cancel-modal')?.addEventListener('click', () => UI.closeModal('modal-overlay'));
}

// ── HELPERS ───────────────────────────────────
function modal(title, sub, body) {
  document.getElementById('modal-title').textContent = title;
  document.getElementById('modal-sub').textContent   = sub;
  document.getElementById('modal-body').innerHTML    = body;
  document.getElementById('modal-btns').innerHTML    = `
    <button class="btn-cancel" id="btn-cancel-modal">Cancel</button>
    <button class="btn-confirm" id="btn-confirm-modal" onclick="window._submitModal()">Confirm</button>`;
  document.getElementById('btn-cancel-modal').addEventListener('click', () => UI.closeModal('modal-overlay'));
  UI.openModal('modal-overlay');
}

function val(id)       { return (document.getElementById(id)?.value || '').trim(); }
function fieldErr(id)  { const e = document.getElementById(id); if(!e)return; e.classList.add('show'); setTimeout(()=>e.classList.remove('show'),3000); }
function setBusy(on)   { const b = document.getElementById('btn-confirm-modal'); if(b){b.disabled=on; b.textContent=on?'Please wait…':'Confirm';} }

function clock() {
  const el = document.getElementById('clock');
  const tick = () => el && (el.textContent = new Date().toTimeString().slice(0,8));
  tick(); setInterval(tick, 1000);
}

document.addEventListener('DOMContentLoaded', boot);
