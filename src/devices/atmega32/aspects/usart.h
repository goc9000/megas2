// Note: this is part of class Atmega32

public:

protected:
    uint8_t _reg_UCSRC;
    uint64_t _last_UBRRH_access;

    void _usartInit();
    void _usartHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _usartHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
