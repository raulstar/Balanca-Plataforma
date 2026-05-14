#include "Nextion_Display.hpp"

NextionDisplay::NextionDisplay(HardwareSerial &serial)
{
    _serial = &serial;
}

void NextionDisplay::begin(uint32_t baud)
{
    _serial->begin(baud);
}

void NextionDisplay::listen()
{
    while (_serial->available() >= 7)
    {
        uint8_t header = _serial->read();

        if (header != 0x65)
            return;

        uint8_t page = _serial->read();
        uint8_t component = _serial->read();
        uint8_t event = _serial->read();

        // descarta FF FF FF
        _serial->read();
        _serial->read();
        _serial->read();

        processEvent(page, component, event);
    }
}

void NextionDisplay::processEvent(uint8_t page,
                                  uint8_t component,
                                  uint8_t event)
{
    // considera apenas TOUCH PRESS
    if (event != 0x01)
        return;

    // =====================================================
    // PAGE 0
    // =====================================================

    if (page == 0)
    {
        switch (component)
        {
        case 1:
            if (on_bsom)
                on_bsom();
            break;

        case 2:
            if (on_bconf)
                on_bconf();
            break;

        case 3:
            if (on_bimprir)
                on_bimprir();
            break;

        case 4:
            if (on_bhis)
                on_bhis();
            break;

        case 5:
            if (on_bsalvar)
                on_bsalvar();
            break;

        case 6:
            if (on_bzero)
                on_bzero();
            break;

        case 7:
            if (on_blimpar)
                on_blimpar();
            break;
        }
    }

    // =====================================================
    // PAGE 1
    // =====================================================

    else if (page == 1)
    {
        switch (component)
        {
        case 1:
            if (on_bexport)
                on_bexport();
            break;

        case 2:
            if (on_bhome)
                on_bhome();
            break;

        case 3:
            if (on_bimprir)
                on_bimprir();
            break;

        case 4:
            if (on_bzero)
                on_bzero();
            break;

        case 5:
            if (on_b0)
                on_b0();
            break;
        }
    }

    // =====================================================
    // PAGE 2
    // =====================================================

    else if (page == 2)
    {
        switch (component)
        {
        case 1:
            if (on_bzero)
                on_bzero();
            break;

        case 2:
            if (on_bsom)
                on_bsom();
            break;

        case 3:
            if (on_bsalvar)
                on_bsalvar();
            break;

        case 4:
            if (on_bexport)
                on_bexport();
            break;

        case 5:
            if (on_bimprir)
                on_bimprir();
            break;

        case 6:
            if (on_bhome)
                on_bhome();
            break;
        }
    }
}