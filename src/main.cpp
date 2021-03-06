#include <Arduino.h>
#include <esp32_utilities.h>
#include <CAN.h>
#include <Adafruit_NeoPixel.h>

//**************************************** Pins
#define PIN 23
#define NUMPIXELS 16
#define DELAYVAL 50

//**************************************** Macros
#define debugMessage(X) terminal.printMessage(TerminalMessage(String(X), "APP", INFO, micros()))
//#define debugMessage(X, Y) terminal.printMessage(TerminalMessage(String(X), "APP", INFO, micros(), micros() - Y))

//**************************************** Objects
SystemOnChip esp;
Terminal terminal;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//**************************************** Tasks
QueueHandle_t rpm_queue;

void CAN_readTask(void *parameters);
void RPM_displayTask(void *parameters);
void gearDisplayTask(void *parameters);

void setup()
{
    //* 1. Begin debug port
    esp.uart0.begin(115200);
    terminal.begin(esp.uart0);

    //* 2. Begin RTOS Objects
    rpm_queue = xQueueCreate(100, sizeof(uint16_t));
    if (rpm_queue == NULL)
    {
        debugMessage("Error creating the rpm object queue");
        abort();
    }

    //* 3. Begin LED arrays
    pixels.begin();
    pixels.clear();

    //* 4. Begin esp CAN controller
    if (!CAN.begin(1000E3))
    {
        debugMessage("Error initializing CAN controller");
        abort();
    }

    debugMessage("CAN controller initialized");

    //* 5. Create Tasks
    xTaskCreatePinnedToCore(CAN_readTask, "CAN read", 10000, nullptr, 1, nullptr, 0);
    xTaskCreatePinnedToCore(RPM_displayTask, "RPM display", 10000, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(gearDisplayTask, "Gear display", 10000, nullptr, 1, nullptr, 1);

    vTaskDelete(NULL);
}

void CAN_readTask(void *parameters)
{
    String message = "";
    uint8_t data[8];

    while (1)
    {
        int packet_size = CAN.parsePacket();

        if (packet_size)
        {
            // 1 Get message ID
            long message_id = CAN.packetId();

            // 2 Output the whole message to the terminal for debugging
            if (CAN.packetExtended())
                message += " EXT ";

            message += "ID:0x";
            message += String(message_id, HEX);

            if (CAN.packetRtr())
            {
                message += " RTR rqt length:";
                message += CAN.packetDlc();
                "\t";
            }
            else
            {
                message += " msg. length:";
                message += packet_size;
                message += "\t data: ";

                while (CAN.available())
                {
                    message += (char)CAN.read();

                    uint8_t data_length = CAN.readBytes(data, 8);
                }

                debugMessage(message);
            }

            // 3 Switch message and add data to corresponding queue
            switch (message_id)
            {
            case 1907: // 0x773
                debugMessage("RPM message received");
                break;
            case 1911:
                debugMessage("Gear message received");
                break;

            default:
                break;
            }
        }
        else
        {
            // if no message is detected on bus idle for some time
            delay(10);
        }
    }
}

void RPM_displayTask(void *parameters)
{
    uint32_t ShiftLightColors[] = {
        /// Green
        pixels.Color(0, 150, 0),
        pixels.Color(0, 150, 0),
        pixels.Color(0, 150, 0),
        // Yellow
        pixels.Color(170, 60, 0),
        pixels.Color(170, 60, 0),
        pixels.Color(170, 60, 0),
        // Orange
        pixels.Color(240, 40, 0),
        pixels.Color(240, 40, 0),
        pixels.Color(200, 40, 0),
        // Red
        pixels.Color(255, 0, 0),
        pixels.Color(255, 0, 0),
        pixels.Color(255, 0, 0),
    };

    while (1)
    {
        pixels.clear(); // The first NeoPixel in a strand is #0, second is 1, all the way up
        // to the count of pixels minus one.
        for (int i = 0; i < 12; i++)
        { // For each pixel...

            // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
            // Here we're using a moderately bright green color:
            pixels.setPixelColor(i, ShiftLightColors[i]);

            pixels.show(); // Send the updated pixel colors to the hardware.

            delay(DELAYVAL); // Pause before next pass through loop
        }
    }
}

void gearDisplayTask(void *parameters)
{
    while (1)
        ;
}

void loop() {}