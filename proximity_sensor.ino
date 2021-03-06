#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//=======================================================================================
// configurations

// aliases
using pin_t = uint8_t;
using LCD = LiquidCrystal_I2C;


// pins initialization
//-------------------------------------------------------------------------------------------------------------

// input button pin
constexpr pin_t g_buttonFlagPin { 2 };      // on digital pin 2

// ultrasonic pins
constexpr pin_t g_trigPin { 3 };            // on digital pin 3
constexpr pin_t g_echoPin { 4 };            // on digital pin 4

// for lcd_i2c, SDA goes to A4, and SCL goes to A5 (on arduino UNO (can't be changed unless the board differs))
//-------------------------------------------------------------------------------------------------------------


// configuration variables
//--------------------------------------------------------------------------------------------------

// lcd
constexpr uint8_t g_lcdAddress { 0x27 };              // see online on how to search for lcd address

// speed of sound
constexpr double g_defaultSpeedOfSound { 340 };       // default speed of sound value (in m/s)
constexpr double g_calibrateDistance { 0.1 };         // in meters (change freely)
//---------------------------------------------------------------------------------------------------


//=======================================================================================
// function forward declarations

bool getFlag();
bool waitForInput(const int);
double calibrate(const pin_t, const pin_t, const double, LCD&);
double measureDistance(const pin_t, const pin_t, const double);
double measureSoundSpeed(const pin_t, const pin_t, const double);


//=======================================================================================
// main program

// init variables that used in loop()
//------------------------------------------------

// instantiate lcd object
LCD lcd { g_lcdAddress, 16, 2 };

// intialize measured distance and speed of sound
double measuredDistance {};
double speedOfSound { g_defaultSpeedOfSound };
//------------------------------------------------


// main setup
void setup()
{
    // set pins mode
    pinMode(g_buttonFlagPin, INPUT);
    pinMode(g_trigPin, OUTPUT);
    pinMode(g_echoPin, INPUT);

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
    measuredDistance = 100 * measureDistance(g_trigPin, g_echoPin, speedOfSound);        // in cm
    lcd.setCursor(0, 1);
    lcd.print("             ");
    lcd.setCursor(0, 1);
    lcd.print(measuredDistance, 2);
    
    // serial
    Serial.print(measuredDistance, 2);
    Serial.println("\tcm");

    // delay 1 second, if it returns true, go to calibrate
    if (waitForInput(1000))
        speedOfSound = calibrate(g_trigPin, g_echoPin, g_calibrateDistance, lcd);
}


//=======================================================================================
// forward declared function definitions

// return true if button pressed, false if otherwise
bool getFlag()
{
    return digitalRead(g_buttonFlagPin);
}

// wait for input from button: return true if pressed, otherwise return false if not pressed and [duration] in miliseconds has elapsed
bool waitForInput(const int durationInMs)
{
    const int delayInMs { 10 };
    int duration { durationInMs / delayInMs };

    for(int count { duration }; count != 0; --count)
    {
        delay(delayInMs);
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
    
    // wait for input for 10 seconds
    if (!waitForInput(10000))
    {
        // cancelled
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Canceled      :(");
        lcd.setCursor(0, 1);
        lcd.print("UseDefaultSpeed");
        delay(3000);
        lcd.clear();
        return g_defaultSpeedOfSound;
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

    // wait for input for 10 seconds
    if (!waitForInput(10000))
    {
        // canceled
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Canceled      :(");
        lcd.setCursor(0, 1);
        lcd.print("UseDefaultSpeed");
        delay(3000);
        lcd.clear();
        return g_defaultSpeedOfSound;
    }

    lcd.clear();
    delay(1000);
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