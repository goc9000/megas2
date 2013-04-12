// Note: this is part of class Atmega32

public:

protected:
    vector<bool> _pin_overrides;

    void _pinsInit();
    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value);
    
    void _handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _handleDataPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);

    void _enablePinOverride(int pin_id, int mode, pin_val_t float_value);
    void _disablePinOverride(int pin_id);
