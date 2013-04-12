// Note: this is part of class Atmega32

public:


protected:
    bool adc_enabled;
    uint16_t adc_result;
    bool adc_result_locked;
    
    void _adcInit();
    void _adcHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _adcHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);

    uint8_t _handleAdcIrqs();
    
    void _setAdcEnabled(bool enabled);
