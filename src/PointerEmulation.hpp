#pragma once

#include <hyprland/src/devices/ITouch.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>

#include <cstdint>
#include <optional>
#include <set>

// While the modifiers in plugin:hyprgrass:pointer_emulation_mods are held, a finger
// drives the pointer instead of touching: left button down where it lands, pointer
// motion while it moves, button up when it lifts. Every app then selects text,
// drags and clicks exactly as with a mouse.
//
// The decision is made once, at touch-down, and only when no other finger is on the
// screen. The finger stays a pointer until it lifts, so releasing the modifier
// mid-drag cannot leave the button pressed, pressing it mid-touch cannot hand an app
// half a touch sequence, and other fingers are swallowed while it is active.
class PointerEmulation {
  public:
    // Each returns whether the event is consumed: neither clients nor gesture
    // recognition may see it.
    bool onTouchDown(const ITouch::SDownEvent& ev);
    bool onTouchMove(const ITouch::SMotionEvent& ev);
    bool onTouchUp(const ITouch::SUpEvent& ev);

  private:
    std::set<int32_t>      m_fingersDown;
    std::optional<int32_t> m_pointerFinger;
    PHLMONITORREF          m_monitor;

    void                   movePointerTo(const Vector2D& normalizedPos);
};

inline UP<PointerEmulation> g_pPointerEmulation;
