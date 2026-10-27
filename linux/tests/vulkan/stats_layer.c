// Vulkan layer: like fps_layer.c, but also counts recorded GPU work. Once per second appends to
// $STATS_LAYER_OUT: "<unix_s> presents submits renderpasses rp_mpix draws kverts draw_mpix dispatches
// kgroups copies copy_mpix resolves resolve_mpix copybuf_mb barriers" (totals for that second).
// draw_mpix sums the scissor area of every draw (overdraw proxy). Single device assumed.
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static PFN_vkGetInstanceProcAddr next_gipa;
static PFN_vkGetDeviceProcAddr next_gdpa;
static PFN_vkQueuePresentKHR next_present;
static PFN_vkQueueSubmit next_submit;
static PFN_vkCmdBeginRenderPass next_begin_rp;
static PFN_vkCmdDraw next_draw;
static PFN_vkCmdDrawIndexed next_draw_indexed;
static PFN_vkCmdDispatch next_dispatch;
static PFN_vkCmdCopyImage next_copy_image;
static PFN_vkCmdResolveImage next_resolve;
static PFN_vkCmdCopyBuffer next_copy_buffer;
static PFN_vkCmdSetScissor next_set_scissor;
static PFN_vkCmdPipelineBarrier next_barrier;
static PFN_vkCmdBindPipeline next_bind_pipeline;
static unsigned long long cur_pipeline;
static FILE* out;
static double window_start;
static double c_present, c_submit, c_rp, c_rp_pix, c_draw, c_vert, c_draw_pix, c_disp, c_groups,
    c_copy, c_copy_pix, c_resolve, c_resolve_pix, c_copybuf, c_barrier;
static double scissor_pix;
// One-shot dump: STATS_LAYER_DUMP_AT=<seconds after first present> logs each render pass of the next 3
// presents to $STATS_LAYER_OUT.dump: "present rp width height draws verts".
static double first_present, dump_at = -1;
static int dumping, dump_presents, dump_rp;
static FILE* dump;
static unsigned rp_w, rp_h, rp_draws; static double rp_verts;
static unsigned long long rp_fb; static unsigned submits_total;
static void dump_rp_line(void) {
    if (dumping && rp_w) fprintf(dump, "%d %d %u %u %u %.0f S%u %llx\n", dump_presents, dump_rp++, rp_w, rp_h, rp_draws, rp_verts, submits_total, rp_fb);
    rp_w = rp_h = rp_draws = 0; rp_verts = 0;
}  // last scissor set (any command buffer; recording is effectively serial here)

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static VKAPI_ATTR VkResult VKAPI_CALL layer_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* info) {
    VkResult result = next_present(queue, info);
    double t = now();
    ++c_present;
    if (first_present == 0) {
        first_present = t;
        const char* at = getenv("STATS_LAYER_DUMP_AT");
        if (at) dump_at = atof(at);
    }
    if (dumping) {
        dump_rp_line();
        dump_rp = 0;
        fprintf(dump, "PRESENT %d\n", dump_presents);
        if (++dump_presents == 6) { dumping = 0; fclose(dump); }
    } else if (dump_at >= 0 && t - first_present >= dump_at) {
        char path[512];
        const char* o = getenv("STATS_LAYER_OUT");
        snprintf(path, sizeof path, "%s.dump", o ? o : "/tmp/stats_layer.txt");
        dump = fopen(path, "w");
        dumping = dump != NULL; dump_at = -1; rp_w = 0;
    }
    if (window_start == 0) window_start = t;
    if (t - window_start >= 1.0) {
        if (!out) {
            const char* path = getenv("STATS_LAYER_OUT");
            out = fopen(path ? path : "/tmp/stats_layer.txt", "a");
        }
        if (out) {
            struct timespec wall;
            clock_gettime(CLOCK_REALTIME, &wall);
            double s = t - window_start;
            fprintf(out, "%ld %.1f %.0f %.0f %.1f %.0f %.1f %.1f %.0f %.1f %.0f %.1f %.0f %.1f %.2f %.0f\n",
                (long)wall.tv_sec, c_present / s, c_submit / s, c_rp / s, c_rp_pix / s / 1e6, c_draw / s,
                c_vert / s / 1e3, c_draw_pix / s / 1e6, c_disp / s, c_groups / s / 1e3, c_copy / s,
                c_copy_pix / s / 1e6, c_resolve / s, c_resolve_pix / s / 1e6, c_copybuf / s / 1e6, c_barrier / s);
            fflush(out);
        }
        c_present = c_submit = c_rp = c_rp_pix = c_draw = c_vert = c_draw_pix = c_disp = c_groups = 0;
        c_copy = c_copy_pix = c_resolve = c_resolve_pix = c_copybuf = c_barrier = 0;
        window_start = t;
    }
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL layer_QueueSubmit(VkQueue q, uint32_t n, const VkSubmitInfo* s, VkFence f) {
    c_submit += n; submits_total += n;
    if (dumping) fprintf(dump, "SUBMIT %u\n", submits_total);
    return next_submit(q, n, s, f);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdBeginRenderPass(VkCommandBuffer cb, const VkRenderPassBeginInfo* b, VkSubpassContents c) {
    ++c_rp;
    dump_rp_line();
    rp_w = b->renderArea.extent.width; rp_h = b->renderArea.extent.height; rp_fb = (unsigned long long)(uintptr_t)b->framebuffer;
    c_rp_pix += (double)b->renderArea.extent.width * b->renderArea.extent.height;
    scissor_pix = (double)b->renderArea.extent.width * b->renderArea.extent.height;
    next_begin_rp(cb, b, c);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdSetScissor(VkCommandBuffer cb, uint32_t first, uint32_t n, const VkRect2D* r) {
    if (n) scissor_pix = (double)r[0].extent.width * r[0].extent.height;
    next_set_scissor(cb, first, n, r);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdDraw(VkCommandBuffer cb, uint32_t v, uint32_t i, uint32_t fv, uint32_t fi) {
    ++c_draw; c_vert += (double)v * i; c_draw_pix += scissor_pix; ++rp_draws; rp_verts += (double)v * i;
    if (dumping) fprintf(dump, "D %d %d %u %llx %.0f\n", dump_presents, dump_rp, v * i, cur_pipeline, scissor_pix);
    next_draw(cb, v, i, fv, fi);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdDrawIndexed(VkCommandBuffer cb, uint32_t n, uint32_t i, uint32_t fi, int32_t vo, uint32_t fin) {
    ++c_draw; c_vert += (double)n * i; c_draw_pix += scissor_pix; ++rp_draws; rp_verts += (double)n * i;
    if (dumping) fprintf(dump, "I %d %d %u %llx %.0f %u\n", dump_presents, dump_rp, n * i, cur_pipeline, scissor_pix, fi);
    next_draw_indexed(cb, n, i, fi, vo, fin);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdDispatch(VkCommandBuffer cb, uint32_t x, uint32_t y, uint32_t z) {
    ++c_disp; c_groups += (double)x * y * z;
    next_dispatch(cb, x, y, z);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdCopyImage(VkCommandBuffer cb, VkImage si, VkImageLayout sl, VkImage di, VkImageLayout dl, uint32_t n, const VkImageCopy* r) {
    for (uint32_t k = 0; k < n; ++k) { ++c_copy; c_copy_pix += (double)r[k].extent.width * r[k].extent.height; }
    next_copy_image(cb, si, sl, di, dl, n, r);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdResolveImage(VkCommandBuffer cb, VkImage si, VkImageLayout sl, VkImage di, VkImageLayout dl, uint32_t n, const VkImageResolve* r) {
    for (uint32_t k = 0; k < n; ++k) { ++c_resolve; c_resolve_pix += (double)r[k].extent.width * r[k].extent.height; }
    next_resolve(cb, si, sl, di, dl, n, r);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdCopyBuffer(VkCommandBuffer cb, VkBuffer s, VkBuffer d, uint32_t n, const VkBufferCopy* r) {
    for (uint32_t k = 0; k < n; ++k) c_copybuf += (double)r[k].size;
    next_copy_buffer(cb, s, d, n, r);
}
static VKAPI_ATTR void VKAPI_CALL layer_CmdPipelineBarrier(VkCommandBuffer cb, VkPipelineStageFlags a, VkPipelineStageFlags b, VkDependencyFlags f,
    uint32_t mn, const VkMemoryBarrier* m, uint32_t bn, const VkBufferMemoryBarrier* bb, uint32_t in, const VkImageMemoryBarrier* ib) {
    ++c_barrier;
    next_barrier(cb, a, b, f, mn, m, bn, bb, in, ib);
}

static VKAPI_ATTR void VKAPI_CALL layer_CmdBindPipeline(VkCommandBuffer cb, VkPipelineBindPoint bp, VkPipeline p) {
    if (bp == VK_PIPELINE_BIND_POINT_GRAPHICS) cur_pipeline = (unsigned long long)(uintptr_t)p;
    next_bind_pipeline(cb, bp, p);
}

static VKAPI_ATTR VkResult VKAPI_CALL layer_CreateInstance(const VkInstanceCreateInfo* info,
    const VkAllocationCallbacks* alloc, VkInstance* instance) {
    VkLayerInstanceCreateInfo* chain = (VkLayerInstanceCreateInfo*)info->pNext;
    while (chain && !(chain->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain->function == VK_LAYER_LINK_INFO))
        chain = (VkLayerInstanceCreateInfo*)chain->pNext;
    if (!chain) return VK_ERROR_INITIALIZATION_FAILED;
    next_gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;
    PFN_vkCreateInstance create = (PFN_vkCreateInstance)next_gipa(VK_NULL_HANDLE, "vkCreateInstance");
    return create(info, alloc, instance);
}

#define LOAD(var, name) var = (void*)next_gdpa(*device, name)
static VKAPI_ATTR VkResult VKAPI_CALL layer_CreateDevice(VkPhysicalDevice physical, const VkDeviceCreateInfo* info,
    const VkAllocationCallbacks* alloc, VkDevice* device) {
    VkLayerDeviceCreateInfo* chain = (VkLayerDeviceCreateInfo*)info->pNext;
    while (chain && !(chain->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain->function == VK_LAYER_LINK_INFO))
        chain = (VkLayerDeviceCreateInfo*)chain->pNext;
    if (!chain) return VK_ERROR_INITIALIZATION_FAILED;
    PFN_vkGetInstanceProcAddr gipa = chain->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    next_gdpa = chain->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    chain->u.pLayerInfo = chain->u.pLayerInfo->pNext;
    PFN_vkCreateDevice create = (PFN_vkCreateDevice)gipa(VK_NULL_HANDLE, "vkCreateDevice");
    VkResult result = create(physical, info, alloc, device);
    if (result == VK_SUCCESS) {
        LOAD(next_present, "vkQueuePresentKHR"); LOAD(next_submit, "vkQueueSubmit");
        LOAD(next_begin_rp, "vkCmdBeginRenderPass"); LOAD(next_draw, "vkCmdDraw");
        LOAD(next_draw_indexed, "vkCmdDrawIndexed"); LOAD(next_dispatch, "vkCmdDispatch");
        LOAD(next_copy_image, "vkCmdCopyImage"); LOAD(next_resolve, "vkCmdResolveImage");
        LOAD(next_copy_buffer, "vkCmdCopyBuffer"); LOAD(next_set_scissor, "vkCmdSetScissor");
        LOAD(next_barrier, "vkCmdPipelineBarrier"); LOAD(next_bind_pipeline, "vkCmdBindPipeline");
    }
    return result;
}

#define HOOK(name, fn) if (!strcmp(n, name)) return (PFN_vkVoidFunction)fn
static PFN_vkVoidFunction hooked(const char* n) {
    HOOK("vkQueuePresentKHR", layer_QueuePresentKHR); HOOK("vkQueueSubmit", layer_QueueSubmit);
    HOOK("vkCmdBeginRenderPass", layer_CmdBeginRenderPass); HOOK("vkCmdDraw", layer_CmdDraw);
    HOOK("vkCmdDrawIndexed", layer_CmdDrawIndexed); HOOK("vkCmdDispatch", layer_CmdDispatch);
    HOOK("vkCmdCopyImage", layer_CmdCopyImage); HOOK("vkCmdResolveImage", layer_CmdResolveImage);
    HOOK("vkCmdCopyBuffer", layer_CmdCopyBuffer); HOOK("vkCmdSetScissor", layer_CmdSetScissor);
    HOOK("vkCmdPipelineBarrier", layer_CmdPipelineBarrier); HOOK("vkCmdBindPipeline", layer_CmdBindPipeline);
    return NULL;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL stats_layer_GetDeviceProcAddr(VkDevice device, const char* n) {
    PFN_vkVoidFunction f = hooked(n);
    if (f) return f;
    if (!strcmp(n, "vkGetDeviceProcAddr")) return (PFN_vkVoidFunction)stats_layer_GetDeviceProcAddr;
    return next_gdpa(device, n);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL stats_layer_GetInstanceProcAddr(VkInstance instance, const char* n) {
    if (!strcmp(n, "vkCreateInstance")) return (PFN_vkVoidFunction)layer_CreateInstance;
    if (!strcmp(n, "vkCreateDevice")) return (PFN_vkVoidFunction)layer_CreateDevice;
    if (!strcmp(n, "vkGetInstanceProcAddr")) return (PFN_vkVoidFunction)stats_layer_GetInstanceProcAddr;
    if (!strcmp(n, "vkGetDeviceProcAddr")) return (PFN_vkVoidFunction)stats_layer_GetDeviceProcAddr;
    PFN_vkVoidFunction f = hooked(n);
    if (f) return f;
    return next_gipa(instance, n);
}
