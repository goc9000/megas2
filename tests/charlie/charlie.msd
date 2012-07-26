{
    "entities" : [
                { "id": "mcu", "type": "Atmega32", "firmware": "charlie32.elf", "frequency": 16000000 },
                { "id": "rtc", "type": "Ds1307", "slave_id": 104 },
                { "id": "sd_card", "type": "SdCard", "image": "fsimage.bin", "capacity": 268435456 },
                { "id": "enc28j60", "type": "Enc28J60" },
                { "type": "I2cBus", "devices": ["mcu", "rtc"] },
                { "type": "SpiBus", "devices": ["mcu", "sd_card", "enc28j60"] },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "B1"}, { "device": "sd_card", "pin": "SS"}] },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "B3"}, { "device": "enc28j60", "pin": "RESET"}] },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "B4"}, { "device": "enc28j60", "pin": "SS"}] },
                { "id": "debug_led", "type": "SimpleLed", "x": 16, "y": 16, "size": 16, "color": {"r":255, "g":0, "b":0}, "caption": "Debug LED" },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "D7"}, { "device": "debug_led", "pin": "INPUT"}] },
                { "id": "debug_btn", "type": "SimplePushButton", "up_value": "Z", "down_value": 0, "x": 16, "y": 40, "size": 16, "color": {"r":0, "g":0, "b":0}, "caption": "Debug button" },
                { "type": "AnalogBus", "pins": [{ "device": "mcu", "pin": "D6"}, { "device": "debug_btn", "pin": "OUTPUT"}] },
                { "id": "dashboard", "type": "Dashboard", "background": "charlie_dashboard_bkgd.png", "widgets": ["debug_led", "debug_btn"] }
               ]
}
