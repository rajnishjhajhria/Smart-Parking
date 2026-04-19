#pragma once
// ─────────────────────────────────────────────
//  database.h  —  In-memory data store
// ─────────────────────────────────────────────
#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

// ── DATA STRUCTS ──────────────────────────────

struct Vehicle {
    std::string id;
    std::string plate;
    std::string type;
    std::string slot_id;
    std::time_t parked_at;
};

struct Slot {
    std::string id;
    int         level;
    std::string status;   // available | occupied | reserved
    std::string vehicle_id;
    std::string reserved_by;
    std::string reserved_time;
    std::time_t parked_at;
};

struct Ticket {
    std::string id;
    std::string plate;
    std::string type;
    std::string slot_id;
    int         level;
    std::time_t parked_at;
    std::time_t exit_at;
    double      duration_hrs;
    int         fee;
};

struct Booking {
    std::string id;
    std::string slot_id;
    std::string plate;
    std::string time_slot;
    std::time_t created_at;
};

// ── DATABASE CLASS ────────────────────────────
class Database {
public:
    std::map<std::string, Slot>    slots;
    std::map<std::string, Vehicle> vehicles;
    std::vector<Ticket>            tickets;
    std::vector<Booking>           bookings;

    static const int RATE_PER_HOUR = 40; // ₹

    Database() { seed(); }

    // ── HELPERS ──────────────────────────────
    static std::string make_id(const std::string& prefix) {
        static int counter = 1000;
        return prefix + std::to_string(++counter);
    }

    static std::string fmt_time(std::time_t t) {
        if (t == 0) return "—";
        struct tm* tm_info = localtime(&t);
        char buf[6];
        strftime(buf, sizeof(buf), "%H:%M", tm_info);
        return std::string(buf);
    }

    static std::string fmt_iso(std::time_t t) {
        if (t == 0) return "";
        struct tm* tm_info = localtime(&t);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm_info);
        return std::string(buf);
    }

    int calc_fee(std::time_t parked_at) const {
        if (parked_at == 0) return 0;
        double hrs = difftime(std::time(nullptr), parked_at) / 3600.0;
        if (hrs < 0.25) hrs = 0.25;
        return static_cast<int>(hrs * RATE_PER_HOUR + 0.5);
    }

    // ── FIND FIRST AVAILABLE SLOT ─────────────
    Slot* first_available(int level = 0) {
        for (auto it = slots.begin(); it != slots.end(); ++it) {
            Slot& slot = it->second;
            if (slot.status == "available") {
                if (level == 0 || slot.level == level) return &slot;
            }
        }
        return nullptr;
    }

    // ── JSON SERIALIZATION ────────────────────
    std::string slot_to_json(const Slot& s) const {
        std::ostringstream o;
        o << "{";
        o << "\"id\":\"" << s.id << "\",";
        o << "\"level\":" << s.level << ",";
        o << "\"status\":\"" << s.status << "\",";
        o << "\"reservedBy\":\"" << s.reserved_by << "\",";
        o << "\"reservedTime\":\"" << s.reserved_time << "\",";
        o << "\"parkedAt\":\"" << fmt_iso(s.parked_at) << "\"";

        if (!s.vehicle_id.empty()) {
            auto it = vehicles.find(s.vehicle_id);
            if (it != vehicles.end()) {
                const Vehicle& v = it->second;
                o << ",\"vehicle\":{";
                o << "\"id\":\"" << v.id << "\",";
                o << "\"plate\":\"" << v.plate << "\",";
                o << "\"type\":\"" << v.type << "\",";
                o << "\"slotId\":\"" << v.slot_id << "\",";
                o << "\"parkedAt\":\"" << fmt_iso(v.parked_at) << "\"";
                o << "}";
                o << ",\"fee\":" << calc_fee(s.parked_at);
            }
        }
        o << "}";
        return o.str();
    }

    std::string vehicle_to_json(const Vehicle& v) const {
        auto it = slots.find(v.slot_id);
        int lvl = it != slots.end() ? it->second.level : 0;
        int fee = calc_fee(v.parked_at);
        std::ostringstream o;
        o << "{";
        o << "\"id\":\"" << v.id << "\",";
        o << "\"plate\":\"" << v.plate << "\",";
        o << "\"type\":\"" << v.type << "\",";
        o << "\"slotId\":\"" << v.slot_id << "\",";
        o << "\"level\":" << lvl << ",";
        o << "\"parkedAt\":\"" << fmt_iso(v.parked_at) << "\",";
        o << "\"fee\":" << fee;
        o << "}";
        return o.str();
    }

    std::string ticket_to_json(const Ticket& t) const {
        std::ostringstream o;
        o << "{";
        o << "\"id\":\"" << t.id << "\",";
        o << "\"plate\":\"" << t.plate << "\",";
        o << "\"type\":\"" << t.type << "\",";
        o << "\"slotId\":\"" << t.slot_id << "\",";
        o << "\"level\":" << t.level << ",";
        o << "\"parkedAt\":\"" << fmt_iso(t.parked_at) << "\",";
        o << "\"exitAt\":\"" << fmt_iso(t.exit_at) << "\",";
        o << "\"duration\":" << std::fixed << std::setprecision(2) << t.duration_hrs << ",";
        o << "\"fee\":" << t.fee;
        o << "}";
        return o.str();
    }

private:
    void seed() {
        // Slot counts per level
        int counts[] = {0, 15, 15, 15, 15};
        for (int lvl = 1; lvl <= 4; ++lvl) {
            for (int i = 1; i <= counts[lvl]; ++i) {
                char buf[16];
                snprintf(buf, sizeof(buf), "P%d-%02d", lvl, i);
                Slot s;
                s.id        = buf;
                s.level     = lvl;
                s.status    = "available";
                s.parked_at = 0;
                slots[s.id] = s;
            }
        }

        // Seed some occupied slots
        struct { const char* slot; const char* plate; int hrs_ago; } occ[] = {
            {"P1-02","DL 1AB 2345",2}, {"P1-04","MH 04 CD 6789",1},
            {"P1-07","KA 09 EF 1234",3},{"P1-12","TN 22 GH 5678",1},
            {"P1-15","RJ 14 KL 3456",4},{"P2-03","UP 81 IJ 9012",2},
            {"P3-09","GJ 01 MN 7890",1},
        };
        for (auto& row : occ) {
            std::string vid = make_id("V");
            std::time_t t   = std::time(nullptr) - row.hrs_ago * 3600;
            Vehicle v; v.id=vid; v.plate=row.plate; v.type="Car"; v.slot_id=row.slot; v.parked_at=t;
            vehicles[vid] = v;
            slots[row.slot].status     = "occupied";
            slots[row.slot].vehicle_id = vid;
            slots[row.slot].parked_at  = t;
        }

        // Seed reserved
        slots["P1-05"].status       = "reserved";
        slots["P1-05"].reserved_by  = "Walk-in";
        slots["P1-05"].reserved_time= "13:00";
        slots["P2-10"].status       = "reserved";
        slots["P2-10"].reserved_by  = "Walk-in";
        slots["P2-10"].reserved_time= "15:00";
    }
};
