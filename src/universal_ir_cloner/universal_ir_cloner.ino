#include <IRremote.hpp>
#include <LiquidCrystal.h>

// ================= PINS =================

// IR
#define IR_RECV_CAPTURE 2
#define IR_RECV_VERIFY  3
#define IR_SEND_PIN     43

// Buttons
#define BTN_CAPTURE 4
#define BTN_REPLAY  5
#define BTN_CLEAR   6

// LCD
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// ================= SETTINGS =================

#define RAW_BUFFER_SIZE 750
#define MATCH_THRESHOLD 0.85

uint16_t rawBuffer[RAW_BUFFER_SIZE];
uint16_t verifyBuffer[RAW_BUFFER_SIZE];

uint16_t rawLength = 0;
uint16_t verifyLength = 0;

bool hasSignal = false;
float lastSimilarityPercent = 0.0;

// ================= BUTTON =================

bool buttonPressed(int pin)
{
    if (digitalRead(pin) == HIGH)
    {
        delay(40);
        return digitalRead(pin) == HIGH;
    }
    return false;
}

// ================= LCD =================

void lcdMsg(const char* l1, const char* l2 = "")
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(l1);

    lcd.setCursor(0, 1);
    lcd.print(l2);
}

// ================= PRINT ARRAY =================

void printArray(uint16_t buffer[], uint16_t length)
{
    Serial.print("RAW length: ");
    Serial.println(length);

    for (int i = 0; i < length; i++)
    {
        Serial.print(buffer[i]);
        Serial.print(",");

        if ((i + 1) % 10 == 0)
            Serial.println();
    }

    Serial.println();
    Serial.println("----------------------");
}

// ================= SIGNAL COMPARISON =================

bool signalsMatch()
{
    const int MAX_SHIFT = 3;

    long bestErrorSum = 999999999;
    int bestCount = 1;

    for (int shift = -MAX_SHIFT;
         shift <= MAX_SHIFT;
         shift++)
    {
        long errorSum = 0;
        int count = 0;

        for (int i = 2; i < rawLength; i++)
        {
            int j = i + shift;

            if (j < 2 || j >= verifyLength)
                continue;

            int diff =
                abs((int)rawBuffer[i] -
                    (int)verifyBuffer[j]);

            errorSum += diff;
            count++;
        }

        if (count > 0 &&
            errorSum < bestErrorSum)
        {
            bestErrorSum = errorSum;
            bestCount = count;
        }
    }

    float avgError =
        (float)bestErrorSum /
        (float)bestCount;

    float similarity =
        1.0 - (avgError / 600.0);

    if (similarity < 0)
        similarity = 0;

    lastSimilarityPercent =
        similarity * 100.0;

    Serial.print("Average timing error: ");
    Serial.print(avgError);
    Serial.println(" us");

    Serial.print("Similarity: ");
    Serial.print(lastSimilarityPercent);
    Serial.println("%");

    return similarity >= MATCH_THRESHOLD;
}

// ================= FORMAT PERCENT =================

void formatPercent(char *buffer)
{
    int whole = (int)lastSimilarityPercent;

    int decimal =
        (int)((lastSimilarityPercent -
               whole) * 10);

    sprintf(buffer,
            "%d.%d%%",
            whole,
            decimal);
}

// ================= SETUP =================

void setup()
{
    Serial.begin(115200);

    lcd.begin(16, 2);

    pinMode(BTN_CAPTURE, INPUT);
    pinMode(BTN_REPLAY, INPUT);
    pinMode(BTN_CLEAR, INPUT);

    IrReceiver.begin(IR_RECV_CAPTURE);
    IrSender.begin(IR_SEND_PIN);

    lcdMsg("IR CLONER", "READY");

    Serial.println("System ready");
}

// ================= LOOP =================

void loop()
{
    // -------- CAPTURE --------

    if (buttonPressed(BTN_CAPTURE))
    {
        Serial.println("CAPTURE button pressed");

        lcdMsg("Capturing...");

        while (!IrReceiver.decode());

        rawLength =
            IrReceiver.irparams.rawlen;

        if (rawLength > RAW_BUFFER_SIZE)
            rawLength = RAW_BUFFER_SIZE;

        for (int i = 0; i < rawLength; i++)
        {
            rawBuffer[i] =
                IrReceiver.irparams.rawbuf[i]
                * MICROS_PER_TICK;
        }

        hasSignal = true;

        Serial.println(
            "Captured original signal:"
        );

        printArray(
            rawBuffer,
            rawLength
        );

        IrReceiver.resume();

        lcdMsg("Captured");
    }

    // -------- REPLAY --------

    if (buttonPressed(BTN_REPLAY))
    {
        Serial.println("REPLAY button pressed");

        if (!hasSignal)
        {
            lcdMsg("No Signal");
            return;
        }

        lcdMsg("Transmitting");

        IrReceiver.stop();

        delay(50);

        IrReceiver.begin(IR_RECV_VERIFY);

        delay(100);

        for (int i = 0; i < 3; i++)
        {
            IrSender.sendRaw(
                rawBuffer,
                rawLength,
                38
            );

            delay(200);
        }

        unsigned long startTime =
            millis();

        bool received = false;

        while (millis() -
               startTime < 2000)
        {
            if (IrReceiver.decode())
            {
                received = true;

                verifyLength =
                    IrReceiver.irparams.rawlen;

                if (verifyLength > RAW_BUFFER_SIZE)
                    verifyLength =
                        RAW_BUFFER_SIZE;

                for (int i = 0;
                     i < verifyLength;
                     i++)
                {
                    verifyBuffer[i] =
                        IrReceiver.irparams.rawbuf[i]
                        * MICROS_PER_TICK;
                }

                Serial.println(
                    "Received transmitted signal:"
                );

                printArray(
                    verifyBuffer,
                    verifyLength
                );

                bool match =
                    signalsMatch();

                char percentText[17];

                formatPercent(
                    percentText
                );

                if (match)
                    lcdMsg(
                        "SIGNALS",
                        percentText
                    );
                else
                    lcdMsg(
                        "DIFFERENT",
                        percentText
                    );

                IrReceiver.resume();

                break;
            }
        }

        if (!received)
        {
            lcdMsg("NO SIGNAL");
        }

        IrReceiver.begin(
            IR_RECV_CAPTURE
        );
    }

    // -------- CLEAR --------

    if (buttonPressed(BTN_CLEAR))
    {
        Serial.println("CLEAR button pressed");

        hasSignal = false;
        rawLength = 0;

        lcdMsg("Cleared");

        Serial.println("Memory cleared");
    }
}
