#define __AVR_ATtiny85__
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "CapacitiveSensor.h"

// first value is output pin, second value is input pin connected to touch sensor with 1m ohm resistor between
CapacitiveSensor capSensor = CapacitiveSensor(4, 2);
int threshold = 300;

int tap_limit = 50;

// average the sensor signal over this many samples
long avgOver = 16;


#define PWM_DIVISOR 10
#define PWM_MAX (255*PWM_DIVISOR)
#define PWM_MIN 0

// pin the LED is connected to
//const int ledPin = 1;
#define LED_OUTPUT_PIN  1

void setup() {
  pinMode(LED_OUTPUT_PIN, OUTPUT);
  analogWrite(LED_OUTPUT_PIN, 0);
  // make sure we don't have ADC running
  ADCSRA &= ~(1<<ADEN);
}

  int dir = 1;
  long pwm = 0;
  int held = 0;
  int peak = 0;
  long lastValue=0;
  long avgValue=0;
void set_pwm() {
  analogWrite(LED_OUTPUT_PIN, pwm/PWM_DIVISOR);  
}

// move faster at larger values where it isn't noticed
// move slower at smaller values where it is
void pwm_adjust() {
  long offset = pwm / 100;
  if (offset == 0) {
    offset = 1;
  }
  
  if (!dir) offset = -offset;
  if (pwm + offset > PWM_MAX) {
    pwm = PWM_MAX;
    return;
  }
  if (pwm + offset < PWM_MIN) {
    pwm = PWM_MIN;
    return;
  }
  pwm += offset;
}


void loop() {

#ifdef BOGUS

  setup_watchdog(4);
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  sleep_mode();
  sensor_loop();
  controls();

#else

  static uint32_t StartMs=millis();
  uint32_t NowMS = millis();

  if((NowMS-StartMs) >= 10)
  {
    StartMs=NowMS;
    sensor_loop();
    controls();
  }
#endif
}


void sensor_loop() {
  // store the value reported by the sensor in a variable
  long sensorValue = capSensor.capacitiveSensor(30);

  lastValue = avgValue;
  avgValue += (sensorValue / avgOver) - (avgValue / avgOver);
  if (avgValue) avgValue -= 1;

}

void controls() {
  if (avgValue > threshold && (lastValue <= threshold || peak)) {
    if (held < tap_limit) {
      held++;
    }
    if (avgValue > peak) {
      peak = avgValue;
    }
    if (avgValue < peak / 2) {
      // no longer held
      if (held < tap_limit) {
          // let off quickly
          if (dir) {
            pwm = PWM_MAX;
          } else {
            pwm = PWM_MIN;
          }
          set_pwm();
      }
      if (dir) {
        dir = 0;
      } else {
        dir = 1;
      }
      held = 0;
      peak = 0;
    } else {
      // slow adjust
      pwm_adjust();
      set_pwm();
    }
  }
}





//This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
//  digitalWrite(1, LOW);
}



//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
//From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {

  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  char bb = timerPrescaler & 7;
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}

