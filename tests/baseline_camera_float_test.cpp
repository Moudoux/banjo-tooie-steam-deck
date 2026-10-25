// Six-F4 BK float camera route: camera_float.h against E443C's documented product (A * R * P, row
// vectors) and the linked RT64 predicates RSP::matrixCommon applies to a PROJECTION LOAD
// (isMatrixViewProj) and MUL (isMatrixAffine, isMatrixIdentity). Synthetic cameras only.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "common/rt64_math.h"
#include "camera_float.h"

namespace {

int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

struct Camera { double yaw, pitch, roll, x, y, z; float scale, fovy, aspect, znear, zfar; };

// CB124 shape: perspective entries, P33 = 0, P23 = -1, then all 16 entries scaled by 0.5.
void perspective(const Camera& c, float p[16]) {
    const float cot = 1.0f / std::tan(c.fovy * 0.5f);
    for (int i = 0; i < 16; i++) p[i] = 0.0f;
    p[0] = cot / c.aspect; p[5] = cot;
    p[10] = (c.znear + c.zfar) / (c.znear - c.zfar); p[11] = -1.0f;
    p[14] = 2.0f * c.znear * c.zfar / (c.znear - c.zfar);
    for (int i = 0; i < 16; i++) p[i] *= 0.5f;
}

// D965C shape: a proper rotation with an exact homogeneous last row and column.
void rotation(const Camera& c, float r[16]) {
    const double cy = std::cos(c.yaw), sy = std::sin(c.yaw), cp = std::cos(c.pitch), sp = std::sin(c.pitch);
    const double cr = std::cos(c.roll), sr = std::sin(c.roll);
    const double ry[3][3] = {{cy, 0, -sy}, {0, 1, 0}, {sy, 0, cy}};
    const double rx[3][3] = {{1, 0, 0}, {0, cp, sp}, {0, -sp, cp}};
    const double rz[3][3] = {{cr, sr, 0}, {-sr, cr, 0}, {0, 0, 1}};
    double t[3][3], m[3][3];
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
        t[i][j] = 0; for (int k = 0; k < 3; k++) t[i][j] += ry[i][k] * rx[k][j];
    }
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
        m[i][j] = 0; for (int k = 0; k < 3; k++) m[i][j] += t[i][k] * rz[k][j];
    }
    for (int i = 0; i < 16; i++) r[i] = 0.0f;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r[i * 4 + j] = (float)m[i][j];
    r[15] = 1.0f;
}

void mul(const double a[16], const double b[16], double out[16]) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
        double v = 0; for (int k = 0; k < 4; k++) v += a[i * 4 + k] * b[k * 4 + j];
        out[i * 4 + j] = v;
    }
}

hlslpp::float4x4 matrix(const volatile float m[16]) {
    return hlslpp::float4x4(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7],
        m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
}

// One production: build the block, then check routing and that V * P is E443C's A * R * P.
void production(const Camera& c) {
    float p[16], r[16];
    const float position[3] = {(float)c.x, (float)c.y, (float)c.z};
    perspective(c, p); rotation(c, r);
    volatile TooieCameraFloat block;
    const TooieCameraWord rsp = 0x00123440u;
    CHECK(cam_float_build(&block, rsp, p, r, position, c.scale) == 1u);

    for (int i = 0; i < 16; i++) CHECK(block.projection[i] == p[i]);
    CHECK(block.view[3] == 0.0f && block.view[7] == 0.0f && block.view[11] == 0.0f && block.view[15] == 1.0f);

    // RT64 routing: the LOAD is a projection (no combined decomposition), the MUL is a view.
    const hlslpp::float4x4 pm = matrix(block.projection), vm = matrix(block.view);
    CHECK(!RT64::isMatrixViewProj(pm));
    CHECK(RT64::isMatrixAffine(vm) && !RT64::isMatrixIdentity(vm));
    const float p33 = pm[3][3], p11 = pm[1][1];
    CHECK(p33 == 0.0f && std::fabs(p11) > 1e-6f);   // RSP::getCurrentProjectionType: perspective

    // E443C: P, premultiplied by R, premultiplied by A = T(-p) * S(1/s).
    double a[16] = {0}, rd[16], pd[16], ar[16], reference[16], vd[16], product[16];
    for (int i = 0; i < 3; i++) a[i * 4 + i] = 1.0 / c.scale;
    for (int j = 0; j < 3; j++) a[12 + j] = -position[j] / (double)c.scale;
    a[15] = 1.0;
    for (int i = 0; i < 16; i++) { rd[i] = r[i]; pd[i] = p[i]; vd[i] = block.view[i]; }
    mul(a, rd, ar); mul(ar, pd, reference); mul(vd, pd, product);
    double largest = 1.0;
    for (int i = 0; i < 16; i++) largest = std::fmax(largest, std::fabs(reference[i]));
    for (int i = 0; i < 16; i++) CHECK(std::fabs(product[i] - reference[i]) <= 1e-5 * largest);
    // RT64's own float product (viewProj = MUL matrix * current viewProj) agrees as well.
    const hlslpp::float4x4 rt64 = hlslpp::mul(vm, pm);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
        const float e = rt64[i][j];
        CHECK(std::fabs(e - reference[i * 4 + j]) <= 1e-5 * largest);
    }

    // Two-command list: gEXMatrixFloat LOAD P, gEXMatrixFloat MUL V, end.
    CHECK(block.commands[0] == 0x64000030u && block.commands[1] == 0x07u &&
        block.commands[2] == 0u && block.commands[3] == rsp);
    CHECK(block.commands[4] == 0x64000030u && block.commands[5] == 0x05u &&
        block.commands[6] == 0u && block.commands[7] == rsp + 64u);
    CHECK(block.commands[8] == 0xDF000000u && block.commands[9] == 0u);
}

void refusals() {
    const Camera c = {0.4, -0.2, 0.0, 120.0, 80.0, -340.0, 1.0f, 0.9f, 1.333f, 10.0f, 8000.0f};
    float p[16], r[16];
    const float position[3] = {120.0f, 80.0f, -340.0f};
    volatile TooieCameraFloat block;
    perspective(c, p); rotation(c, r);
    CHECK(!cam_float_build(&block, 0, p, r, position, 0.0f));
    CHECK(!cam_float_build(&block, 0, p, r, position, -1.0f));
    CHECK(!cam_float_build(&block, 0, p, r, position, std::numeric_limits<float>::infinity()));
    CHECK(!cam_float_build(&block, 0, p, r, position, std::numeric_limits<float>::quiet_NaN()));
    float bad[16];
    for (int i = 0; i < 16; i++) bad[i] = r[i];
    bad[3] = 1e-3f;                                   // R not affine
    CHECK(!cam_float_build(&block, 0, p, bad, position, 1.0f));
    for (int i = 0; i < 16; i++) bad[i] = r[i];
    bad[15] = 0.5f;
    CHECK(!cam_float_build(&block, 0, p, bad, position, 1.0f));
    for (int i = 0; i < 16; i++) bad[i] = p[i];
    bad[15] = 0.25f;                                  // P33 != 0
    CHECK(!cam_float_build(&block, 0, bad, r, position, 1.0f));
}

}  // namespace

int main() {
    const double pi = 3.14159265358979323846;
    const Camera cameras[] = {
        {0.4, -0.2, 0.0, 120.0, 80.0, -340.0, 1.0f, 0.9f, 1.333f, 10.0f, 8000.0f},      // rotated, offset
        {2.3, 0.35, 0.1, -1450.0, 260.0, 990.0, 0.37f, 1.1f, 1.333f, 25.0f, 20000.0f},  // nonunit scale
        {2.3, 0.35, 0.1, -1450.0, 260.0, 990.0, 0.74f, 1.1f, 1.333f, 25.0f, 20000.0f},  // doubled scale
        {0.0, -0.3, 0.0, 300.0, 120.0, -800.0, 1.0f, 0.9f, 1.778f, 10.0f, 8000.0f},     // forward on world Z
        {pi, 0.2, 0.0, -50.0, 40.0, 1200.0, 1.25f, 0.9f, 1.333f, 10.0f, 8000.0f},
        {pi / 2, 0.0, 0.0, 1500.0, -300.0, 1500.0, 1.0f, 0.7f, 1.333f, 5.0f, 30000.0f},
    };
    for (const Camera& c : cameras) production(c);
    refusals();
    if (failures) { std::fprintf(stderr, "baseline_camera_float: %d failure(s)\n", failures); return 1; }
    std::printf("baseline_camera_float: ok\n");
    return 0;
}
