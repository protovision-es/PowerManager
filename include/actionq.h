#ifndef ACTIONQ_H
#define ACTIONQ_H

// ============================================================
// Action queue (no dependencies, so any module can push to it)
// The web server and the internet monitor run in their own FreeRTOS tasks,
// so they must not touch GPIOs or the WoL socket directly. They push an
// action here and loop() executes it (see actions.h).
// ============================================================

#include <Arduino.h>

enum ActionType : uint8_t {
    ACTION_RESTART = 1,
    ACTION_WOL,
    ACTION_PING_NOW,
};

struct Action {
    ActionType type;
    int8_t id;
    const char *reason;   // must point to a string literal
};

#define ACTION_QUEUE_LEN 8

static Action actionQueue[ACTION_QUEUE_LEN];
static uint8_t actionHead = 0, actionTail = 0;
static portMUX_TYPE actionMux = portMUX_INITIALIZER_UNLOCKED;

// Safe to call from any task. Returns false if the queue is full.
inline bool actionPush(ActionType type, int id, const char *reason) {
    bool ok = false;
    portENTER_CRITICAL(&actionMux);
    uint8_t next = (actionHead + 1) % ACTION_QUEUE_LEN;
    if (next != actionTail) {
        actionQueue[actionHead] = {type, (int8_t)id, reason};
        actionHead = next;
        ok = true;
    }
    portEXIT_CRITICAL(&actionMux);
    return ok;
}

static bool actionPop(Action &out) {
    bool ok = false;
    portENTER_CRITICAL(&actionMux);
    if (actionTail != actionHead) {
        out = actionQueue[actionTail];
        actionTail = (actionTail + 1) % ACTION_QUEUE_LEN;
        ok = true;
    }
    portEXIT_CRITICAL(&actionMux);
    return ok;
}

#endif
