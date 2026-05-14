#ifndef NEXTION_DISPLAY_HPP
#define NEXTION_DISPLAY_HPP

#include <Arduino.h>

class NextionDisplay
{
public:
    NextionDisplay(HardwareSerial &serial);

    void begin(uint32_t baud = 115200);

    void listen();

    // CALLBACKS
    void (*on_bsom)();
    void (*on_bconf)();
    void (*on_bimprir)();
    void (*on_bhis)();
    void (*on_bsalvar)();
    void (*on_bzero)();
    void (*on_blimpar)();
    void (*on_bexport)();
    void (*on_bhome)();
    void (*on_b0)();

private:
    HardwareSerial *_serial;

    void processEvent(uint8_t page,
                      uint8_t component,
                      uint8_t event);
};

#endif