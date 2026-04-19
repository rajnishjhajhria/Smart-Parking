# 🚗 Smart Parking Management System — C++ Backend

A full parking management system with a **native C++ HTTP server** and HTML/CSS/JS frontend.
**No external libraries. No frameworks. Pure C++ sockets.**

---

## 📁 Project Structure

```
smart-parking/
│
├── backend/                       ← C++ server
│   ├── main.cpp                   ← Entry point, socket loop, all API routes
│   ├── Makefile                   ← Build system
│   └── include/
│       ├── database.h             ← In-memory data store (slots, vehicles, tickets)
│       ├── http_server.h          ← HTTP parsing, response builder, JSON utils
│       └── router.h               ← URL pattern router with :param support
│
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

### Requirements
- Linux / macOS
- `g++` with C++17 support (`g++ --version`)

### 1. Compile & Start the C++ Server

```bash
cd backend
make          # compile
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
python3 -m http.server 5500

# Option B — Node.js
npx serve frontend -p 5500
or
cd frontend
npx serve . -p 5500

# Option C — VS Code Live Server
# Right-click index.html → "Open with Live Server"
```

Open **http://localhost:5500** in your browser.

---

## 🔌 REST API Reference

| Method   | Endpoint              | Description                         |
|----------|-----------------------|-------------------------------------|
| `GET`    | `/api/slots`          | All slots (`?level=1` to filter)    |
| `GET`    | `/api/slots/:id`      | Single slot with vehicle details    |
| `GET`    | `/api/stats`          | Occupancy counts (`?level=1`)       |
| `POST`   | `/api/park`           | Park a vehicle                      |
| `POST`   | `/api/unpark`         | Remove vehicle + generate ticket    |
| `POST`   | `/api/reserve`        | Reserve an available slot           |
| `DELETE` | `/api/reserve/:id`    | Cancel a reservation                |
| `GET`    | `/api/vehicles`       | List vehicles (`?search=plate`)     |
| `GET`    | `/api/tickets`        | List all generated tickets          |

### Example cURL calls

```bash
# Park a vehicle
curl -X POST http://localhost:3001/api/park \
  -H "Content-Type: application/json" \
  -d '{"plate":"DL 9ZZ 0001","type":"SUV","slotId":"P1-01"}'

# Get Level 1 stats
curl http://localhost:3001/api/stats?level=1

# Remove vehicle by slot
curl -X POST http://localhost:3001/api/unpark \
  -H "Content-Type: application/json" \
  -d '{"slotId":"P1-02"}'

# Reserve a slot
curl -X POST http://localhost:3001/api/reserve \
  -H "Content-Type: application/json" \
  -d '{"slotId":"P1-03","plate":"MH 04 XY 1234","time":"14:00"}'
```

---

## 🧱 C++ Architecture

```
main.cpp
 ├── POSIX TCP socket (AF_INET / SOCK_STREAM)
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
| Backend  | C++17, POSIX sockets  |
| HTTP     | Hand-rolled (main.cpp)|
| Data     | In-memory (`std::map`)|
| Frontend | HTML5, CSS3, Vanilla JS (ES Modules) |
| Build    | GNU Make + g++        |
