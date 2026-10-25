#pragma once
// Private baseline input values and bounded callback script; no SDL or guest access.
#include "../../src/frontend_input_policy.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <array>
#include <istream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace baseline::input {
struct State {
    uint16_t buttons = 0;
    float x = 0, y = 0;
    bool nonneutral() const { return buttons || x || y; }
};
inline float axis(int value) {
    return tooie::input::baseline_axis(value);
}
struct Step { std::string kind, name; uint64_t ms; State state; };
struct Plan { std::vector<Step> steps; };
inline void require_script_bound(bool script, bool explicit_seconds, bool manual) {
    if (script && (!explicit_seconds || manual))
        throw std::runtime_error("--input-script requires explicit --seconds and cannot use --manual");
}
inline uint64_t milliseconds(const std::string& word, uint64_t maximum) {
    if (word.empty() || word.find_first_not_of("0123456789") != std::string::npos || word.size() > 6)
        throw std::runtime_error("Input script requires decimal milliseconds");
    const auto value = std::stoull(word);
    if (!value || value > maximum) throw std::runtime_error("Input script duration out of bounds");
    return value;
}
inline State chord(const std::string& word) {
    State out;
    std::set<std::string> seen;
    std::istringstream words(word);
    std::string token;
    if (word.empty() || word.back() == ',') throw std::runtime_error("Input script empty chord");
    while (std::getline(words, token, ',')) {
        if (!seen.insert(token).second) throw std::runtime_error("Input script duplicate chord token");
        if (token == "A") out.buttons |= 0x8000;
        else if (token == "B") out.buttons |= 0x4000;
        else if (token == "Z") out.buttons |= 0x2000;
        else if (token == "START") out.buttons |= 0x1000;
        else if (token == "L") out.buttons |= 0x0020;
        else if (token == "R") out.buttons |= 0x0010;
        else if (token == "C_UP") out.buttons |= 0x0008;
        else if (token == "C_DOWN") out.buttons |= 0x0004;
        else if (token == "C_LEFT") out.buttons |= 0x0002;
        else if (token == "C_RIGHT") out.buttons |= 0x0001;
        else if (token == "D_UP") out.buttons |= 0x0800;
        else if (token == "D_DOWN") out.buttons |= 0x0400;
        else if (token == "D_LEFT") out.buttons |= 0x0200;
        else if (token == "D_RIGHT") out.buttons |= 0x0100;
        else if (token == "STICK_UP") out.y += 1;
        else if (token == "STICK_DOWN") out.y -= 1;
        else if (token == "STICK_LEFT") out.x -= 1;
        else if (token == "STICK_RIGHT") out.x += 1;
        else throw std::runtime_error("Input script unknown chord token");
    }
    if (!out.nonneutral() || (seen.count("STICK_UP") && seen.count("STICK_DOWN")) ||
        (seen.count("STICK_LEFT") && seen.count("STICK_RIGHT")) ||
        (seen.count("D_UP") && seen.count("D_DOWN")) || (seen.count("D_LEFT") && seen.count("D_RIGHT")) ||
        (seen.count("C_UP") && seen.count("C_DOWN")) || (seen.count("C_LEFT") && seen.count("C_RIGHT")))
        throw std::runtime_error("Input script contradictory or neutral hold");
    return out;
}
inline Plan parse(std::istream& in) {
    Plan out;
    std::string line, command, duration, extra;
    size_t bytes = 0;
    std::set<std::string> names;
    while (std::getline(in, line)) {
        bytes += line.size() + 1;
        if (bytes > 16384 || line.size() > 256) throw std::runtime_error("Input script too large");
        std::istringstream row(line);
        if (!(row >> command)) continue;
        if (!out.steps.empty() && out.steps.back().kind == "checkpoint")
            throw std::runtime_error("Input script checkpoint must be terminal");
        Step step;
        step.kind = command;
        if (command != "hold" && command != "wait" && command != "release" && command != "checkpoint")
            throw std::runtime_error("Input script unknown step");
        if (!(row >> step.name >> duration) || step.name.size() > 48 ||
            step.name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos ||
            !names.insert(step.name).second) throw std::runtime_error("Input script invalid or duplicate step name");
        step.ms = milliseconds(duration, 30000);
        if (command == "hold") {
            if (!(row >> extra)) throw std::runtime_error("Input script hold requires chord");
            step.state = chord(extra);
        }
        if (row >> extra) throw std::runtime_error("Input script extra step fields");
        if (out.steps.size() >= 64) throw std::runtime_error("Input script exceeds run bounds");
        out.steps.push_back(std::move(step));
    }
    if (!in.eof() || out.steps.empty() || out.steps.back().kind != "checkpoint") throw std::runtime_error("Input script absent, unreadable or empty");
    return out;
}

// Per-call receipt records contain immutable plan pointers; print only after unlocking.
// At most two records per method call: start+step, delivered+step or delivered+finish.
struct Receipt {
    enum Kind { Start, StepEntry, Delivered, Finish } kind;
    const Step* step = nullptr;
    const char* reason = nullptr;
    size_t index = 0;
    uint64_t reads = 0, elapsed = 0;
};
struct Events {
    std::array<Receipt, 2> records{};
    size_t count = 0;
    void add(Receipt value) { records.at(count++) = value; }
};
// All methods require the caller's input mutex; no SDL, runtime or I/O.
class Player {
    Plan plan;
    State physical;
    size_t index = 0;
    uint64_t run_at = 0, step_at = 0, reads = 0;
    uint64_t latest = 0;
    uint64_t clock(uint64_t t) { if (t > latest) latest = t; return latest; }
    bool begun = false, done = false, latched = false, stopped = false, focused = true, awaiting_delivery = true;
    bool scripted() const { return !plan.steps.empty() && !done; }
    static void emit(Events* events, Receipt value) { if (events) events->add(value); }
    void finish(const char* reason, uint64_t now, Events* events) {
        done = latched = true;
        emit(events, {Receipt::Finish, nullptr, reason, index, reads, begun ? now - run_at : 0});
    }
    void next_step(uint64_t now, Events* events) {
        emit(events, {Receipt::Delivered, &plan.steps[index], nullptr, index, reads, now - step_at});
        if (++index == plan.steps.size()) { finish("complete", now, events); return; }
        reads = 0; awaiting_delivery = true;
    }
    void expire(uint64_t now, Events* events) {
        if (scripted() && begun && !awaiting_delivery && now - step_at >= 30000) {
            if (reads >= 2) next_step(now, events); else finish("abort=step-cap", now, events);
        }
    }
public:
    explicit Player(Plan value = {}) : plan(std::move(value)) {}
    bool has_script() const { return !plan.steps.empty(); }
    bool owns_port() const { return scripted(); }
    bool started() const { return begun; }
    bool finished() const { return done; }
    // Whether read() currently returns the published physical state.
    bool delivers_physical() const { return !stopped && !latched && !scripted() && focused; }
    void publish(State value, bool physical_event, bool stop, uint64_t now, bool has_focus = true, Events* events = nullptr) {
        now = clock(now);
        physical = value;
        focused = has_focus;
        stopped |= stop;
        if (scripted() && (physical.nonneutral() || physical_event || stop))
            finish(stop ? "abort=quit" : "abort=physical-input", now, events);
        else if (latched && !physical.nonneutral() && !physical_event) latched = false;
        expire(now, events);
    }
    State read(uint64_t now, bool quit = false, Events* events = nullptr) {
        now = clock(now);
        if (quit || stopped) return {}; // Shutdown never advances or counts delivery.
        expire(now, events);
        if (latched) return {};
        if (!scripted()) return focused ? physical : State{};
        if (!begun) {
            begun = true;
            run_at = now;
            emit(events, {Receipt::Start});
        }
        if (!awaiting_delivery && reads >= 2 && now - step_at >= plan.steps[index].ms) {
            next_step(now, events);
            if (!scripted()) return {};
        }
        if (awaiting_delivery) {
            step_at = now; awaiting_delivery = false;
            emit(events, {Receipt::StepEntry, &plan.steps[index], nullptr, index});
        }
        if (reads != UINT64_MAX) ++reads;
        return plan.steps[index].state;
    }
};
}
