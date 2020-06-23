#include "Servo.h"
#include "mbed.h"

static float clamp(float value, float min, float max) {
    if(value < min) {
        return min;
    } else if(value > max) {
        return max;
    } else {
        return value;
    }
}

Servo::Servo(PinName pin) : _pwm(pin) {
    calibrate();
    write(0.5);
}

void Servo::write(float percent) {
    float offset = _range * 2.0 * (percent - 0.5);
    _pwm.pulsewidth(0.0015 + clamp(offset, -_range, _range));
    _p = clamp(percent, 0.0, 1.0);
}

void Servo::position(float degrees) {
    float offset = _range * (degrees / _degrees);
    _pwm.pulsewidth(0.0015 + clamp(offset, -_range, _range));
}

void Servo::calibrate(float range, float degrees) {
    _range = range;
    _degrees = degrees;
}

float Servo::read() {
    return _p;
}

Servo& Servo::operator= (float percent) { 
    write(percent);
    return *this;
}

Servo& Servo::operator= (Servo& rhs) {
    write(rhs.read());
    return *this;
}

Servo::operator float() {
    return read();
}
