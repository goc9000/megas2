{
    "entities" : [
                { "id": "mcu", "type": "Atmega32", "firmware": "charliev2.elf", "frequency": 16000000 },
                { "id": "rtc", "type": "Ds1307", "i2c_address": 104 },
                { "id": "sd_card", "type": "SdCard", "image": "fsimage.bin", "capacity": 268435456 },
                { "id": "enc28j60", "type": "Enc28J60" },
                { "type": "I2cBus", "devices": ["mcu", "rtc"] },
                { "type": "SpiBus", "devices": ["mcu", "sd_card", "enc28j60"] },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "B1"}, { "device": "sd_card", "pin": "SS"}] },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "B3"}, { "device": "enc28j60", "pin": "RESET"}] },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "B4"}, { "device": "enc28j60", "pin": "SS"}] }
               ]
}
