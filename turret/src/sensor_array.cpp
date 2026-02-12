/**
 * @file sensor_array.cpp
 * @brief Implementation of 4-channel TSOP38238 sensor reading and filtering.
 */

#include "sensor_array.h"
#include "config.h"
#include <Arduino.h>

// Pin look-up table indexed by [0]=top, [1]=bottom, [2]=left, [3]=right.
static const uint8_t SENSOR_PINS[4] = {
    PIN_SENSOR_TOP,
    PIN_SENSOR_BOTTOM,
    PIN_SENSOR_LEFT,
    PIN_SENSOR_RIGHT
};

// ===================================================================
// SensorReading helpers
// ===================================================================

bool SensorReading::anyActive() const {
    return top    == SensorState::ACTIVE ||
           bottom == SensorState::ACTIVE ||
           left   == SensorState::ACTIVE ||
           right  == SensorState::ACTIVE;
}

bool SensorReading::noneActive() const {
    return !anyActive();
}

// ===================================================================
// SensorArray
// ===================================================================

void SensorArray::init() {
    for (uint8_t i = 0; i < 4; i++) {
        pinMode(SENSOR_PINS[i], INPUT_PULLUP);
        filters_[i] = FilterState{};
    }
}

void SensorArray::update() {
    for (uint8_t i = 0; i < 4; i++) {
        // TSOP38238 is active-low: LOW = signal detected.
        bool active = (digitalRead(SENSOR_PINS[i]) == LOW);
        pushSample(i, active);

        // Saturation tracking.
        if (active) {
            filters_[i].lowRunMs += LOOP_PERIOD_MS;
            if (filters_[i].lowRunMs >= SENSOR_SATURATED_MS) {
                filters_[i].saturated = true;
            }
        } else {
            filters_[i].lowRunMs  = 0;
            filters_[i].saturated = false;
        }
    }
}

SensorReading SensorArray::getFiltered() const {
    SensorReading r;
    r.top    = evaluateSensor(0);
    r.bottom = evaluateSensor(1);
    r.left   = evaluateSensor(2);
    r.right  = evaluateSensor(3);
    return r;
}

Direction SensorArray::getDirection() const {
    SensorReading r = getFiltered();

    bool l = (r.left   == SensorState::ACTIVE);
    bool ri = (r.right  == SensorState::ACTIVE);
    bool u = (r.top    == SensorState::ACTIVE);
    bool d = (r.bottom == SensorState::ACTIVE);

    if (r.noneActive()) return Direction::NONE;

    // Determine horizontal component.
    int8_t h = 0;  // -1 = left, 0 = center, +1 = right
    if (l && !ri)  h = -1;
    if (ri && !l)  h = +1;

    // Determine vertical component.
    int8_t v = 0;  // -1 = up, 0 = center, +1 = down
    if (u && !d)   v = -1;
    if (d && !u)   v = +1;

    // Map to Direction enum.
    if (h == -1 && v == -1) return Direction::UP_LEFT;
    if (h == -1 && v ==  0) return Direction::LEFT;
    if (h == -1 && v == +1) return Direction::DOWN_LEFT;
    if (h ==  0 && v == -1) return Direction::UP;
    if (h ==  0 && v == +1) return Direction::DOWN;
    if (h == +1 && v == -1) return Direction::UP_RIGHT;
    if (h == +1 && v ==  0) return Direction::RIGHT;
    if (h == +1 && v == +1) return Direction::DOWN_RIGHT;

    return Direction::CENTER;
}

// ===================================================================
// Private helpers
// ===================================================================

void SensorArray::pushSample(uint8_t idx, bool active) {
    FilterState &f = filters_[idx];

    // Write the sample bit into the ring buffer.
    if (active) {
        f.buffer |=  (1 << f.index);
    } else {
        f.buffer &= ~(1 << f.index);
    }

    f.index = (f.index + 1) % SENSOR_FILTER_WINDOW;
}

uint8_t SensorArray::popcount(uint8_t bits) {
    uint8_t count = 0;
    // Only count the lower SENSOR_FILTER_WINDOW bits.
    uint8_t mask = (1 << SENSOR_FILTER_WINDOW) - 1;
    bits &= mask;
    while (bits) {
        count += (bits & 1);
        bits >>= 1;
    }
    return count;
}

SensorState SensorArray::evaluateSensor(uint8_t idx) const {
    const FilterState &f = filters_[idx];

    if (f.saturated) {
        return SensorState::SATURATED;
    }

    uint8_t activeCount = popcount(f.buffer);
    if (activeCount >= SENSOR_FILTER_THRESHOLD) {
        return SensorState::ACTIVE;
    }
    return SensorState::INACTIVE;
}
