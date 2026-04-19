// ─────────────────────────────────────────────
//  main.cpp  —  Smart Parking C++ Backend
//  HTTP server on port 3001 (POSIX sockets)
// ─────────────────────────────────────────────
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <ctime>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include "include/http_server.h"
#include "include/router.h"
#include "include/database.h"

static const int PORT = 3001;

#ifdef _WIN32
using socket_t = SOCKET;
using socket_len_t = int;
#else
using socket_t = int;
using socket_len_t = socklen_t;
#endif

static bool init_network() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

static void cleanup_network() {
#ifdef _WIN32
    WSACleanup();
#endif
}

static void close_socket(socket_t fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

// ── GLOBAL DB (single instance) ───────────────
Database db;

// ── HTTP PARSING ──────────────────────────────
HttpRequest parse_request(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    // Request line
    std::getline(stream, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream rl(line);
    std::string full_path;
    rl >> req.method >> full_path;

    // Split path and query
    auto qpos = full_path.find('?');
    if (qpos != std::string::npos) {
        req.path  = full_path.substr(0, qpos);
        req.query = full_path.substr(qpos + 1);
    } else {
        req.path = full_path;
    }
    req.query_params = parse_query(url_decode(req.query));

    // Headers
    int content_length = 0;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto col = line.find(':');
        if (col != std::string::npos) {
            std::string key = line.substr(0, col);
            std::string val = line.substr(col + 2);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            req.headers[key] = val;
            if (key == "content-length")
                content_length = std::stoi(val);
        }
    }

    // Body
    if (content_length > 0) {
        req.body.resize(content_length);
        stream.read(&req.body[0], content_length);
    }

    return req;
}

// ── HTTP RESPONSE BUILDER ─────────────────────
std::string build_response(const HttpResponse& res) {
    std::ostringstream out;
    out << "HTTP/1.1 " << res.status;
    switch (res.status) {
        case 200: out << " OK"; break;
        case 201: out << " Created"; break;
        case 204: out << " No Content"; break;
        case 400: out << " Bad Request"; break;
        case 404: out << " Not Found"; break;
        case 409: out << " Conflict"; break;
        default:  out << " Error"; break;
    }
    out << "\r\n";
    out << "Content-Type: "   << res.content_type << "; charset=utf-8\r\n";
    out << "Content-Length: " << res.body.size()  << "\r\n";
    out << "Access-Control-Allow-Origin: *\r\n";
    out << "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n";
    out << "Access-Control-Allow-Headers: Content-Type\r\n";
    out << "Connection: close\r\n";
    out << "\r\n";
    out << res.body;
    return out.str();
}

// ─────────────────────────────────────────────
//  ROUTE HANDLERS
// ─────────────────────────────────────────────

// GET /api/slots?level=1
HttpResponse handle_get_slots(const HttpRequest& req) {
    int level = 0;
    auto it = req.query_params.find("level");
    if (it != req.query_params.end()) level = std::stoi(it->second);

    std::ostringstream o;
    o << "{\"slots\":[";
    bool first = true;

    // Collect and sort by id
    std::vector<const Slot*> list;
    for (auto it = db.slots.begin(); it != db.slots.end(); ++it) {
        Slot& s = it->second;
        if (level == 0 || s.level == level) list.push_back(&s);
    }
    std::sort(list.begin(), list.end(), [](const Slot* a, const Slot* b){ return a->id < b->id; });

    for (const Slot* s : list) {
        if (!first) o << ",";
        o << db.slot_to_json(*s);
        first = false;
    }
    o << "]}";
    return HttpResponse::json(o.str());
}

// GET /api/slots/:id
HttpResponse handle_get_slot(const HttpRequest& req) {
    auto it = db.slots.find(req.params.at("id"));
    if (it == db.slots.end()) return HttpResponse::error("Slot not found", 404);
    return HttpResponse::json(db.slot_to_json(it->second));
}

// GET /api/stats?level=1
HttpResponse handle_get_stats(const HttpRequest& req) {
    int level = 0;
    auto it = req.query_params.find("level");
    if (it != req.query_params.end()) level = std::stoi(it->second);

    int total = 0, occupied = 0, available = 0, reserved = 0;
    for (auto it = db.slots.begin(); it != db.slots.end(); ++it) {
        Slot& s = it->second;
        if (level != 0 && s.level != level) continue;
        ++total;
        if (s.status == "occupied")  ++occupied;
        if (s.status == "available") ++available;
        if (s.status == "reserved")  ++reserved;
    }

    std::ostringstream o;
    o << "{\"total\":" << total
      << ",\"occupied\":"  << occupied
      << ",\"available\":" << available
      << ",\"reserved\":"  << reserved
      << ",\"ratePerHour\":" << Database::RATE_PER_HOUR << "}";
    return HttpResponse::json(o.str());
}

// POST /api/park  { plate, type, slotId? }
HttpResponse handle_park(const HttpRequest& req) {
    std::string plate  = json_get(req.body, "plate");
    std::string type   = json_get(req.body, "type");
    std::string slotId = json_get(req.body, "slotId");
    if (type.empty()) type = "Car";

    if (plate.empty()) return HttpResponse::error("Plate number required");

    // Duplicate check
    for (auto it = db.vehicles.begin(); it != db.vehicles.end(); ++it) {
        const Vehicle& v = it->second;
        if (v.plate == plate && db.slots.count(v.slot_id) &&
            db.slots.at(v.slot_id).status == "occupied")
            return HttpResponse::error("Vehicle already parked", 409);
    }

    // Find slot
    Slot* slot = nullptr;
    if (!slotId.empty()) {
        auto it = db.slots.find(slotId);
        if (it != db.slots.end()) slot = &it->second;
    } else {
        slot = db.first_available();
    }

    if (!slot) return HttpResponse::error("No available slot found", 404);
    if (slot->status != "available") return HttpResponse::error("Slot not available", 409);

    std::string vid     = Database::make_id("V");
    std::time_t now     = std::time(nullptr);
    Vehicle v; v.id=vid; v.plate=plate; v.type=type; v.slot_id=slot->id; v.parked_at=now;
    db.vehicles[vid] = v;

    slot->status     = "occupied";
    slot->vehicle_id = vid;
    slot->parked_at  = now;

    std::ostringstream o;
    o << "{\"message\":\"Vehicle parked\","
      << "\"slot\":" << db.slot_to_json(*slot) << ","
      << "\"vehicle\":" << db.vehicle_to_json(v) << "}";
    return HttpResponse::json(o.str(), 201);
}

// POST /api/unpark  { slotId?, plate? }
HttpResponse handle_unpark(const HttpRequest& req) {
    std::string slotId = json_get(req.body, "slotId");
    std::string plate  = json_get(req.body, "plate");

    Slot* slot = nullptr;
    if (!slotId.empty()) {
        auto it = db.slots.find(slotId);
        if (it != db.slots.end()) slot = &it->second;
    }
    if (!slot && !plate.empty()) {
        for (auto itv = db.vehicles.begin(); itv != db.vehicles.end(); ++itv) {
            const Vehicle& v = itv->second;
            if (v.plate == plate) {
                auto it = db.slots.find(v.slot_id);
                if (it != db.slots.end()) { slot = &it->second; break; }
            }
        }
    }

    if (!slot) return HttpResponse::error("Slot/vehicle not found", 404);
    if (slot->status != "occupied") return HttpResponse::error("Slot is not occupied", 409);

    auto& v        = db.vehicles.at(slot->vehicle_id);
    std::time_t now = std::time(nullptr);
    double hrs     = difftime(now, slot->parked_at) / 3600.0;
    if (hrs < 0.25) hrs = 0.25;
    int fee        = static_cast<int>(hrs * Database::RATE_PER_HOUR + 0.5);

    Ticket t;
    t.id           = "TKT-" + std::to_string(now);
    t.plate        = v.plate;
    t.type         = v.type;
    t.slot_id      = slot->id;
    t.level        = slot->level;
    t.parked_at    = slot->parked_at;
    t.exit_at      = now;
    t.duration_hrs = hrs;
    t.fee          = fee;
    db.tickets.push_back(t);

    db.vehicles.erase(slot->vehicle_id);
    slot->status     = "available";
    slot->vehicle_id = "";
    slot->parked_at  = 0;

    std::ostringstream o;
    o << "{\"message\":\"Vehicle removed\",\"ticket\":" << db.ticket_to_json(t) << "}";
    return HttpResponse::json(o.str());
}

// POST /api/reserve  { slotId, plate, time }
HttpResponse handle_reserve(const HttpRequest& req) {
    std::string slotId = json_get(req.body, "slotId");
    std::string plate  = json_get(req.body, "plate");
    std::string tslot  = json_get(req.body, "time");

    if (slotId.empty() || plate.empty() || tslot.empty())
        return HttpResponse::error("slotId, plate, time required");

    auto it = db.slots.find(slotId);
    if (it == db.slots.end()) return HttpResponse::error("Slot not found", 404);
    Slot& slot = it->second;
    if (slot.status != "available") return HttpResponse::error("Slot not available", 409);

    slot.status        = "reserved";
    slot.reserved_by   = plate;
    slot.reserved_time = tslot;

    Booking b; b.id=Database::make_id("B"); b.slot_id=slotId; b.plate=plate; b.time_slot=tslot; b.created_at=std::time(nullptr);
    db.bookings.push_back(b);

    std::ostringstream o;
    o << "{\"message\":\"Slot reserved\",\"slot\":" << db.slot_to_json(slot) << "}";
    return HttpResponse::json(o.str(), 201);
}

// DELETE /api/reserve/:id
HttpResponse handle_cancel_reserve(const HttpRequest& req) {
    auto it = db.slots.find(req.params.at("id"));
    if (it == db.slots.end()) return HttpResponse::error("Slot not found", 404);
    Slot& slot = it->second;
    if (slot.status != "reserved") return HttpResponse::error("Slot is not reserved", 409);

    slot.status        = "available";
    slot.reserved_by   = "";
    slot.reserved_time = "";
    db.bookings.erase(std::remove_if(db.bookings.begin(), db.bookings.end(),
        [&](const Booking& b){ return b.slot_id == slot.id; }), db.bookings.end());

    std::ostringstream o;
    o << "{\"message\":\"Reservation cancelled\",\"slot\":" << db.slot_to_json(slot) << "}";
    return HttpResponse::json(o.str());
}

// GET /api/vehicles?search=plate
HttpResponse handle_get_vehicles(const HttpRequest& req) {
    std::string search;
    auto it = req.query_params.find("search");
    if (it != req.query_params.end()) search = it->second;
    // lowercase search
    std::string sl = search;
    std::transform(sl.begin(), sl.end(), sl.begin(), ::tolower);

    std::ostringstream o;
    o << "{\"vehicles\":[";
    bool first = true;
    for (auto itv = db.vehicles.begin(); itv != db.vehicles.end(); ++itv) {
        const Vehicle& v = itv->second;
        std::string pl = v.plate;
        std::transform(pl.begin(), pl.end(), pl.begin(), ::tolower);
        if (!sl.empty() && pl.find(sl) == std::string::npos) continue;
        if (!first) o << ",";
        o << db.vehicle_to_json(v);
        first = false;
    }
    o << "]}";
    return HttpResponse::json(o.str());
}

// GET /api/tickets
HttpResponse handle_get_tickets(const HttpRequest& req) {
    std::ostringstream o;
    o << "{\"tickets\":[";
    bool first = true;
    for (auto it = db.tickets.rbegin(); it != db.tickets.rend(); ++it) {
        if (!first) o << ",";
        o << db.ticket_to_json(*it);
        first = false;
    }
    o << "]}";
    return HttpResponse::json(o.str());
}

// GET /
HttpResponse handle_root(const HttpRequest&) {
    return HttpResponse::json(
        "{\"name\":\"Smart Parking API\","
        "\"status\":\"ok\","
        "\"routes\":["
        "\"/api/slots\","
        "\"/api/stats\","
        "\"/api/vehicles\","
        "\"/api/tickets\","
        "\"/api/park\","
        "\"/api/unpark\","
        "\"/api/reserve\"]}"
    );
}

// GET /health
HttpResponse handle_health(const HttpRequest&) {
    return HttpResponse::json("{\"status\":\"ok\"}");
}

// ─────────────────────────────────────────────
//  MAIN — Socket server loop
// ─────────────────────────────────────────────
int main() {
    if (!init_network()) {
        std::cerr << "Failed to initialize network stack\n";
        return 1;
    }

    // Build router
    Router router;
    router.GET   ("/api/slots",          handle_get_slots);
    router.GET   ("/api/slots/:id",      handle_get_slot);
    router.GET   ("/api/stats",          handle_get_stats);
    router.POST  ("/api/park",           handle_park);
    router.POST  ("/api/unpark",         handle_unpark);
    router.POST  ("/api/reserve",        handle_reserve);
    router.DEL("/api/reserve/:id",       handle_cancel_reserve);
    router.GET   ("/api/vehicles",       handle_get_vehicles);
    router.GET   ("/api/tickets",        handle_get_tickets);
    router.GET   ("/",                   handle_root);
    router.GET   ("/health",             handle_health);

    // Create TCP socket
    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == (socket_t)-1) {
        std::cerr << "socket() failed\n";
        cleanup_network();
        return 1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed\n";
        close_socket(server_fd);
        cleanup_network();
        return 1;
    }
    if (listen(server_fd, 64) < 0) {
        std::cerr << "listen() failed\n";
        close_socket(server_fd);
        cleanup_network();
        return 1;
    }

    std::cout << "\n\033[1;32m🚗 Smart Parking API (C++) running on http://localhost:"
              << PORT << "\033[0m\n\n";

    // Accept loop (single-threaded)
    while (true) {
        sockaddr_in client_addr{};
        socket_len_t client_len = sizeof(client_addr);
        socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd == (socket_t)-1) continue;

        // Read raw request (up to 8 KB)
        char buf[8192] = {};
        int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { close_socket(client_fd); continue; }

        std::string raw(buf, n);
        HttpRequest  req = parse_request(raw);
        HttpResponse res;

        // OPTIONS preflight (CORS)
        if (req.method == "OPTIONS") {
            res.status = 204;
            res.body   = "";
        } else {
            bool matched = router.dispatch(req, res);
            if (!matched) res = HttpResponse::error("Not found", 404);
        }

        std::cout << req.method << " " << req.path
                  << " → " << res.status << "\n";

        std::string response_str = build_response(res);
        send(client_fd, response_str.c_str(), static_cast<int>(response_str.size()), 0);
        close_socket(client_fd);
    }

    close_socket(server_fd);
    cleanup_network();
    return 0;
}
