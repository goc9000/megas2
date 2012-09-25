// Note: this is part of class Atmega32

public:


protected:
    void _dumpRegisters();
    void _dumpSram();
    void _dumpPortRead(const char *header, uint8_t port, int8_t bit, uint8_t &value);
    void _dumpPortWrite(const char *header, uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
