#include "PointerEmulation.hpp"
#include "GestureManager.hpp"

#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/managers/SeatManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/pointer/PointerManager.hpp>
#include <hyprland/src/state/MonitorState.hpp>

#include <linux/input-event-codes.h>

bool PointerEmulation::onTouchDown(const ITouch::SDownEvent& ev) {
    static auto const MODS = g_config->pointerEmulationMods;

    const bool firstFinger = m_fingersDown.empty();
    m_fingersDown.insert(ev.touchID);

    if (m_pointerFinger)
        return true; // a pointer drag owns the screen until its finger lifts

    const auto modsName = MODS->value();
    if (!firstFinger || modsName.empty())
        return false;

    const auto wanted = g_pKeybindManager->stringToModMask(modsName);
    if (wanted == 0 || (g_pInputManager->getModsFromAllKBs() & wanted) != wanted)
        return false;

    auto monitor = State::monitorState()->query().name(!ev.device->m_boundOutput.empty() ? ev.device->m_boundOutput : "").run();
    monitor      = monitor ? monitor : Desktop::focusState()->monitor();
    if (!monitor)
        return false;

    m_pointerFinger = ev.touchID;
    m_monitor       = monitor;

    movePointerTo(ev.pos);
    g_pInputManager->onMouseButton({.timeMs = ev.timeMs, .button = BTN_LEFT, .state = WL_POINTER_BUTTON_STATE_PRESSED}, nullptr);
    g_pSeatManager->sendPointerFrame();
    return true;
}

bool PointerEmulation::onTouchMove(const ITouch::SMotionEvent& ev) {
    if (!m_pointerFinger)
        return false;

    if (ev.touchID == *m_pointerFinger)
        movePointerTo(ev.pos);

    return true;
}

bool PointerEmulation::onTouchUp(const ITouch::SUpEvent& ev) {
    m_fingersDown.erase(ev.touchID);

    if (!m_pointerFinger)
        return false;

    if (ev.touchID == *m_pointerFinger) {
        g_pInputManager->onMouseButton({.timeMs = ev.timeMs, .button = BTN_LEFT, .state = WL_POINTER_BUTTON_STATE_RELEASED}, nullptr);
        g_pSeatManager->sendPointerFrame();
        m_pointerFinger.reset();
        m_monitor.reset();
    }

    return true;
}

void PointerEmulation::movePointerTo(const Vector2D& normalizedPos) {
    const auto monitor = m_monitor.lock();
    if (!monitor)
        return;

    Pointer::mgr()->warpTo(monitor->m_position + normalizedPos * monitor->m_size);

    // The finger is now a pointer, so input is no longer "touch": this also lets the
    // cursor show, and lets the move below reach clients.
    g_pInputManager->m_lastInputTouch = false;
    g_pInputManager->simulateMouseMovement();
    g_pSeatManager->sendPointerFrame();
}
