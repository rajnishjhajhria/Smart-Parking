#pragma once
// ─────────────────────────────────────────────
//  router.h  —  Simple path-pattern router
// ─────────────────────────────────────────────
#include "http_server.h"
#include <vector>
#include <regex>

struct Route {
    std::string method;
    std::string pattern;   // e.g. /api/slots/:id
    std::regex  regex;
    std::vector<std::string> param_names;
    Handler     handler;
};

class Router {
public:
    void add(const std::string& method, const std::string& pattern, Handler h) {
        Route r;
        r.method  = method;
        r.pattern = pattern;
        r.handler = h;

        // Build regex from pattern, extract :param names
        std::string rx = "^";
        std::string seg;
        std::istringstream ss(pattern);
        bool first = true;
        std::string p = pattern;
        size_t i = 0;
        while (i < p.size()) {
            if (p[i] == ':') {
                // param segment
                ++i;
                std::string name;
                while (i < p.size() && p[i] != '/') name += p[i++];
                r.param_names.push_back(name);
                rx += "([^/]+)";
            } else {
                // literal char — escape for regex
                char c = p[i++];
                if (c == '/' || c == '-' || c == '_') rx += c;
                else rx += c;
            }
        }
        rx += "$";
        r.regex = std::regex(rx);
        routes_.push_back(r);
    }

    void GET(const std::string& p, Handler h)    { add("GET",    p, h); }
    void POST(const std::string& p, Handler h)   { add("POST",   p, h); }
    void DEL(const std::string& p, Handler h) { add("DELETE", p, h); }

    bool dispatch(HttpRequest& req, HttpResponse& res) const {
        for (const auto& route : routes_) {
            if (route.method != req.method) continue;
            std::smatch m;
            if (std::regex_match(req.path, m, route.regex)) {
                // Fill path params
                for (size_t i = 0; i < route.param_names.size(); ++i)
                    req.params[route.param_names[i]] = m[i + 1].str();
                res = route.handler(req);
                return true;
            }
        }
        return false;
    }

private:
    std::vector<Route> routes_;
};
