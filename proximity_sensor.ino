#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//=======================================================================================
// configurations

// aliases
using pin_t = uint8_t;
using LCD = LiquidCrystal_I2C;

namespace       // pins initialization
{
    // input button pin
    const pin_t buttonFlagPin_g { 2 };      // on digital pin 2

    // ultrasonic pins
    const pin_t trigPin_g { 3 };            // on digital pin 3
    const pin_t echoPin_g { 4 };            // on digital pin 4

    // for lcd_i2c, SDA goes to A4, and SCL goes to A5 (on arduino UNO (can't be changed unless the board differs))
}

namespace       // configuration variables
{
    // lcd
    const uint8_t lcdAddress_g { 0x27 };              // see online on how to search for lcd address
    
    // speed of sound
    const double defaultSpeedOfSound_g { 340 };       // default speed of sound value (in m/s)
    const double calibrateDistance_g { 0.1 };         // in meters (change freely)
}

namespace       // init variables, used on loop()
{
    // instantiate lcd object
    LCD lcd { lcdAddress_g, 16, 2 };

    // intialize measured distance and speed of sound
    double measuredDistance {};
    double speedOfSound { defaultSpeedOfSound_g };
}


//=======================================================================================
// function forward declarations

bool getFlag();
bool waitForInput(const int);
double calibrate(const pin_t, const pin_t, const double, LCD&);
double measureDistance(const pin_t, const pin_t, const double);
double measureSoundSpeed(const pin_t, const pin_t, const double);


// main setup
void setup()
{
    // set pins mode
    pinMode(buttonFlagPin_g, INPUT);
    pinMode(trigPin_g, OUTPUT);
    pinMode(echoPin_g, INPUT);

    // start serial
    Serial.begin(9600);

    // lcd initialization
    lcd.init();
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.clear();

    // lcd greet
    lcd.setCursor(0, 0);
    const char greet[] {"Praktikum PBL Sensor dan Transduser"};
    lcd.print(greet);
    delay(2000);
    for(int i{ 0 }; i < sizeof(greet)/sizeof(char)-17; ++i)         // 17 comes from the max length of char can be displayed on one row of lcd (16), and null string terminator (1) 
    {
        lcd.scrollDisplayLeft();
        delay(250);
    }
    delay(2000);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Proximity");
    lcd.setCursor(0, 1);
    lcd.print("Sensor");
    delay(2000);
    lcd.clear();
}

// main loop
void loop()
{
    lcd.setCursor(0, 0);
    lcd.print("Distance: ");
    lcd.setCursor(14, 1);
    lcd.print("cm");

    // measure distance
    measuredDistance = 100 * measureDistance(trigPin_g, echoPin_g, speedOfSound);        // in cm
    lcd.setCursor(0, 1);
    lcd.print("             ");
    lcd.setCursor(0, 1);
    lcd.print(measuredDistance, 3);
    
    // serial
    Serial.print(measuredDistance, 3);
    Serial.println("\tcm");

    // delay 1 second, if it returns true, go to calibrate
    if (waitForInput(1))
        speedOfSound = calibrate(trigPin_g, echoPin_g, calibrateDistance_g, lcd);
}


//=======================================================================================
// function definitions

// return true if button pressed, false if otherwise
bool getFlag()
{
    return digitalRead(buttonFlagPin_g);
}

// wait for input from button: return true if pressed, otherwise return false if not pressed and [duration] in seconds has elapsed
bool waitForInput(const int duration)
{
    for(int count { duration*10 }; count != 0; --count)
    {
        delay(100);
        if (getFlag()) return true;
    }

    return false;
}

// calibrate the speed of sound by measuring it 10 times then averaging it
double calibrate(const pin_t trigPin, const pin_t echoPin, const double calibrateDistance, LCD& lcd)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrate Mode");
    lcd.setCursor(0, 1);
    lcd.print("PressToContinue");
    delay(500);
    
    // cancelled
    if (!waitForInput(10))
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Canceled      :(");
        lcd.setCursor(0, 1);
        lcd.print("UseDefaultSpeed");
        delay(3000);
        lcd.clear();
        return defaultSpeedOfSound_g;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");

    int duration { 10 };                                    // in seconds
    double soundSpeed_sum { 0 };
    // measure speed of sound every one second
    for (int t { duration }; t > 0; --t)
    {
        lcd.setCursor(14, 0);
        // timer
        if (t < 10)
            lcd.print(' ');
        lcd.print(t);

        // measure speed (and sum it with the previous measured speed)
        double soundSpeed_measured { measureSoundSpeed(trigPin, echoPin, calibrateDistance) };
        soundSpeed_sum += soundSpeed_measured;

        // display currently measured speed
        lcd.setCursor(0, 1);
        lcd.print("             ");
        lcd.setCursor(0, 1);
        lcd.print(soundSpeed_measured);
        lcd.setCursor(13, 1);
        lcd.print("m/s");
        delay(1000);
    }

    // speed of sound averaged
    const double soundSpeed { soundSpeed_sum / static_cast<double>(duration) };      // take average

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Done  ");
    lcd.print(soundSpeed);
    lcd.setCursor(13, 0);
    lcd.print("m/s");
    lcd.setCursor(0, 1);
    lcd.print("PressToContinue");
    delay(500);

    // canceled
    if (!waitForInput(20))
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Canceled      :(");
        lcd.setCursor(0, 1);
        lcd.print("UseDefaultSpeed");
        delay(3000);
        lcd.clear();
        return defaultSpeedOfSound_g;
    }

    lcd.clear();

    return soundSpeed;
}

// measure the speed of sound in meter/seconds (using distance in meters)
double measureSoundSpeed(const pin_t trigPin, const pin_t echoPin, const double distance)
{
    // send pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(10);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // wait for echo to arrive, pulseIn() returns time when echo arrives
    double duration { static_cast<double>(pulseIn(echoPin, HIGH)) };    // in microseconds
    duration /= 1e6;                                                    // in seconds

    // calcuate speed
    double soundSpeed { 2 * distance / duration };                      // v = 2s / t   (in m/s)

    return soundSpeed;
}

// measure distance using sound in meters
double measureDistance(const pin_t trigPin, const pin_t echoPin, const double soundSpeed)
{
    // send pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(10);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // wait for echo to arrive, pulseIn() returns time when echo arrives
    double duration { static_cast<double>(pulseIn(echoPin, HIGH)) };    // in microseconds
    duration /= 1e6;                                                    // in seconds

    // calculate distance
    double distance { soundSpeed * duration / 2.0 };                    // v = 2s / t  -->   2s = v * t   -->   s = v * t / 2    (in m)

    return distance;
}

//=======================================================================================
// end