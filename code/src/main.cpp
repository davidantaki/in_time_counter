#include <Arduino.h>
#include <DS3231.h>
#include <SparkFun_Alphanumeric_Display.h>

TwoWire i2c1_bus(PB7, PB6);
HT16K33 display;
HardwareSerial serialUSB(PA3, PA2); // rx, tx
DS3231 rtc(i2c1_bus);

// Birth Day
int birth_yr = 2000;
int birth_mo = 3;
int birth_day = 27;
// Age you will die
int useful_lifetime_age = 70;

void loading() {
  display.illuminateChar(0b0000000000000001, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000000011, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000000111, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000001111, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000011111, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000111111, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000111110, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000111100, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000111000, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000110000, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000100000, 0);
  display.updateDisplay();
  delay(100);
  display.illuminateChar(0b0000000000000000, 0);
  display.updateDisplay();
  delay(100);
}

/*
3 displays with all segments on draws 0.572A.
Don't think the 4th turned on because not enough power.
*/
void current_draw_test() { display.write("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"); }

void counter() {
  int last_sec_left_ms_update_ms = 0;
  int last_time_left_to_live_update_time_ms = 0;
  int yrs_left = 70;
  int wks_left = 52;
  int days_left = 7;
  int hrs_left = 24;
  int min_left = 59;
  int sec_left_ms = 60 * 1000;
  int ms_left = 999;
  uint64_t time_left_to_live_s = 70 * 365 * 24 * 60 * 60; // 70yrs

  if (sec_left_ms <= 0) {
    sec_left_ms = 59 * 1000;
    min_left -= 1;
    serialUSB.printf(
        "millis(): %d\tyrs_left: %d\twks_left: %d\tdays_left: %d\thrs_left: "
        "%d\tmin_left: %d\tsec_left_ms: %d\tms_left: %d\n",
        millis(), yrs_left, wks_left, days_left, hrs_left, min_left,
        sec_left_ms, ms_left);
  }
  if (min_left <= 0) {
    min_left = 59;
    hrs_left -= 1;
  }
  if (hrs_left <= 0) {
    hrs_left = 24;
    hrs_left -= 1;
  }
  if (min_left <= 0) {
    min_left = 59;
    hrs_left -= 1;
  }

  sec_left_ms -= millis() - last_sec_left_ms_update_ms;
  last_sec_left_ms_update_ms = millis();

  auto display_str = std::to_string(yrs_left);
  // display.printf(display_str.c_str());
  // serialUSB.printf(display_str.c_str());
  display.printf("%d%d%d%d%d%d%d", yrs_left, wks_left, days_left, hrs_left,
                 min_left, sec_left_ms);
}

/**
 * Given year, mo, day, hr, min, sec, return the time in sec since the Unix
 * epoch acounting for leap years.
 */
int get_sec_since_epoch(int yr, int mo, int day, int hr, int min, int sec) {
  int time_since_epoch_sec = 0;
  // Add a day if a leap year (i.e. if the year is divisible by 4).
  // Does't include current year.
  for (int i = 1970; i < yr; i++) {
    if (i % 4 == 0) {
      time_since_epoch_sec += 366 * 24 * 60 * 60;
    } else {
      time_since_epoch_sec += 365 * 24 * 60 * 60;
    }
  }

  // Don't include the current month
  for (int i = 1; i < mo; i++) {
    if (mo == 1) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    } else if (mo == 2) {
      // Check if current year is a leap year
      if (yr % 4 == 0) {
        time_since_epoch_sec += 29 * 24 * 60 * 60;
      } else {
        time_since_epoch_sec += 28 * 24 * 60 * 60;
      }
    } else if (mo == 3) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    } else if (mo == 4) {
      time_since_epoch_sec += 30 * 24 * 60 * 60;
    } else if (mo == 5) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    } else if (mo == 6) {
      time_since_epoch_sec += 30 * 24 * 60 * 60;
    } else if (mo == 7) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    } else if (mo == 8) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    } else if (mo == 9) {
      time_since_epoch_sec += 30 * 24 * 60 * 60;
    } else if (mo == 10) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    } else if (mo == 11) {
      time_since_epoch_sec += 30 * 24 * 60 * 60;
    } else if (mo == 12) {
      time_since_epoch_sec += 31 * 24 * 60 * 60;
    }
  }

  time_since_epoch_sec += day * 24 * 60 * 60;
  time_since_epoch_sec += hr * 60 * 60;
  time_since_epoch_sec += min * 60;
  time_since_epoch_sec += sec;

  return time_since_epoch_sec;
}

/**
 * @brief Counter using a real time clock.
 */
void rtc_counter() {
  static int last_print_time;

  // Calculate expected death date
  int death_yr = birth_yr + useful_lifetime_age;
  int death_mo = birth_mo;
  int death_day = birth_day;
  int death_hr = 12;
  int death_min = 0;
  int death_sec = 0;

  // Get current date time
  bool century, h12, pm_time;
  int curr_yr = 1970 + rtc.getYear();
  int curr_mo = rtc.getMonth(century);
  int curr_day = rtc.getDate();
  int curr_hr = rtc.getHour(h12, pm_time);
  int curr_min = rtc.getMinute();
  int curr_sec = rtc.getSecond();

  // // Calculate time remaining in your life.
  // // Note: day_left and months_left will always =0 because we assume you will
  // // die on your birthday.
  // int sec_left = death_sec - curr_sec;
  // if (sec_left < 0) {
  //   // Borrow from the death_min
  //   death_min -= 1;
  //   sec_left += 60;
  // }
  // int min_left = death_min - curr_min;
  // if (min_left < 0) {
  //   // Borrow from the death_hr
  //   death_hr -= 1;
  //   min_left += 60;
  // }
  // int hr_left = death_hr - curr_hr;
  // if (hr_left < 0) {
  //   // Borrow from the death_yr because days and months left should always be
  //   0.
  //   // Also this way we don't have to track leap years.
  //   death_yr -= 1;
  //   hr_left += 365 * 24;
  // }
  // int day_left = curr_day;
  // int yr_left = death_yr - curr_yr;

  // if (millis() - last_print_time > 1000) {
  //   last_print_time = millis();
  //   // Display time remaining in your life
  //   serialUSB.printf("millis(): %d yr_left: %d mo_left: "
  //                    "%d day_left: %d hr_left: "
  //                    "%d min_left: %d sec_left: %d\r\n",
  //                    millis(), yr_left, mo_left, days_left, hr_left,
  //                    min_left, sec_left);
  // }
  // // Calc time left in hours
  // int time_left_in_hrs = yr_left * 365 * 24 + hr_left;

  // // Calc time left in new format
  // yrs_left = time_left

  // Calc time remaining in life in seconds
  int death_time_since_epoch_sec = get_sec_since_epoch(
      death_yr, death_mo, death_day, death_hr, death_min, death_sec);
  int curr_time_since_epoch_sec = get_sec_since_epoch(
      curr_yr, curr_mo, curr_day, curr_hr, curr_min, curr_sec);
  int time_remaining_sec =
      death_time_since_epoch_sec - curr_time_since_epoch_sec;

  // Put time into new format
  int time_remaining_sec_temp = time_remaining_sec;
  int yrs_left = time_remaining_sec_temp / (365 * 24 * 60 * 60);
  time_remaining_sec_temp -= yrs_left * 365 * 24 * 60 * 60;
  int wks_left = time_remaining_sec_temp / (7 * 24 * 60 * 60);
  time_remaining_sec_temp -= wks_left * 7 * 24 * 60 * 60;
  // ASSERT(wks_left<52)
  int days_left = time_remaining_sec_temp / (24 * 60 * 60);
  time_remaining_sec_temp -= days_left * 24 * 60 * 60;
  // ASSERT(days_left<7)
  int hrs_left = time_remaining_sec_temp / (60 * 60);
  time_remaining_sec_temp -= hrs_left * 60 * 60;
  // ASSERT(hrs_left<24)
  int mins_left = time_remaining_sec_temp / 60;
  time_remaining_sec_temp -= mins_left * 60;
  // ASSERT(mins_left<60)
  int sec_left = time_remaining_sec_temp;
  // ASSERT(sec_left<60)

  if (millis() - last_print_time > 1000) {
    last_print_time = millis();
    // Display time remaining in your life
    serialUSB.printf("death_time_since_epoch_sec: %d\r\n",
                     death_time_since_epoch_sec);
    serialUSB.printf("curr_time_since_epoch_sec: %d\r\n",
                     curr_time_since_epoch_sec);
    serialUSB.printf("total_wks_left: %d\r\n",
                     time_remaining_sec / (7 * 24 * 60 * 60));
    serialUSB.printf("total_days_left: %d\r\n",
                     time_remaining_sec / (24 * 60 * 60));
    serialUSB.printf("total_min_left: %d\r\n", time_remaining_sec / 60);
    serialUSB.printf("total_sec_left: %d\r\n", time_remaining_sec);
    serialUSB.printf(
        "millis(): %d yrs_left: %d wks_left: %d days_left: %d hrs_left: "
        "%d min_left: %d sec_left: %d\r\n",
        millis(), yrs_left, wks_left, days_left, hrs_left, mins_left, sec_left);
  }
}

void john_is_so_hot_mode() { display.print("JOHN IS SO HOT"); }

void counter_mode() {
  // counter();
  rtc_counter();
}

void print_rtc_date_and_time() {
  bool century, h12, pm_time;

  byte yr = rtc.getYear();
  byte mo = rtc.getMonth(century);
  byte hr = rtc.getHour(h12, pm_time);
  byte min = rtc.getMinute();
  byte sec = rtc.getSecond();
  byte date = rtc.getDate();

  serialUSB.printf("RTC: yr: %d mo: %d date: %d hr: %d min: %d sec: %d 12hr: "
                   "0x%X pm_time: 0x%X\r\n",
                   yr + 1970, mo, date, hr, min, sec, h12, pm_time);
}

void setup() {
  // put your setup code here, to run once:
  serialUSB.begin(115200);
  i2c1_bus.begin();
  // put your main code here, to run repeatedly:
  serialUSB.printf("George is Cool\r\n");
  while (display.begin(0x70, 0x71, 0x72, 0x73, i2c1_bus) == false) {
    serialUSB.printf("Display did not acknowledge!\r\n");
    delay(1000);
  }
  serialUSB.printf("Display acknowledged.\r\n");

  display.printf("JOHN IS SO HOT");
  // display.setBlinkRate(1);
  display.setBrightness(1);
  // display.colonOn();
  // display.decimalOn();
  // current_draw_test();
  display.clear();

  // rtc.setYear(2022 - 1970);
  // rtc.setMonth(5);
  // rtc.setDate(22);
  // rtc.setHour(0);
  // rtc.setMinute(15);
  // rtc.setSecond(0);
  rtc.setClockMode(true);
}

void displayFakeColon(uint8_t digit) {
  display.illuminateChar(0b0001001000000000, digit);
  display.updateDisplay();
}

void loop() {
  // john_is_so_hot_mode();
  counter_mode();
  // loading();

  static int last_print_time;
  if (millis() - last_print_time > 1000) {
    last_print_time = millis();
    serialUSB.printf("main loop\r\n");
    print_rtc_date_and_time();
  }
}