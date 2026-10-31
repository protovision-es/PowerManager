#ifndef ACTIONS_H
#define ACTIONS_H

// Executes queued actions (actionq.h) from loop().

#include <Arduino.h>
#include "actionq.h"
#include "relay.h"
#include "wol.h"
#include "ping.h"

inline void actionsProcess() {
    Action a;
    while (actionPop(a)) {
        switch (a.type) {
            case ACTION_RESTART:
                if (!relayRestart(a.id, a.reason) && a.id >= 0 && a.id < RELAY_COUNT) {
                    logAddf("%s: restart ignored, already restarting", appConfig.relays[a.id].name);
                }
                break;
            case ACTION_WOL:
                wolSend(a.id);
                break;
            case ACTION_PING_NOW:
                pingRequestNow();
                break;
        }
    }
}

#endif
