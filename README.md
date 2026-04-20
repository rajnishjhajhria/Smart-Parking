# Smart Parking Management System

A parking management dashboard with a native C++ HTTP API and an HTML/CSS/Vanilla JavaScript frontend.

## Features

- View parking slots by floor
- Track available, occupied, and reserved slots
- Park vehicles manually or auto-assign an available slot
- Reserve and cancel parking slots
- Unpark vehicles and generate parking tickets
- Calculate parking fees based on parked duration
- Refresh live dashboard data from the backend API

## Project Structure

```text
smart-parking/
|-- backend/
|   |-- main.cpp
|   |-- Makefile
|   |-- make.cmd
|   `-- include/
|       |-- database.h
|       |-- http_server.h
|       `-- router.h
|-- frontend/
|   |-- index.html
|   |-- assets/
|   |   |-- logo.png
|   |   `-- mustang.png
|   |-- css/
|   |   `-- styles.css
|   `-- js/
|       |-- api.js
|       |-- app.js
|       `-- ui.js
|-- .gitignore
`-- README.md
```

## Requirements

- `g++` with C++17 support
- `make` for Linux/macOS, or the included `backend/make.cmd` wrapper on Windows
- A local static file server for the frontend, such as Python, Node.js, or VS Code Live Server

## Build and Run

### 1. Start the Backend

```bash
cd backend
make run
```

On Windows, this repository includes `make.cmd`, so the same command can be used from the `backend` folder if your terminal resolves local command wrappers.

The API runs on:

```text
http://localhost:3001
```

### 2. Start the Frontend

The frontend uses ES modules, so serve it through a local server instead of opening `index.html` directly.

Using Python:

```bash
cd frontend
python -m http.server 5500
```

Using Node.js:

```bash
cd frontend
npx serve . -p 5500
```

Then open:

```text
http://localhost:5500
```

## Quick Checks

```bash
curl http://localhost:3001/health
curl http://localhost:3001/api/stats?level=1
```

## API Reference

| Method | Endpoint | Description |
| --- | --- | --- |
| `GET` | `/` | API information |
| `GET` | `/health` | Backend health check |
| `GET` | `/api/slots` | List all slots |
| `GET` | `/api/slots?level=1` | List slots for one floor |
| `GET` | `/api/slots/:id` | Get one slot with vehicle details |
| `GET` | `/api/stats` | Get occupancy stats |
| `POST` | `/api/park` | Park a vehicle |
| `POST` | `/api/unpark` | Unpark a vehicle and generate a ticket |
| `POST` | `/api/reserve` | Reserve an available slot |
| `DELETE` | `/api/reserve/:id` | Cancel a reservation |
| `GET` | `/api/vehicles` | List parked vehicles |
| `GET` | `/api/vehicles?search=plate` | Search vehicles by plate |
| `GET` | `/api/tickets` | List generated tickets |

## Tech Stack

| Layer | Technology |
| --- | --- |
| Backend | C++17, raw sockets |
| HTTP | Custom lightweight HTTP handling |
| Data | In-memory `std::map` and `std::vector` storage |
| Frontend | HTML5, CSS3, Vanilla JavaScript |
| Build | Makefile and g++ |

## Notes

- Data is stored in memory, so restarting the backend resets slots, vehicles, reservations, and tickets.
- Parking fee is currently fixed at `Rs. 40/hour` with a minimum billable duration.
- The backend runs on port `3001`.
- The frontend expects the API at `http://127.0.0.1:3001`.
