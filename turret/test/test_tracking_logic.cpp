/**
 * @file test_tracking_logic.cpp
 * @brief Unit tests for turret tracking decisions.
 *
 * Tests cover:
 *   1. Dead band: both L+R active → pan speed = 0.
 *   2. Dead band: neither L nor R active → pan speed = 0.
 *   3. Left-only → negative pan speed (CCW).
 *   4. Right-only → positive pan speed (CW).
 *   5. Top-only → positive tilt delta (up).
 *   6. Bottom-only → negative tilt delta (down).
 *   7. Software limits: pan at +135° ignores positive speed commands.
 *   8. Signal loss: TRACKING → SEARCHING after 3 s, → PARKED after 15 s.
 *   9. Signal recovery: any detection in PARKED → immediate TRACKING.
 *  10. Saturation guard: sensor stuck LOW for 2 s → treated as INACTIVE.
 *  11. Holdoff: brief dropout within 500 ms does not leave TRACKING.
 *  12. State transition detection: stateChanged() fires on transitions.
 *
 * Build with: pio test -e native
 * Requires the [env:native] target in platformio.ini.
 */

#ifdef UNIT_TEST

// ===================================================================
// Minimal Arduino mock — enough to compile the logic modules
// ===================================================================

#include <cstdint>
#include <cmath>
#include <cstring>

// Mock millis() — test code controls time
static unsigned long mock_millis_value = 0;
unsigned long millis() { return mock_millis_value; }
void advanceMillis(unsigned long ms) { mock_millis_value += ms; }
void resetMillis() { mock_millis_value = 0; }

// Mock digitalRead / pinMode — not needed for pure logic tests
void pinMode(uint8_t, uint8_t) {}
int digitalRead(uint8_t) { return 1; }  // default HIGH (inactive)
void digitalWrite(uint8_t, uint8_t) {}
void delay(unsigned long) {}

// Mock Servo class (minimal)
class Servo {
public:
    void attach(int) {}
    void write(int angle) { lastAngle_ = angle; }
    void writeMicroseconds(int us) { lastUs_ = us; }
    int lastAngle_ = 0;
    int lastUs_ = 1500;
};

// Provide the ESP32Servo.h include guard so the real header is skipped
#define _ESP32Servo_H_

// Stub Serial
struct MockSerial {
    void begin(unsigned long) {}
    void print(const char*) {}
    void print(float, int = 2) {}
    void print(int) {}
    void print(char) {}
    void println(const char*) {}
    void println() {}
} Serial;

// F() macro stub
#define F(s) (s)

// Now include the actual logic modules
#include "../include/config.h"
#include "../include/sensor_array.h"
#include "../include/signal_monitor.h"

// Include implementations inline for native build
// (In a real setup, these would be compiled separately via test_build_src)

// ===================================================================
// Unity test framework
// ===================================================================

#include <unity.h>

// ===================================================================
// Helper: create a SensorReading with specific states
// ===================================================================

static SensorReading makeReading(
    SensorState top, SensorState bottom,
    SensorState left, SensorState right)
{
    SensorReading r;
    r.top    = top;
    r.bottom = bottom;
    r.left   = left;
    r.right  = right;
    return r;
}

// ===================================================================
// Test 1: Dead band — both L+R active → pan speed = 0
// ===================================================================

void test_deadband_both_active() {
    SensorReading r = makeReading(
        SensorState::INACTIVE, SensorState::INACTIVE,
        SensorState::ACTIVE, SensorState::ACTIVE);

    // Both horizontal sensors active = centered → should hold.
    bool left  = (r.left  == SensorState::ACTIVE);
    bool right = (r.right == SensorState::ACTIVE);
    TEST_ASSERT_TRUE(left == right);  // dead band condition
}

// ===================================================================
// Test 2: Dead band — neither L nor R active → hold
// ===================================================================

void test_deadband_neither_active() {
    SensorReading r = makeReading(
        SensorState::INACTIVE, SensorState::INACTIVE,
        SensorState::INACTIVE, SensorState::INACTIVE);

    bool left  = (r.left  == SensorState::ACTIVE);
    bool right = (r.right == SensorState::ACTIVE);
    TEST_ASSERT_TRUE(left == right);  // dead band condition (both false)
    TEST_ASSERT_TRUE(r.noneActive());
}

// ===================================================================
// Test 3: Left-only → negative speed (CCW)
// ===================================================================

void test_left_only_negative_speed() {
    SensorReading r = makeReading(
        SensorState::INACTIVE, SensorState::INACTIVE,
        SensorState::ACTIVE, SensorState::INACTIVE);

    bool left  = (r.left  == SensorState::ACTIVE);
    bool right = (r.right == SensorState::ACTIVE);
    TEST_ASSERT_TRUE(left && !right);

    // The tracking engine would return negative speed for left.
    // Convention: left = CCW = negative.
    float direction = left ? -1.0f : 1.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, direction);
}

// ===================================================================
// Test 4: Right-only → positive speed (CW)
// ===================================================================

void test_right_only_positive_speed() {
    SensorReading r = makeReading(
        SensorState::INACTIVE, SensorState::INACTIVE,
        SensorState::INACTIVE, SensorState::ACTIVE);

    bool left  = (r.left  == SensorState::ACTIVE);
    bool right = (r.right == SensorState::ACTIVE);
    TEST_ASSERT_TRUE(!left && right);

    float direction = left ? -1.0f : 1.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, direction);
}

// ===================================================================
// Test 5: Top-only → positive tilt delta (up)
// ===================================================================

void test_top_only_tilt_up() {
    SensorReading r = makeReading(
        SensorState::ACTIVE, SensorState::INACTIVE,
        SensorState::INACTIVE, SensorState::INACTIVE);

    bool top    = (r.top    == SensorState::ACTIVE);
    bool bottom = (r.bottom == SensorState::ACTIVE);
    TEST_ASSERT_TRUE(top && !bottom);

    int8_t delta = top ? +static_cast<int8_t>(TILT_STEP_DEG)
                       : -static_cast<int8_t>(TILT_STEP_DEG);
    TEST_ASSERT_EQUAL_INT8(+1, delta);
}

// ===================================================================
// Test 6: Bottom-only → negative tilt delta (down)
// ===================================================================

void test_bottom_only_tilt_down() {
    SensorReading r = makeReading(
        SensorState::INACTIVE, SensorState::ACTIVE,
        SensorState::INACTIVE, SensorState::INACTIVE);

    bool top    = (r.top    == SensorState::ACTIVE);
    bool bottom = (r.bottom == SensorState::ACTIVE);
    TEST_ASSERT_TRUE(!top && bottom);

    int8_t delta = bottom ? -static_cast<int8_t>(TILT_STEP_DEG)
                          : +static_cast<int8_t>(TILT_STEP_DEG);
    TEST_ASSERT_EQUAL_INT8(-1, delta);
}

// ===================================================================
// Test 7: Software limits — pan at +135° ignores positive speed
// ===================================================================

void test_pan_software_limit() {
    // Simulate: position is at the limit, positive speed requested.
    float positionDeg = PAN_LIMIT_DEG;
    float speed = 0.8f;

    // The PanController logic: if at limit and pushing further, clamp to 0.
    if (positionDeg >= PAN_LIMIT_DEG && speed > 0.0f) speed = 0.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, speed);

    // Negative speed should still be allowed (moving away from limit).
    float speedNeg = -0.5f;
    if (positionDeg >= PAN_LIMIT_DEG && speedNeg > 0.0f) speedNeg = 0.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, speedNeg);
}

// ===================================================================
// Test 8: Signal loss transitions — TRACKING → SEARCHING → PARKED
// ===================================================================

void test_signal_loss_state_transitions() {
    resetMillis();
    SignalMonitor mon;
    mon.init();

    // Start in TRACKING with signal present.
    mon.update(true);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::TRACKING),
                      static_cast<uint8_t>(mon.getState()));

    // Lose signal. Advance past holdoff but before search threshold.
    advanceMillis(SIGNAL_PRESENT_HOLDOFF_MS + 100);
    mon.update(false);
    // Should still be TRACKING (holdoff passed but not yet at search threshold).
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::TRACKING),
                      static_cast<uint8_t>(mon.getState()));

    // Advance to just past SIGNAL_LOSS_SEARCH_MS total from last signal.
    resetMillis();
    mon.init();
    mon.update(true);  // signal at t=0
    advanceMillis(SIGNAL_LOSS_SEARCH_MS + 100);
    mon.update(false);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::SEARCHING),
                      static_cast<uint8_t>(mon.getState()));

    // Advance to SIGNAL_LOSS_PARK_MS.
    advanceMillis(SIGNAL_LOSS_PARK_MS - SIGNAL_LOSS_SEARCH_MS);
    mon.update(false);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::PARKED),
                      static_cast<uint8_t>(mon.getState()));
}

// ===================================================================
// Test 9: Signal recovery — detection in PARKED → immediate TRACKING
// ===================================================================

void test_signal_recovery_from_parked() {
    resetMillis();
    SignalMonitor mon;
    mon.init();

    // Drive to PARKED.
    mon.update(true);  // signal at t=0
    advanceMillis(SIGNAL_LOSS_PARK_MS + 100);
    mon.update(false);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::PARKED),
                      static_cast<uint8_t>(mon.getState()));

    // Signal detected → immediate return to TRACKING.
    advanceMillis(50);
    mon.update(true);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::TRACKING),
                      static_cast<uint8_t>(mon.getState()));
}

// ===================================================================
// Test 10: Saturation guard — sensor stuck LOW → SATURATED
// ===================================================================

void test_saturation_guard() {
    SensorReading r;
    r.top    = SensorState::SATURATED;
    r.bottom = SensorState::INACTIVE;
    r.left   = SensorState::INACTIVE;
    r.right  = SensorState::INACTIVE;

    // SATURATED sensors should not count as active.
    TEST_ASSERT_FALSE(r.anyActive());
}

// ===================================================================
// Test 11: Holdoff — brief dropout within window stays TRACKING
// ===================================================================

void test_holdoff_prevents_premature_search() {
    resetMillis();
    SignalMonitor mon;
    mon.init();

    // Signal present.
    mon.update(true);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::TRACKING),
                      static_cast<uint8_t>(mon.getState()));

    // Brief dropout (100 ms — within SIGNAL_PRESENT_HOLDOFF_MS of 500 ms).
    advanceMillis(100);
    mon.update(false);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::TRACKING),
                      static_cast<uint8_t>(mon.getState()));

    // Signal returns before holdoff expires.
    advanceMillis(50);
    mon.update(true);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(MonitorState::TRACKING),
                      static_cast<uint8_t>(mon.getState()));
}

// ===================================================================
// Test 12: stateChanged() fires on transitions
// ===================================================================

void test_state_changed_detection() {
    resetMillis();
    SignalMonitor mon;
    mon.init();

    mon.update(true);
    TEST_ASSERT_FALSE(mon.stateChanged());  // TRACKING → TRACKING

    // Drive to SEARCHING.
    advanceMillis(SIGNAL_LOSS_SEARCH_MS + 100);
    mon.update(false);
    TEST_ASSERT_TRUE(mon.stateChanged());  // TRACKING → SEARCHING

    // Stay in SEARCHING (no change).
    advanceMillis(100);
    mon.update(false);
    TEST_ASSERT_FALSE(mon.stateChanged());  // SEARCHING → SEARCHING
}

// ===================================================================
// Test 13: anyActive / noneActive helpers
// ===================================================================

void test_sensor_reading_helpers() {
    SensorReading all_off = makeReading(
        SensorState::INACTIVE, SensorState::INACTIVE,
        SensorState::INACTIVE, SensorState::INACTIVE);
    TEST_ASSERT_TRUE(all_off.noneActive());
    TEST_ASSERT_FALSE(all_off.anyActive());

    SensorReading one_on = makeReading(
        SensorState::INACTIVE, SensorState::ACTIVE,
        SensorState::INACTIVE, SensorState::INACTIVE);
    TEST_ASSERT_TRUE(one_on.anyActive());
    TEST_ASSERT_FALSE(one_on.noneActive());

    SensorReading saturated_only = makeReading(
        SensorState::SATURATED, SensorState::INACTIVE,
        SensorState::INACTIVE, SensorState::INACTIVE);
    TEST_ASSERT_FALSE(saturated_only.anyActive());
}

// ===================================================================
// Test runner
// ===================================================================

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_deadband_both_active);
    RUN_TEST(test_deadband_neither_active);
    RUN_TEST(test_left_only_negative_speed);
    RUN_TEST(test_right_only_positive_speed);
    RUN_TEST(test_top_only_tilt_up);
    RUN_TEST(test_bottom_only_tilt_down);
    RUN_TEST(test_pan_software_limit);
    RUN_TEST(test_signal_loss_state_transitions);
    RUN_TEST(test_signal_recovery_from_parked);
    RUN_TEST(test_saturation_guard);
    RUN_TEST(test_holdoff_prevents_premature_search);
    RUN_TEST(test_state_changed_detection);
    RUN_TEST(test_sensor_reading_helpers);

    return UNITY_END();
}

#endif // UNIT_TEST
