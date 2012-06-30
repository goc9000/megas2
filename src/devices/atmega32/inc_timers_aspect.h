// Note: this is part of class Atmega32

public:


protected:
    int prescaler01;
    int prescaler2;
    
    uint8_t timer1_temp_high_byte;

    void _timersInit();
    void _runTimers();
    void _timersHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _timersHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);

    void _triggerTimerIrq(uint8_t flags);
    uint8_t _handleTimerIrqs();

    void _timer0Init();
    void _timer0Tick();
    void _timer0HandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _timer0HandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    
    void _timer1Init();
    void _timer1Tick();
    void _timer1HandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _timer1HandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    
    void _timer2Init();
    void _timer2Tick();
    void _timer2HandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _timer2HandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
