// Note: this is part of class Atmega32

public:


protected:
    int prescaler01;
    int prescaler2;

    void _timersInit();
    void _runTimers();
    void _tickTimer0();
    void _tickTimer1();
    void _tickTimer2();
    void _timersHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _timersHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
