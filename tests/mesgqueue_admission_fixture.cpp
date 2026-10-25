// MQ01: actual selected admission bodies over a queue with scripted refusals.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <vector>

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"

#include "mesgqueue_admission_struct.inl"

// Only the two members used by the extracted bodies. Refusal leaves no message,
// matching the pinned BlockingConcurrentQueue::enqueue contract.
static struct {
    std::deque<QueuedMessage> items;
    int attempts = 0;
    int refuse_attempt = -1;
    bool enqueue(const QueuedMessage& message) {
        if (attempts++ == refuse_attempt) return false;
        items.push_back(message);
        return true;
    }
    bool try_dequeue(QueuedMessage& message) {
        if (items.empty()) return false;
        message = items.front();
        items.pop_front();
        return true;
    }
} external_messages;

static std::vector<bool> send_results;
static std::vector<QueuedMessage> sent;

bool do_send(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, bool jam, bool block) {
    if (block || sent.size() >= send_results.size()) std::_Exit(3);
    sent.push_back({mq_, msg, jam, false});
    return send_results[sent.size() - 1];
}

#include "mesgqueue_admission_bodies.inl"

static constexpr PTR(OSMesgQueue) queue = static_cast<PTR(OSMesgQueue)>(0x80100000u);

static bool same(const QueuedMessage& message, OSMesg msg, bool jam, bool requeue) {
    return message.mq == queue && message.mesg == msg && message.jam == jam && message.requeue_if_blocked == requeue;
}

int main(int argc, char** argv) {
    uint8_t* rdram = nullptr;
    const char* name = argc == 2 ? argv[1] : "";
    auto& q = external_messages;
    if (!std::strcmp(name, "admitted")) {
        ultramodern::enqueue_external_message(queue, 7, true, true);
        ultramodern::enqueue_external_message(queue, 8, false, false);
        return q.attempts == 2 && q.items.size() == 2 &&
            same(q.items[0], 7, true, true) && same(q.items[1], 8, false, false) ? 0 : 3;
    }
    if (!std::strcmp(name, "droppable-refused")) {
        q.refuse_attempt = 0;
        ultramodern::enqueue_external_message(queue, 8, false, false);
        return q.attempts == 1 && q.items.empty() ? 0 : 3;
    }
    if (!std::strcmp(name, "required-refused")) {
        q.refuse_attempt = 0;
        ultramodern::enqueue_external_message(queue, 7, false, true);
        std::fputs("required refusal returned\n", stderr);
        return 3;
    }
    if (!std::strcmp(name, "retry")) {
        // 1 sends; blocked reliable 2 and 4 retry once in order; droppable 3 drops.
        ultramodern::enqueue_external_message(queue, 1, false, true);
        ultramodern::enqueue_external_message(queue, 2, true, true);
        ultramodern::enqueue_external_message(queue, 3, false, false);
        ultramodern::enqueue_external_message(queue, 4, false, true);
        send_results = {true, false, false, false};
        dequeue_external_messages(PASS_RDRAM1);
        return q.attempts == 6 && sent.size() == 4 && same(sent[0], 1, false, false) &&
            same(sent[1], 2, true, false) && same(sent[2], 3, false, false) && same(sent[3], 4, false, false) &&
            q.items.size() == 2 && same(q.items[0], 2, true, true) && same(q.items[1], 4, false, true) ? 0 : 3;
    }
    if (!std::strcmp(name, "retry-refused")) {
        ultramodern::enqueue_external_message(queue, 2, false, true);
        q.refuse_attempt = 1;
        send_results = {false};
        dequeue_external_messages(PASS_RDRAM1);
        std::fputs("required retry refusal returned\n", stderr);
        return 3;
    }
    return 2;
}
