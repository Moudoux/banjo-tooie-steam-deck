#include "level_minimap.hpp"
#include "map_names.hpp"

#include "imgui/imgui.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <iterator>
#include <memory>
#include <mutex>
#include <unordered_map>
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
// Actors: a Vector (include/vector.h: element size, begin, end, capacity end)
// whose pointer is at 0x80136EE0; func_80106668 pushes each 0x9C-byte actor.
// Every actor of the loaded map is in it, not only those near the player, and
// one leaves it when collected. func_80107C2C (the descriptors' spawner) puts
// the position (f32 x3) at +0x04 and the descriptor's actor id (u16 +0x02, the
// gemarkersDll table index) in bits 9..20 of +0x6C.
constexpr std::uint32_t k_actors = 0x80136EE0U, k_actor_size = 0x9CU, k_actor_limit = 0x400U;
// Game state, D_80127630 +0x02 (func_800A759C): 3 in play, 4 paused, with
// gcnewpause's instance at 0x80128AF0 (func_800C08F8 / func_800C0920).
constexpr std::uint32_t k_game_state = 0x80127632U, k_pause_menu = 0x80128AF0U;
constexpr std::uint32_t k_rdram_first = 0x80000400U, k_rdram_end = 0x80800000U;

// World units from the centre to the edge, per zoom level.
constexpr float k_zoom_radius[tooie::level_minimap::zoom_levels] = {6000, 4500, 3000, 2000, 1500, 1000};
constexpr const char* k_zoom_labels[tooie::level_minimap::zoom_levels] = {"0.5x", "0.67x", "1x", "1.5x", "2x", "3x"};
constexpr float k_full_zoom_step = 1.5f;
constexpr int k_full_zoom_max = 6;
constexpr float k_above = 300.0f;  // Floors this far over the player are faded.
constexpr float k_marker_level = 400.0f; // Markers this far above or below are dimmed.
constexpr auto k_stale = std::chrono::milliseconds(300);
// Explored ground: square cells, those whose centre is within reach of the player
// marked; one row's neighbours merge into a run unless their heights differ.
constexpr float k_cell = 50.0f, k_reach = 150.0f, k_run_step = 200.0f;
constexpr auto k_explored_publish = std::chrono::milliseconds(100);

// Marker kinds by actor id (gemarkersDll table: overlay that returns the descriptor).
enum class Kind : std::uint8_t { MumboSkull, Wigwam, WarpPad, MumboPad, Jinjo, NoteNest, TrebleClef, CheatoPage, Jiggy, Count };
struct KindInfo { std::uint16_t actor; const char* label; };
constexpr KindInfo k_kinds[] = {
    {0, "Mumbo's Skull"},     // Level geometry: k_doors.
    {0, "Wumba's Wigwam"},    // Level geometry: k_doors.
    {0x1E5, "Warp pad"},      // chwarppad
    {0x2B0, "Mumbo pad"},     // chmumbopad
    {0x1F4, "Jinjo"},         // chjinjo
    {0x1D7, "Note nest"},     // chnests (16 in each world)
    {0x1D8, "Treble Clef"},   // chnests (one in each world)
    {0x136, "Cheato page"},   // chcheatopage
    {0x21F, "Jiggy"},         // chjigsaw
};
static_assert(std::size(k_kinds) == std::size_t(Kind::Count));
// Mumbo's Skull and Wumba's Wigwam are level geometry, not actors: where the player
// stands after walking out of each one's map (measured with local/minimap/capture.py,
// owner's save, 2026-10-25). The wigwam that grows in Terrydactyland uses the small one's door.
struct Door { std::uint16_t map; Kind kind; float x, y, z; };
constexpr Door k_doors[] = {
    {0x0B8, Kind::MumboSkull, 2985, 2100, 4056},   // Mayahem Temple (from 0x0B7).
    {0x0C4, Kind::Wigwam, 6348, -1060, 6435},      // Jade Snake Grove (0x0B6).
    {0x0C7, Kind::MumboSkull, 9233, 1283, 3114},   // Glitter Gulch Mine (0x0D9).
    {0x0C7, Kind::Wigwam, -3377, 734, 1833},       // (0x0E9).
    {0x0E7, Kind::MumboSkull, 3347, 112, -3012},   // Witchyworld, The Inferno (0x176).
    {0x0D6, Kind::Wigwam, 7325, 1150, -3348},      // Witchyworld (0x0D5).
    {0x1A7, Kind::MumboSkull, -5912, 9700, -10225}, // Jolly Roger's Lagoon (0x0EF).
    {0x1A8, Kind::Wigwam, -17334, -236, 8942},     // Atlantis (0x120).
    {0x112, Kind::MumboSkull, 11818, 183, 3666},   // Terrydactyland (0x171).
    {0x112, Kind::Wigwam, 107, 1786, 2185},        // (0x11E; 0x122's door is at 250, 2759).
    {0x108, Kind::MumboSkull, -5173, 100, 4246},   // Grunty Industries floor 3 (0x172).
    {0x106, Kind::Wigwam, 1173, -100, 3410},       // Grunty Industries floor 2 (0x11F).
    {0x127, Kind::MumboSkull, -6421, 1500, -1678}, // Hailfire Peaks lava side (0x134).
    {0x128, Kind::Wigwam, -5150, -98, -3256},      // Hailfire Peaks icy side (0x135).
    {0x136, Kind::MumboSkull, -9, 3350, -7541},    // Cloud Cuckooland (0x13E).
    {0x136, Kind::Wigwam, 14910, -1000, 6359},     // (0x140).
    {0x155, Kind::MumboSkull, 3215, 1250, 4799},   // Isle o' Hags, Cliff Top (0x156).
    {0x154, Kind::Wigwam, -3748, 0, -310},         // Isle o' Hags, Pine Grove (0x157).
};

struct Floor { float x[3], z[3], y, y_min; bool water; };
struct Wall { float x0, z0, x1, z1, y_min, y_max; };
struct Mesh { std::vector<Floor> floors; std::vector<Wall> walls; }; // Floors low to high.
struct Marker {
    Kind kind; float x, y, z;
    bool operator==(const Marker&) const = default;
};
struct Run { int row, first, last; float y; }; // Explored cells first..last of a row.
struct Source { std::uint32_t collision = 0, vertices = 0; float offset[3]{}; bool water = false; };
struct Pose { float x = 0, y = 0, z = 0, yaw = 0, view_yaw = 0; std::uint16_t map = 0; std::chrono::steady_clock::time_point at{}; };
// Full-screen map: zoom steps over the fitted view, the view centre (u, v) in map
// coordinates (right, up from the player, world units), the fitted extent
// (centre, half sizes) and the half width the last frame showed.
struct Full {
    bool open = false, fitted = false; int zoom = 0;
    float u = 0, v = 0, fit_u = 0, fit_v = 0, half_u = 1, half_v = 1, view = 1;
};

std::atomic<bool> on{true};
std::atomic<int> zoom_level{tooie::level_minimap::default_zoom};
std::atomic<std::int64_t> updated_at{0}; // steady_clock ticks of the last pose.
std::atomic<bool> game_paused{false};
std::atomic<bool> explored_on{false}, explored_clear{false};
std::atomic<std::uint16_t> screen_map{0};
std::mutex mutex;
std::shared_ptr<const Mesh> published; // Guarded by mutex, with pose, markers and full.
std::shared_ptr<const std::vector<Marker>> markers;
std::shared_ptr<const std::vector<Run>> explored_runs; // The current map's.
Pose pose;
Full full;
// Guest thread only.
std::vector<std::uint32_t> signature;
std::vector<Marker> sampled;
// Per map: cell (row << 32 | column) -> the player's height when first there.
std::unordered_map<std::uint16_t, std::unordered_map<std::uint64_t, float>> visited;
bool visited_changed = false;
std::uint16_t runs_map = 0xFFFF;
std::chrono::steady_clock::time_point runs_at{};

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

void sample_markers(std::uint8_t* rdram, std::uint16_t map, std::vector<Marker>& out) {
    out.clear();
    for (const auto& door : k_doors) if (door.map == map) out.push_back({door.kind, door.x, door.y, door.z});
    const auto vector = word(rdram, k_actors);
    if (!guest_range(vector, 0x10U) || word(rdram, vector) != k_actor_size) return;
    const auto begin = word(rdram, vector + 4U), end = word(rdram, vector + 8U);
    if (end < begin || (end - begin) % k_actor_size || (end - begin) / k_actor_size > k_actor_limit ||
        !guest_range(begin, end - begin)) return;
    for (auto actor = begin; actor < end; actor += k_actor_size) {
        const auto id = (word(rdram, actor + 0x6CU) >> 9) & 0xFFFU;
        const auto* kind = std::find_if(std::begin(k_kinds), std::end(k_kinds), [id](const KindInfo& k) { return k.actor && k.actor == id; });
        if (kind == std::end(k_kinds)) continue;
        const Marker marker{Kind(kind - std::begin(k_kinds)), real(rdram, actor + 4U), real(rdram, actor + 8U), real(rdram, actor + 12U)};
        if (std::isfinite(marker.x) && std::isfinite(marker.y) && std::isfinite(marker.z)) out.push_back(marker);
    }
}

std::uint64_t cell_key(int row, int column) noexcept {
    return (std::uint64_t(std::uint32_t(row)) << 32) | std::uint32_t(column);
}
void mark_explored(const Pose& at) {
    auto& cells = visited[at.map];
    const int r0 = int(std::floor((at.z - k_reach) / k_cell)), r1 = int(std::floor((at.z + k_reach) / k_cell));
    const int c0 = int(std::floor((at.x - k_reach) / k_cell)), c1 = int(std::floor((at.x + k_reach) / k_cell));
    for (int row = r0; row <= r1; ++row) for (int column = c0; column <= c1; ++column) {
        const float dx = (column + 0.5f) * k_cell - at.x, dz = (row + 0.5f) * k_cell - at.z;
        if (dx * dx + dz * dz <= k_reach * k_reach && cells.try_emplace(cell_key(row, column), at.y).second)
            visited_changed = true;
    }
}
std::shared_ptr<const std::vector<Run>> explored_rows(std::uint16_t map) {
    auto runs = std::make_shared<std::vector<Run>>();
    const auto found = visited.find(map);
    if (found == visited.end()) return runs;
    std::vector<std::pair<std::uint64_t, float>> cells(found->second.begin(), found->second.end());
    // Signed rows and columns sort as their biased values.
    const auto order = [](std::uint64_t key) { return key ^ 0x8000000080000000ULL; };
    std::sort(cells.begin(), cells.end(), [&](const auto& a, const auto& b) { return order(a.first) < order(b.first); });
    for (const auto& [key, y] : cells) {
        const int row = int(std::int32_t(key >> 32)), column = int(std::int32_t(key));
        if (!runs->empty()) {
            auto& run = runs->back();
            if (run.row == row && run.last + 1 == column && std::abs(run.y - y) < k_run_step) { run.last = column; continue; }
        }
        runs->push_back({row, column, column, y});
    }
    return runs;
}

// Map frame: (u, v) = (right, up) from the player, rotated so the camera looks up.
struct View {
    ImVec2 lo, hi, centre;
    float k, u, v, sine, cosine; // Pixels per unit; the map point at the centre.
    Pose at;
    ImVec2 project(float x, float z) const {
        const float dx = x - at.x, dz = z - at.z;
        return ImVec2(centre.x + (dz * sine - dx * cosine - u) * k, centre.y - (dx * sine + dz * cosine - v) * k);
    }
    bool outside(const ImVec2& a, const ImVec2& b, const ImVec2& c) const {
        return std::max({a.x, b.x, c.x}) < lo.x || std::min({a.x, b.x, c.x}) > hi.x ||
            std::max({a.y, b.y, c.y}) < lo.y || std::min({a.y, b.y, c.y}) > hi.y;
    }
};

ImU32 alpha(ImU32 colour, float scale) {
    const auto a = static_cast<ImU32>(((colour >> IM_COL32_A_SHIFT) & 0xFF) * scale);
    return (colour & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

// One icon, about 2r across, centred on p.
void draw_icon(ImDrawList* draw, Kind kind, ImVec2 p, float r, float fade) {
    const ImU32 edge = alpha(IM_COL32(20, 16, 10, 255), fade);
    const float line = std::max(1.0f, r * 0.3f);
    switch (kind) {
    case Kind::WarpPad:
        draw->AddCircleFilled(p, r, alpha(IM_COL32(20, 60, 90, 220), fade));
        draw->AddCircle(p, r, alpha(IM_COL32(90, 210, 255, 255), fade), 0, line);
        draw->AddCircleFilled(p, r * 0.35f, alpha(IM_COL32(90, 210, 255, 255), fade));
        break;
    case Kind::MumboSkull:
        // A skull: bone-white head and jaw, dark eyes.
        draw->AddRectFilled(ImVec2(p.x - r * 0.5f, p.y + r * 0.2f), ImVec2(p.x + r * 0.5f, p.y + r * 0.95f), alpha(IM_COL32(235, 228, 205, 255), fade), r * 0.2f);
        draw->AddCircleFilled(ImVec2(p.x, p.y - r * 0.15f), r * 0.85f, alpha(IM_COL32(235, 228, 205, 255), fade));
        draw->AddCircle(ImVec2(p.x, p.y - r * 0.15f), r * 0.85f, edge, 0, line * 0.6f);
        draw->AddCircleFilled(ImVec2(p.x - r * 0.33f, p.y - r * 0.15f), r * 0.24f, edge);
        draw->AddCircleFilled(ImVec2(p.x + r * 0.33f, p.y - r * 0.15f), r * 0.24f, edge);
        break;
    case Kind::MumboPad:
        draw->AddCircleFilled(p, r, alpha(IM_COL32(60, 25, 80, 220), fade));
        draw->AddCircle(p, r, alpha(IM_COL32(200, 120, 255, 255), fade), 0, line);
        break;
    case Kind::Wigwam: {
        const ImVec2 top(p.x, p.y - r * 1.1f), left(p.x - r, p.y + r * 0.8f), right(p.x + r, p.y + r * 0.8f);
        draw->AddTriangleFilled(top, left, right, alpha(IM_COL32(70, 205, 180, 255), fade));
        draw->AddTriangle(top, left, right, edge, line * 0.6f);
        break;
    }
    case Kind::Jinjo:
        draw->AddCircleFilled(p, r * 0.8f, alpha(IM_COL32(255, 110, 200, 255), fade));
        draw->AddCircle(p, r * 0.8f, edge, 0, line * 0.6f);
        break;
    case Kind::NoteNest: case Kind::TrebleClef: {
        // A quaver: head, stem and flag; the Treble Clef is larger and outlined white.
        const float s = kind == Kind::TrebleClef ? r * 1.4f : r;
        const ImU32 fill = alpha(kind == Kind::TrebleClef ? IM_COL32(255, 225, 90, 255) : IM_COL32(255, 160, 40, 255), fade);
        const ImVec2 head(p.x - s * 0.25f, p.y + s * 0.45f), stem_top(p.x + s * 0.2f, p.y - s * 0.9f);
        if (kind == Kind::TrebleClef) {
            draw->AddCircleFilled(head, s * 0.55f, alpha(IM_COL32(255, 255, 255, 255), fade));
            draw->AddLine(ImVec2(stem_top.x, head.y), stem_top, alpha(IM_COL32(255, 255, 255, 255), fade), line * 2.2f);
        }
        draw->AddCircleFilled(head, s * 0.42f, fill);
        draw->AddLine(ImVec2(stem_top.x, head.y), stem_top, fill, line);
        draw->AddLine(stem_top, ImVec2(stem_top.x + s * 0.45f, stem_top.y + s * 0.4f), fill, line);
        break;
    }
    case Kind::CheatoPage:
        draw->AddRectFilled(ImVec2(p.x - r * 0.7f, p.y - r * 0.9f), ImVec2(p.x + r * 0.7f, p.y + r * 0.9f), alpha(IM_COL32(240, 235, 220, 255), fade));
        draw->AddRect(ImVec2(p.x - r * 0.7f, p.y - r * 0.9f), ImVec2(p.x + r * 0.7f, p.y + r * 0.9f), alpha(IM_COL32(200, 40, 40, 255), fade), 0.0f, 0, line * 0.8f);
        break;
    case Kind::Jiggy: {
        // A diamond, gold with a dark edge.
        const ImVec2 a(p.x, p.y - r * 1.1f), b(p.x + r * 1.1f, p.y), c(p.x, p.y + r * 1.1f), d(p.x - r * 1.1f, p.y);
        draw->AddQuadFilled(a, b, c, d, alpha(IM_COL32(255, 205, 40, 255), fade));
        draw->AddQuad(a, b, c, d, edge, line * 0.8f);
        break;
    }
    case Kind::Count: break;
    }
}

void text(ImDrawList* draw, ImVec2 at, const char* label, float size, ImU32 colour = IM_COL32(240, 244, 248, 255)) {
    auto* font = ImGui::GetFont();
    draw->AddText(font, size, ImVec2(at.x + 1, at.y + 1), IM_COL32(0, 0, 0, 200), label);
    draw->AddText(font, size, at, colour, label);
}

// Floors, walls, markers and the player arrow, clipped to the view.
void draw_map(ImDrawList* draw, const View& view, const Mesh& mesh, const std::vector<Marker>& marks,
    const std::vector<Run>* explored, float icon) {
    const Pose& at = view.at;
    draw->PushClipRect(view.lo, view.hi, true);
    // Adjacent triangles would show seams with anti-aliased fringes.
    const auto flags = draw->Flags;
    draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;
    for (int pass = 0; pass < 2; ++pass) for (const auto& floor : mesh.floors) {
        const bool above = floor.y_min > at.y + k_above;
        if (above != (pass == 0)) continue; // Floors over the player first, faded.
        const ImVec2 a = view.project(floor.x[0], floor.z[0]), b = view.project(floor.x[1], floor.z[1]), c = view.project(floor.x[2], floor.z[2]);
        if (view.outside(a, b, c)) continue;
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
    // Explored ground over the floors, under the walls; dimmed well above or below the player.
    if (explored) for (const auto& run : *explored) {
        const float x0 = run.first * k_cell, x1 = (run.last + 1) * k_cell, z0 = run.row * k_cell, z1 = z0 + k_cell;
        const ImVec2 a = view.project(x0, z0), b = view.project(x1, z0), c = view.project(x1, z1), d = view.project(x0, z1);
        if (view.outside(a, b, c) && view.outside(a, c, d)) continue;
        draw->AddQuadFilled(a, b, c, d, std::abs(run.y - at.y) > k_marker_level ? IM_COL32(255, 140, 50, 45) : IM_COL32(255, 140, 50, 115));
    }
    draw->Flags = flags;
    const float size = std::min(view.hi.x - view.lo.x, view.hi.y - view.lo.y);
    const float line = std::max(1.0f, size / 180.0f);
    for (const auto& wall : mesh.walls) {
        if (wall.y_max < at.y - 150.0f || wall.y_min > at.y + 450.0f) continue;
        const ImVec2 a = view.project(wall.x0, wall.z0), b = view.project(wall.x1, wall.z1);
        if (view.outside(a, b, b)) continue;
        draw->AddLine(a, b, IM_COL32(225, 230, 235, 170), std::min(line, 2.0f));
    }
    // Collectibles over pads; those well above or below the player dimmed.
    for (int kind = 0; kind < int(Kind::Count); ++kind) for (const auto& mark : marks) {
        if (int(mark.kind) != kind) continue;
        const ImVec2 p = view.project(mark.x, mark.z);
        if (p.x < view.lo.x - icon || p.x > view.hi.x + icon || p.y < view.lo.y - icon || p.y > view.hi.y + icon) continue;
        draw_icon(draw, mark.kind, p, icon, std::abs(mark.y - at.y) > k_marker_level ? 0.45f : 1.0f);
    }
    // Facing arrow: the player's yaw relative to the view.
    const float turn = (std::remainder(at.yaw, 360.0f) * 0.01745329252f) - std::atan2(view.sine, view.cosine);
    const ImVec2 me = view.project(at.x, at.z);
    const float r = std::clamp(size / 22.0f, 6.0f, 16.0f), fx = -std::sin(turn), fy = -std::cos(turn);
    const ImVec2 tip(me.x + fx * r * 1.4f, me.y + fy * r * 1.4f);
    const ImVec2 left(me.x + (-fx * 0.8f - fy * 0.9f) * r, me.y + (-fy * 0.8f + fx * 0.9f) * r);
    const ImVec2 right(me.x + (-fx * 0.8f + fy * 0.9f) * r, me.y + (-fy * 0.8f - fx * 0.9f) * r);
    draw->AddTriangleFilled(tip, left, right, IM_COL32(255, 214, 64, 255));
    draw->AddTriangle(tip, left, right, IM_COL32(40, 30, 10, 255), line);
    draw->PopClipRect();
    draw->AddRect(view.lo, view.hi, IM_COL32(190, 207, 218, 160), 0.0f, 0, line);
}

View make_view(ImVec2 lo, ImVec2 hi, float k, float u, float v, const Pose& at) {
    const float yaw = std::remainder(at.view_yaw, 360.0f) * 0.01745329252f;
    return {lo, hi, ImVec2((lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f), k, u, v, std::sin(yaw), std::cos(yaw), at};
}

// The full map's extent: floors and markers in map coordinates.
void fit(Full& out, const Mesh& mesh, const std::vector<Marker>& marks, const Pose& at) {
    const View view = make_view({}, {}, 1.0f, 0.0f, 0.0f, at);
    float u0 = -1000, u1 = 1000, v0 = -1000, v1 = 1000; // The player is always in view.
    const auto add = [&](float x, float z) {
        const ImVec2 p = view.project(x, z); // k = 1 at the origin: (u, -v).
        u0 = std::min(u0, p.x); u1 = std::max(u1, p.x); v0 = std::min(v0, -p.y); v1 = std::max(v1, -p.y);
    };
    for (const auto& floor : mesh.floors) for (int i = 0; i < 3; ++i) add(floor.x[i], floor.z[i]);
    for (const auto& mark : marks) add(mark.x, mark.z);
    out.u = out.fit_u = (u0 + u1) * 0.5f; out.v = out.fit_v = (v0 + v1) * 0.5f;
    out.half_u = (u1 - u0) * 0.5f; out.half_v = (v1 - v0) * 0.5f;
    out.fitted = true;
}

void render_full(ImDrawList* draw, const Mesh& mesh, const std::vector<Marker>& marks, const std::vector<Run>* explored, const Pose& at) {
    Full view_state;
    {
        std::lock_guard lock(mutex);
        if (!full.fitted) fit(full, mesh, marks, at);
        view_state = full;
    }
    const auto* viewport = ImGui::GetMainViewport();
    const float margin = std::max(12.0f, viewport->WorkSize.y * 0.03f), font = ImGui::GetFontSize();
    const float title = font * 1.5f, footer = font * 3.2f;
    const ImVec2 lo(viewport->WorkPos.x + margin, viewport->WorkPos.y + margin + title + font * 0.5f);
    const ImVec2 hi(viewport->WorkPos.x + viewport->WorkSize.x - margin, viewport->WorkPos.y + viewport->WorkSize.y - margin - footer);
    if (hi.x - lo.x < 64.0f || hi.y - lo.y < 64.0f) return;
    draw->AddRectFilled(viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y),
        IM_COL32(6, 10, 14, 235));
    const float zoom = std::pow(k_full_zoom_step, float(view_state.zoom));
    const float k = std::min((hi.x - lo.x) * 0.5f / view_state.half_u, (hi.y - lo.y) * 0.5f / view_state.half_v) * 0.95f * zoom;
    { std::lock_guard lock(mutex); full.view = (hi.x - lo.x) * 0.5f / k; }
    const View view = make_view(lo, hi, k, view_state.u, view_state.v, at);
    draw->AddRectFilled(lo, hi, IM_COL32(12, 20, 28, 255));
    const float icon = std::clamp((hi.y - lo.y) / 70.0f, 5.0f, 11.0f);
    draw_map(draw, view, mesh, marks, explored, icon);
    const char* name = tooie::map_names::name(at.map);
    text(draw, ImVec2(lo.x, viewport->WorkPos.y + margin), name ? name : "Map", title);
    // Legend: the kinds this map has, then the controls.
    bool present[std::size_t(Kind::Count)]{};
    for (const auto& mark : marks) present[std::size_t(mark.kind)] = true;
    ImVec2 at_legend(lo.x, hi.y + font * 0.5f);
    if (explored && !explored->empty()) {
        const char* label = "Explored";
        draw->AddRectFilled(ImVec2(at_legend.x, at_legend.y + font * 0.5f - icon * 0.8f),
            ImVec2(at_legend.x + icon * 2.0f, at_legend.y + font * 0.5f + icon * 0.8f), IM_COL32(255, 140, 50, 170));
        text(draw, ImVec2(at_legend.x + icon * 2.0f + font * 0.4f, at_legend.y), label, font);
        at_legend.x += icon * 2.0f + font * 0.4f + ImGui::CalcTextSize(label).x + font;
    }
    for (int kind = int(Kind::Count) - 1; kind >= 0; --kind) {
        if (!present[kind]) continue;
        const char* label = k_kinds[kind].label;
        const float width = icon * 2.0f + font * 0.4f + ImGui::CalcTextSize(label).x + font;
        if (at_legend.x + width > hi.x) break;
        draw_icon(draw, Kind(kind), ImVec2(at_legend.x + icon, at_legend.y + font * 0.5f), icon, 1.0f);
        text(draw, ImVec2(at_legend.x + icon * 2.0f + font * 0.4f, at_legend.y), label, font);
        at_legend.x += width;
    }
    text(draw, ImVec2(lo.x, hi.y + font * 1.8f), "A / B zoom    Stick move    Start or F5 back to the pause menu    F5 + X explored    F5 + Y clear", font,
        IM_COL32(190, 200, 210, 255));
}
}

namespace tooie::level_minimap {

void set_enabled(bool enabled) noexcept { on.store(enabled, std::memory_order_release); }
bool enabled() noexcept { return on.load(std::memory_order_acquire); }
const char* zoom_label(int level) noexcept { return k_zoom_labels[std::clamp(level, 0, zoom_levels - 1)]; }
void set_zoom(int level) noexcept { zoom_level.store(std::clamp(level, 0, zoom_levels - 1), std::memory_order_release); }
int zoom() noexcept { return zoom_level.load(std::memory_order_acquire); }

bool visible() noexcept {
    const auto at = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(updated_at.load(std::memory_order_acquire)));
    return enabled() && !game_paused.load(std::memory_order_acquire) && std::chrono::steady_clock::now() - at <= k_stale;
}
bool paused() noexcept {
    if (!enabled() || !game_paused.load(std::memory_order_acquire)) return false;
    std::lock_guard lock(mutex);
    return published && pose.map == screen_map.load(std::memory_order_acquire);
}

void set_full_map(bool open) noexcept {
    std::lock_guard lock(mutex);
    if (open == full.open) return;
    full = {};
    full.open = open;
}
bool full_map() noexcept { std::lock_guard lock(mutex); return full.open; }
void set_explored(bool enabled) noexcept { explored_on.store(enabled, std::memory_order_release); }
bool explored() noexcept { return explored_on.load(std::memory_order_acquire); }
void clear_explored() noexcept { explored_clear.store(true, std::memory_order_release); }
void zoom_full_map(int steps) noexcept {
    std::lock_guard lock(mutex);
    full.zoom = std::clamp(full.zoom + steps, 0, k_full_zoom_max);
}
void pan_full_map(float right, float up, float seconds) noexcept {
    std::lock_guard lock(mutex);
    if (!full.open || !full.fitted) return;
    full.u = std::clamp(full.u + right * seconds * full.view, full.fit_u - full.half_u, full.fit_u + full.half_u);
    full.v = std::clamp(full.v + up * seconds * full.view, full.fit_v - full.half_v, full.fit_v + full.half_v);
}

void render() {
    if (!enabled()) return;
    std::shared_ptr<const Mesh> mesh;
    std::shared_ptr<const std::vector<Marker>> marks;
    std::shared_ptr<const std::vector<Run>> runs;
    Pose at;
    bool open;
    { std::lock_guard lock(mutex); mesh = published; marks = markers; runs = explored_runs; at = pose; open = full.open; }
    if (!mesh) return;
    const auto* explored_ground = explored() && runs && !runs->empty() ? runs.get() : nullptr;
    static const std::vector<Marker> none;
    const auto& marked = marks ? *marks : none;
    auto* draw = ImGui::GetBackgroundDrawList();
    const auto* viewport = ImGui::GetMainViewport();
    const float size = std::clamp(viewport->WorkSize.y * 0.3f, 120.0f, 360.0f), margin = 16.0f;
    const ImVec2 lo(viewport->WorkPos.x + margin, viewport->WorkPos.y + viewport->WorkSize.y - margin - size);
    if (paused()) {
        if (open) render_full(draw, *mesh, marked, explored_ground, at);
        else text(draw, ImVec2(lo.x, viewport->WorkPos.y + viewport->WorkSize.y - margin - ImGui::GetFontSize()), "F5: Map",
            ImGui::GetFontSize());
        return;
    }
    if (std::chrono::steady_clock::now() - at.at > k_stale) return;
    const ImVec2 hi(lo.x + size, lo.y + size);
    draw->AddRectFilled(lo, hi, IM_COL32(12, 20, 28, 200));
    draw_map(draw, make_view(lo, hi, size * 0.5f / k_zoom_radius[zoom()], 0.0f, 0.0f, at), *mesh, marked,
        explored_ground, std::clamp(size / 36.0f, 3.5f, 8.0f));
    if (const char* name = tooie::map_names::name(at.map)) {
        // Above the map, wrapped to its width.
        const float font = ImGui::GetFontSize();
        const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(font, FLT_MAX, size, name);
        draw->PushClipRect(ImVec2(lo.x, lo.y - extent.y - 6.0f), ImVec2(hi.x, lo.y), true);
        auto* font_data = ImGui::GetFont();
        draw->AddText(font_data, font, ImVec2(lo.x + 1, lo.y - extent.y - 3.0f), IM_COL32(0, 0, 0, 200), name, nullptr, size);
        draw->AddText(font_data, font, ImVec2(lo.x, lo.y - extent.y - 4.0f), IM_COL32(240, 244, 248, 255), name, nullptr, size);
        draw->PopClipRect();
    }
}

void observe_screen(const std::uint8_t* rdram) noexcept {
    if (!rdram || !enabled()) return;
    const auto at = [rdram](std::uint32_t address) { return rdram + (address - 0x80000000U); };
    const auto state = *reinterpret_cast<const std::int16_t*>(at(k_game_state ^ 2U));
    const auto menu = *reinterpret_cast<const std::uint32_t*>(at(k_pause_menu));
    game_paused.store(state == 4 && menu != 0, std::memory_order_release);
    screen_map.store(*reinterpret_cast<const std::uint16_t*>(at(k_current_map ^ 2U)), std::memory_order_release);
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
    next.map = static_cast<std::uint16_t>(half(rdram, k_current_map));
    next.at = std::chrono::steady_clock::now();

    // Rebuild when the map or a collision source changes (a load, a section, or
    // the heap moving a model). Vertex lists can swap buffers every frame.
    const auto list = sources(rdram);
    std::vector<std::uint32_t> current{next.map};
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
    sample_markers(rdram, next.map, sampled);
    if (explored_clear.exchange(false, std::memory_order_acq_rel)) { visited.clear(); visited_changed = true; }
    if (tooie::level_minimap::explored()) mark_explored(next);
    std::shared_ptr<const std::vector<Run>> runs;
    if (next.map != runs_map || (visited_changed && next.at - runs_at >= k_explored_publish)) {
        runs = explored_rows(next.map);
        runs_map = next.map; runs_at = next.at; visited_changed = false;
    }
    std::lock_guard lock(mutex);
    if (rebuilt) published = std::move(rebuilt);
    if (runs) explored_runs = std::move(runs);
    if (!markers || *markers != sampled) markers = std::make_shared<const std::vector<Marker>>(sampled);
    pose = next;
    updated_at.store(next.at.time_since_epoch().count(), std::memory_order_release);
}
