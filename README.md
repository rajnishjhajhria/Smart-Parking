# 🚗 Smart Parking Management System

A full parking management system with a **native C++ HTTP server** and an **HTML/CSS/Vanilla JS** frontend.

This project includes:
- Slot allotment and status tracking
- Reservation and cancellation
- Vehicle unpark with ticket generation
- Fee calculation by parked duration
- Real-time dashboard refresh

---

## 📁 Project Structure
smart-parking/
```
smart-parking/
│
├── backend/                       ← C++ server
│   ├── make.cmd                   ← Windows-friendly make wrapper
│   ├── main.cpp                   ← Entry point, socket loop, all API routes
│   ├── Makefile                   ← Build system
│   └── include/
│       ├── database.h             ← In-memory data store (slots, vehicles, tickets)
│       ├── http_server.h          ← HTTP parsing, response builder, JSON utils
│       └── router.h               ← URL pattern router with :param support
│
│   ├── assets/                    ← Logo + background images
└── frontend/                      ← Browser UI
    ├── index.html                 ← HTML skeleton
    ├── css/
    │   └── styles.css             ← All styling & design tokens
    └── js/
        ├── api.js                 ← All fetch() calls to C++ backend
        ├── ui.js                  ← Pure DOM rendering helpers
        └── app.js                 ← Controller: state + event wiring
```

---

## ⚙️ Build & Run
- `g++` with C++17 support (`g++ --version`)
- One of the following for frontend serving:
    - Node.js (`npx serve`), or
    - Python 3 (`python -m http.server`)
- Linux / macOS
- `g++` with C++17 support (`g++ --version`)

### 1. Compile & Start the C++ Server

```bash
cd backend
make          # compile

#### Windows (this repo setup)

```powershell
cd backend
make run
```

The repository includes `backend/make.cmd`, so `make run` works in this workspace terminal setup.
make run      # compile + start server on port 3001
```

You should see:
```
🚗 Smart Parking API (C++) running on http://localhost:3001
```

### 2. Serve the Frontend

The frontend uses ES modules, so it must be served (not opened as file://).

```bash
# Option A — Python (built-in, no install)
cd frontend
python -m http.server 5500

# Option B — Node.js
cd frontend
npx serve . -p 5500

# Option C — VS Code Live Server
# Right-click index.html → "Open with Live Server"
```

Open **http://localhost:5500** in your browser.

Note: If port `5500` is already in use, `serve` may auto-select a different port.

---

## ✅ Quick Health Checks

```bash
# backend health
curl http://localhost:3001/health

# level stats
curl http://localhost:3001/api/stats?level=1
```

---

## 🔌 REST API Reference

| Method   | Endpoint              | Description                         |
|----------|-----------------------|-------------------------------------|
| `GET`    | `/`                   | Basic API info                      |
| `GET`    | `/health`             | Health check                        |
| `GET`    | `/api/slots`          | All slots (`?level=1` to filter)    |
| `GET`    | `/api/slots/:id`      | Single slot with vehicle details    |
| `GET`    | `/api/stats`          | Occupancy counts (`?level=1`)       |
| `POST`   | `/api/park`           | Park a vehicle                      |
| `POST`   | `/api/unpark`         | Remove vehicle + generate ticket    |
| `POST`   | `/api/reserve`        | Reserve an available slot           |
| `DELETE` | `/api/reserve/:id`    | Cancel a reservation                |
| `GET`    | `/api/vehicles`       | List vehicles (`?search=plate`)     |
| `GET`    | `/api/tickets`        | List all generated tickets          |

---

## 🧱 C++ Architecture

```
main.cpp
 ├── TCP socket server loop (platform-aware: Windows/Linux)
 ├── parse_request()     — reads raw HTTP, splits headers/body/query
 ├── build_response()    — writes HTTP/1.1 response with CORS headers
 ├── Router              — matches path patterns, extracts :params
 └── Handler functions   — one per API endpoint

include/database.h
 ├── struct Slot         — id, level, status, vehicleId, parkedAt
 ├── struct Vehicle      — id, plate, type, slotId, parkedAt
 ├── struct Ticket       — full exit record with fee
 ├── struct Booking      — reservation record
 └── class Database      — std::map storage + JSON serialization

include/http_server.h
 ├── HttpRequest         — method, path, query_params, body, headers
 ├── HttpResponse        — status, body, content_type
 ├── parse_query()       — URL query string parser
 ├── url_decode()        — %xx + decode
 └── json_get()          — lightweight JSON value extractor

include/router.h
 └── Router              — stores routes as std::regex patterns
                           dispatches to Handler with path params
```

---

## 🧠 System Notes

- Data is **in-memory** (no DB), so server restart resets runtime state.
- Slot pricing is currently fixed at `₹40/hour` with minimum billable duration.
- Full conceptual explanation is available in `SYSTEM_EXPLAINED.md`.

---

## 💡 How to Extend

| Goal                    | Where to change                                  |
|-------------------------|--------------------------------------------------|
| Add a new API endpoint  | Add handler fn + `router.GET/POST/DELETE` in `main.cpp` |
| Add a new data field    | Add to struct in `database.h`, update `*_to_json()` |
| Persistent storage      | Replace `std::map<>` with SQLite (`libsqlite3`)  |
| Multi-threading         | Wrap `accept` loop with `std::thread` per client |
| Login / Auth            | Add session tokens, check `Authorization` header |
| HTTPS / TLS             | Wrap socket with OpenSSL `BIO`                   |

---

## 🎨 Tech Stack

| Layer    | Technology            |
|----------|-----------------------|
| Backend  | C++17, raw sockets    |
| HTTP     | Hand-rolled (main.cpp)|
| Data     | In-memory (`std::map`)|
| Frontend | HTML5, CSS3, Vanilla JS (ES Modules) |
| Build    | Makefile + g++        |
