#include "level_minimap.hpp"

#include "imgui/imgui.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace {
// Guest layout, read only (Banjo-Kazooie's model format, which Tooie keeps):
// - 0x801285B0: two 0x2C-byte level layers, opaque then translucent (water).
//   func_800BEE40 queries a layer when u8 +0x2A is set, with its collision
//   list at +0x00 and its live vertex list at +0x10 + 4 * u8 +0x29.
// - +0x77 / +0x68 of the first layer: map sections (gcmapsects_entrypoint_21),
//   0x58 bytes each, live when bit 30 of +0x4C is set; the model is asset
//   u16 +0x40 and the vec3 at +0x00 offsets its geometry.
// - The asset cache (func_800D6254): s16 count at 0x8012BE62, 12-byte entries
//   at 0x8012B800 sorted by id (word 2 >> 18), data pointer in word 1.
// - Model: collision list at +0x1C, vertex list at +0x10 (offsets).
//   Collision list: s16 cell count +0x10, triangle count +0x14, u16 pairs from
//   +0x18, then 12-byte triangles (s16 vertex indices). Vertex list: s16 count
//   +0x14, 16-byte N64 vertices (s16 x, y, z) from +0x18.
constexpr std::uint32_t k_layers = 0x801285B0U, k_layer_size = 0x2CU;
constexpr std::uint32_t k_cache = 0x8012B800U, k_cache_count = 0x8012BE62U;
constexpr std::uint32_t k_current_map = 0x80132DC2U; // func_800EA05C.
// Upstream's radar (src/minimap.cpp): controlled index, PlayerState table,
// position (+0xE4) and yaw (+0xF8) pointers. Camera: yaw at +0x1C; the camera
// looks along -(sin, cos) of it, the player faces +(sin, cos) of its yaw.
constexpr std::uint32_t k_controlled = 0x801354DCU, k_players = 0x80135490U;
constexpr std::uint32_t k_camera = 0x8012D500U;
constexpr std::uint32_t k_rdram_first = 0x80000400U, k_rdram_end = 0x80800000U;

constexpr float k_view_radius = 3000.0f; // World units from the centre to the edge.
constexpr float k_above = 300.0f;        // Floors this far over the player are faded.
constexpr auto k_stale = std::chrono::milliseconds(300);

struct Floor { float x[3], z[3], y, y_min; bool water; };
struct Wall { float x0, z0, x1, z1, y_min, y_max; };
struct Mesh { std::vector<Floor> floors; std::vector<Wall> walls; }; // Floors low to high.
struct Source { std::uint32_t collision = 0, vertices = 0; float offset[3]{}; bool water = false; };
struct Pose { float x = 0, y = 0, z = 0, yaw = 0, view_yaw = 0; std::chrono::steady_clock::time_point at{}; };

std::atomic<bool> on{true};
std::mutex mutex;
std::shared_ptr<const Mesh> published; // Guarded by mutex, with pose.
Pose pose;
// Guest thread only.
std::vector<std::uint32_t> signature;

bool guest_range(std::uint32_t address, std::uint32_t bytes) noexcept {
    return address >= k_rdram_first && bytes <= k_rdram_end - k_rdram_first &&
        address <= k_rdram_end - bytes;
}
gpr guest(std::uint32_t address) noexcept { return static_cast<gpr>(static_cast<std::int32_t>(address)); }
std::uint32_t word(std::uint8_t* rdram, std::uint32_t address) noexcept {
    return static_cast<std::uint32_t>(MEM_W(0, guest(address)));
}
std::int16_t half(std::uint8_t* rdram, std::uint32_t address) noexcept { return MEM_H(0, guest(address)); }
std::uint8_t byte(std::uint8_t* rdram, std::uint32_t address) noexcept { return MEM_BU(0, guest(address)); }
float real(std::uint8_t* rdram, std::uint32_t address) noexcept { return std::bit_cast<float>(word(rdram, address)); }

std::uint32_t cached_asset(std::uint8_t* rdram, std::uint32_t id) noexcept {
    const int count = std::min<int>(half(rdram, k_cache_count), 0x87);
    for (int i = 0; i < count; ++i)
        if (word(rdram, k_cache + i * 12U + 8U) >> 18 == id) return word(rdram, k_cache + i * 12U + 4U);
    return 0;
}
bool model_source(std::uint8_t* rdram, std::uint32_t model, Source& out) noexcept {
    if (!guest_range(model, 0x20U) || (model & 3U) || word(rdram, model) != 0x0BU) return false;
    const auto collision = word(rdram, model + 0x1CU), vertices = word(rdram, model + 0x10U);
    if (!collision || !vertices) return false;
    out.collision = model + collision; out.vertices = model + vertices;
    return true;
}
std::vector<Source> sources(std::uint8_t* rdram) noexcept {
    std::vector<Source> out;
    for (std::uint32_t layer = 0; layer < 2; ++layer) {
        const auto base = k_layers + layer * k_layer_size;
        if (!byte(rdram, base + 0x2AU)) continue;
        Source source;
        source.collision = word(rdram, base);
        source.vertices = word(rdram, base + 0x10U + 4U * (byte(rdram, base + 0x29U) & 1U));
        source.water = layer == 1;
        out.push_back(source);
    }
    const auto count = byte(rdram, k_layers + 0x77U);
    const auto sections = word(rdram, k_layers + 0x68U);
    if (count && guest_range(sections, count * 0x58U)) for (std::uint32_t i = 0; i < count; ++i) {
        const auto section = sections + i * 0x58U;
        if (!(word(rdram, section + 0x4CU) & 0x40000000U)) continue;
        Source source;
        if (!model_source(rdram, cached_asset(rdram, static_cast<std::uint16_t>(half(rdram, section + 0x40U))), source))
            continue;
        for (int axis = 0; axis < 3; ++axis) source.offset[axis] = real(rdram, section + 4U * axis);
        out.push_back(source);
    }
    return out;
}

void add_source(std::uint8_t* rdram, const Source& source, Mesh& mesh) {
    const auto collision = source.collision, vertices = source.vertices;
    if (!guest_range(collision, 0x18U) || !guest_range(vertices, 0x18U) || (collision & 3U) || (vertices & 3U)) return;
    const auto cells = static_cast<std::uint16_t>(half(rdram, collision + 0x10U));
    const auto triangles = static_cast<std::uint16_t>(half(rdram, collision + 0x14U));
    const auto vertex_count = static_cast<std::uint16_t>(half(rdram, vertices + 0x14U));
    const auto list = collision + 0x18U + cells * 4U;
    if (!guest_range(list, triangles * 12U) || !guest_range(vertices + 0x18U, vertex_count * 16U)) return;
    std::vector<float> points(vertex_count * 3U);
    for (std::uint32_t i = 0; i < vertex_count; ++i)
        for (std::uint32_t axis = 0; axis < 3; ++axis)
            points[i * 3 + axis] = half(rdram, vertices + 0x18U + i * 16U + axis * 2U) + source.offset[axis];
    // Each grid cell lists the triangles crossing it, so most appear several times.
    std::unordered_set<std::uint64_t> seen;
    for (std::uint32_t t = 0; t < triangles; ++t) {
        std::uint16_t index[3];
        for (std::uint32_t k = 0; k < 3; ++k) index[k] = static_cast<std::uint16_t>(half(rdram, list + t * 12U + k * 2U));
        if (index[0] >= vertex_count || index[1] >= vertex_count || index[2] >= vertex_count) continue;
        std::uint16_t key[3] = {index[0], index[1], index[2]};
        std::sort(key, key + 3);
        if (!seen.insert((std::uint64_t(key[0]) << 32) | (std::uint64_t(key[1]) << 16) | key[2]).second) continue;
        const float* a = &points[index[0] * 3]; const float* b = &points[index[1] * 3]; const float* c = &points[index[2] * 3];
        const float u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        const float n[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
        const float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (length <= 0.0f) continue;
        const float up = n[1] / length;
        const float y_min = std::min({a[1], b[1], c[1]}), y_max = std::max({a[1], b[1], c[1]});
        if (up > 0.3f) {
            mesh.floors.push_back({{a[0], b[0], c[0]}, {a[2], b[2], c[2]}, (a[1] + b[1] + c[1]) / 3.0f, y_min, source.water});
        } else if (up > -0.3f && !source.water) {
            // A wall's footprint is its longest horizontal edge.
            const float* ends[3][2] = {{a, b}, {b, c}, {c, a}};
            int best = 0; float best_length = -1.0f;
            for (int e = 0; e < 3; ++e) {
                const float dx = ends[e][1][0] - ends[e][0][0], dz = ends[e][1][2] - ends[e][0][2];
                if (dx * dx + dz * dz > best_length) { best_length = dx * dx + dz * dz; best = e; }
            }
            mesh.walls.push_back({ends[best][0][0], ends[best][0][2], ends[best][1][0], ends[best][1][2], y_min, y_max});
        } // Ceilings are left out.
    }
}
}

namespace tooie::level_minimap {

void set_enabled(bool enabled) noexcept { on.store(enabled, std::memory_order_release); }
bool enabled() noexcept { return on.load(std::memory_order_acquire); }

void render() {
    if (!enabled()) return;
    std::shared_ptr<const Mesh> mesh;
    Pose at;
    { std::lock_guard lock(mutex); mesh = published; at = pose; }
    if (!mesh || std::chrono::steady_clock::now() - at.at > k_stale) return;
    const auto* viewport = ImGui::GetMainViewport();
    const float size = std::clamp(viewport->WorkSize.y * 0.3f, 120.0f, 360.0f), margin = 16.0f;
    const ImVec2 lo(viewport->WorkPos.x + margin, viewport->WorkPos.y + viewport->WorkSize.y - margin - size);
    const ImVec2 hi(lo.x + size, lo.y + size), centre((lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f);
    const float k = size * 0.5f / k_view_radius;
    const float view = std::remainder(at.view_yaw, 360.0f) * 0.01745329252f;
    const float sine = std::sin(view), cosine = std::cos(view);
    // Camera forward is up; right is forward x up (Y up, right-handed).
    const auto project = [&](float x, float z) {
        const float dx = x - at.x, dz = z - at.z;
        return ImVec2(centre.x + (dz * sine - dx * cosine) * k, centre.y - (dx * sine + dz * cosine) * k);
    };
    const auto outside = [&](const ImVec2& a, const ImVec2& b, const ImVec2& c) {
        return std::max({a.x, b.x, c.x}) < lo.x || std::min({a.x, b.x, c.x}) > hi.x ||
            std::max({a.y, b.y, c.y}) < lo.y || std::min({a.y, b.y, c.y}) > hi.y;
    };
    auto* draw = ImGui::GetBackgroundDrawList();
    draw->PushClipRect(lo, hi, true);
    draw->AddRectFilled(lo, hi, IM_COL32(12, 20, 28, 200));
    // Adjacent triangles would show seams with anti-aliased fringes.
    const auto flags = draw->Flags;
    draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;
    for (int pass = 0; pass < 2; ++pass) for (const auto& floor : mesh->floors) {
        const bool above = floor.y_min > at.y + k_above;
        if (above != (pass == 0)) continue; // Floors over the player first, faded.
        const ImVec2 a = project(floor.x[0], floor.z[0]), b = project(floor.x[1], floor.z[1]), c = project(floor.x[2], floor.z[2]);
        if (outside(a, b, c)) continue;
        ImU32 colour;
        if (above) colour = IM_COL32(150, 160, 170, 45);
        else if (floor.water) colour = IM_COL32(60, 120, 200, 200);
        else {
            // Brighter towards the player's height.
            const float t = std::clamp((floor.y - at.y + 1500.0f) / 1500.0f, 0.0f, 1.0f);
            colour = IM_COL32(int(40 + 110 * t), int(70 + 130 * t), int(50 + 90 * t), 235);
        }
        draw->AddTriangleFilled(a, b, c, colour);
    }
    draw->Flags = flags;
    const float line = std::max(1.0f, size / 180.0f);
    for (const auto& wall : mesh->walls) {
        if (wall.y_max < at.y - 150.0f || wall.y_min > at.y + 450.0f) continue;
        const ImVec2 a = project(wall.x0, wall.z0), b = project(wall.x1, wall.z1);
        if (outside(a, b, b)) continue;
        draw->AddLine(a, b, IM_COL32(225, 230, 235, 170), line);
    }
    // Facing arrow: the player's yaw relative to the view.
    const float turn = (std::remainder(at.yaw, 360.0f) * 0.01745329252f) - view;
    const float r = size / 22.0f, fx = -std::sin(turn), fy = -std::cos(turn);
    const ImVec2 tip(centre.x + fx * r * 1.4f, centre.y + fy * r * 1.4f);
    const ImVec2 left(centre.x + (-fx * 0.8f - fy * 0.9f) * r, centre.y + (-fy * 0.8f + fx * 0.9f) * r);
    const ImVec2 right(centre.x + (-fx * 0.8f + fy * 0.9f) * r, centre.y + (-fy * 0.8f - fx * 0.9f) * r);
    draw->AddTriangleFilled(tip, left, right, IM_COL32(255, 214, 64, 255));
    draw->AddTriangle(tip, left, right, IM_COL32(40, 30, 10, 255), std::max(1.0f, line));
    draw->PopClipRect();
    draw->AddRect(lo, hi, IM_COL32(190, 207, 218, 160), 0.0f, 0, line);
}

} // namespace tooie::level_minimap

extern "C" void tooie_level_minimap_observe(std::uint8_t* rdram, recomp_context* ctx) {
    if (!rdram || !ctx || !tooie::level_minimap::enabled()) return;
    // bainput_update can run once per character; follow the controlled one.
    const auto controlled = word(rdram, k_controlled);
    if (controlled >= 2U) return;
    const auto player = static_cast<std::uint32_t>(ctx->r4);
    if (player != word(rdram, k_players + controlled * 4U) || !guest_range(player, 0x100U)) return;
    const auto position = word(rdram, player + 0xE4U), yaw = word(rdram, player + 0xF8U);
    if (!guest_range(position, 12U) || !guest_range(yaw, 4U)) return;
    Pose next;
    next.x = real(rdram, position); next.y = real(rdram, position + 4U); next.z = real(rdram, position + 8U);
    next.yaw = real(rdram, yaw);
    const auto camera = word(rdram, k_camera);
    next.view_yaw = guest_range(camera, 0x20U) ? real(rdram, camera + 0x1CU) + 180.0f : next.yaw;
    if (!std::isfinite(next.x) || !std::isfinite(next.y) || !std::isfinite(next.z) ||
        !std::isfinite(next.yaw) || !std::isfinite(next.view_yaw)) return;
    next.at = std::chrono::steady_clock::now();

    // Rebuild when the map or a collision source changes (a load, a section, or
    // the heap moving a model). Vertex lists can swap buffers every frame.
    const auto list = sources(rdram);
    std::vector<std::uint32_t> current{static_cast<std::uint16_t>(half(rdram, k_current_map))};
    for (const auto& source : list) {
        current.push_back(source.collision);
        for (float axis : source.offset) current.push_back(std::bit_cast<std::uint32_t>(axis));
    }
    std::shared_ptr<const Mesh> rebuilt;
    if (current != signature) {
        signature = std::move(current);
        auto mesh = std::make_shared<Mesh>();
        for (const auto& source : list) add_source(rdram, source, *mesh);
        std::sort(mesh->floors.begin(), mesh->floors.end(), [](const Floor& a, const Floor& b) { return a.y < b.y; });
        rebuilt = std::move(mesh);
    }
    std::lock_guard lock(mutex);
    if (rebuilt) published = std::move(rebuilt);
    pose = next;
}
