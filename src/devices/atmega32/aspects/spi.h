// Note: this is part of class Atmega32

public:
    virtual bool spiReceiveData(uint8_t &data);

protected:
    bool spi_stat_read;
        
    void _spiInit();
    void _spiHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _spiHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
    bool _spiIsEnabled();
    virtual void _onSpiSlaveSelect(bool select);
