#pragma once
// ─────────────────────────────────────────────
//  http_server.h  —  Minimal HTTP/1.1 server
//  Pure C++ sockets, no external libraries
// ─────────────────────────────────────────────
#include <string>
#include <map>
#include <functional>
#include <sstream>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;    // raw query string after '?'
    std::string body;
    std::map<std::string, std::string> params;  // path params
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> headers;
};

struct HttpResponse {
    int         status  = 200;
    std::string body;
    std::string content_type = "application/json";

    static HttpResponse json(const std::string& b, int s = 200) {
        HttpResponse r; r.status = s; r.body = b;
        return r;
    }
    static HttpResponse error(const std::string& msg, int s = 400) {
        HttpResponse r;
        r.status = s;
        r.body   = "{\"error\":\"" + msg + "\"}";
        return r;
    }
};

using Handler = std::function<HttpResponse(const HttpRequest&)>;

// ── UTILS ─────────────────────────────────────

inline std::map<std::string, std::string> parse_query(const std::string& qs) {
    std::map<std::string, std::string> m;
    if (qs.empty()) return m;
    std::istringstream ss(qs);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto eq = token.find('=');
        if (eq != std::string::npos)
            m[token.substr(0, eq)] = token.substr(eq + 1);
    }
    return m;
}

inline std::string url_decode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int c;
            sscanf(s.substr(i+1,2).c_str(), "%x", &c);
            out += (char)c; i += 2;
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

// Extract value for a JSON key from a flat JSON string
inline std::string json_get(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        // string value
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\') ++pos; // skip escape
            val += json[pos++];
        }
        return val;
    } else {
        // number / bool / null
        std::string val;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']')
            val += json[pos++];
        // trim
        val.erase(val.find_last_not_of(" \t\r\n") + 1);
        return val;
    }
}

// Simple string escape for JSON values
inline std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}
