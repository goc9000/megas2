// Note: this is part of class Atmega32

public:
    virtual void i2cReceiveStart();
    virtual bool i2cReceiveAddress(uint8_t address, bool write);
    virtual bool i2cReceiveData(uint8_t data);
    virtual bool i2cQueryData(uint8_t &data);
    virtual void i2cReceiveStop();

protected:
    bool twi_has_floor;
    bool twi_start_just_sent;
    bool twi_xmit_mode;

    void _twiInit();
    void _twiHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _twiHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
    
    void _twiDoSendStart();
    void _twiDoSendStop();
    void _twiDoSendAddress();
    void _twiDoSendData();
    void _twiDoReceiveData();
    void _twiCompleteOp(uint8_t status);
