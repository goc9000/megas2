// Note: this is part of class Atmega32

public:

protected:
    void _pinsInit();
    virtual void _onPinChanged(int pin_id, int value, int old_value);
    
    void _handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _handleDataPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
