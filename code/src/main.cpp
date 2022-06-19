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

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void _Error_Handler(const char *msg, int val) {
  /* User can add his own implementation to report the HAL error return state */
  serialUSB.printf("Error: %s (%i)\n\r", msg, val);
  display.printf("Error:%s:%i", msg, val);
  while (1) {
  }
}

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
uint32_t get_sec_since_epoch(uint32_t yr, uint32_t mo, uint32_t day,
                             uint32_t hr, uint32_t min, uint32_t sec) {
  uint32_t time_since_epoch_sec = 0;
  // Add a day if a leap year (i.e. if the year is divisible by 4).
  // Does't include current year.
  for (uint32_t i = 1970; i < yr; i++) {
    if (i % 4 == 0) {
      time_since_epoch_sec += 366 * 24 * 60 * 60;
    } else {
      time_since_epoch_sec += 365 * 24 * 60 * 60;
    }
  }

  // Don't include the current month
  for (uint32_t i = 1; i < mo; i++) {
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

  if (time_since_epoch_sec < 0) {
    serialUSB.printf("time_since_epoch_sec overflowed\r\n");
    Error_Handler();
  }

  return time_since_epoch_sec;
}

/**
 * @brief Counter using a real time clock.
 */
void rtc_counter() {
  static int last_serialUSB_print_time;
  static int time_remaining_sec_change_time_ms;
  static uint32_t old_time_remaining_sec = 0;

  // Check RTC datetime.
  // If the year is off, then we might have lost I2C connect to RTC.
  // Try to recover.
  if (rtc.getYear() + 1970 < 2021) {
    NVIC_SystemReset();
  }

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

  // Calc time remaining in life in seconds
  // To this by:
  // Get current datetime, convert that to seconds since epoch.
  // Get death time since epoch in sec.
  // Do death_time_since_epoch - current_time_since_epoch.
  // Put into our desired display format of yrs, weks, days, hrs, mins, secs, ms
  // yy,ww,d,hh,mm,ss,ms
  uint32_t death_time_since_epoch_sec = get_sec_since_epoch(
      death_yr, death_mo, death_day, death_hr, death_min, death_sec);
  uint32_t curr_time_since_epoch_sec = get_sec_since_epoch(
      curr_yr, curr_mo, curr_day, curr_hr, curr_min, curr_sec);
  uint32_t time_remaining_sec =
      death_time_since_epoch_sec - curr_time_since_epoch_sec;

  // Put time into new format
  uint32_t time_remaining_sec_temp = time_remaining_sec;
  uint32_t yrs_left = time_remaining_sec_temp / (365 * 24 * 60 * 60);
  time_remaining_sec_temp -= yrs_left * 365 * 24 * 60 * 60;
  uint32_t wks_left = time_remaining_sec_temp / (7 * 24 * 60 * 60);
  time_remaining_sec_temp -= wks_left * 7 * 24 * 60 * 60;
  // ASSERT(wks_left<52)
  if (!(wks_left < 52)) {
    Error_Handler();
  }
  uint32_t days_left = time_remaining_sec_temp / (24 * 60 * 60);
  time_remaining_sec_temp -= days_left * 24 * 60 * 60;
  // ASSERT(days_left<7)
  if (!(days_left < 7)) {
    Error_Handler();
  }
  uint32_t hrs_left = time_remaining_sec_temp / (60 * 60);
  time_remaining_sec_temp -= hrs_left * 60 * 60;
  // ASSERT(hrs_left<24)
  if (!(hrs_left < 24)) {
    Error_Handler();
  }
  uint32_t mins_left = time_remaining_sec_temp / 60;
  time_remaining_sec_temp -= mins_left * 60;
  // ASSERT(mins_left<60)
  if (!(mins_left < 60)) {
    Error_Handler();
  }
  uint32_t sec_left = time_remaining_sec_temp;
  // ASSERT(sec_left<60)
  if (!(sec_left < 60)) {
    Error_Handler();
  }
  uint32_t ms_left;
  // Basically, reset ms_left when sec_left has changed.
  if (time_remaining_sec != old_time_remaining_sec) {
    old_time_remaining_sec = time_remaining_sec;
    ms_left = 1000;
    time_remaining_sec_change_time_ms = millis();
  } else {
    ms_left = 1000 - (millis() - time_remaining_sec_change_time_ms);
    // Clamp ms_left to (0,1000)
    ms_left = ms_left < 0 ? 0 : ms_left;
  }

  // Format output string
  // Format: yy,ww,d,hh,mm,ss,ms
  /*
    Useful to get a single digit from a number:
    Let N be the input number.
    If N is a one-digit number, return it.
    Set N = N / 10. This step removes the last digit of N.
    N % 10 gives us the last digit of N. Since we have already removed the last
    digit of N in the previous step, N % 10 is equal to the second last digit
    of the input number. Return N % 10.
  */
  /*
    To get ASCII number of a digit just add the digit to '0' char.
  */
  char output_str[16];  // Output display is 16 characters long.
  output_str[0] = 0xFF; // Display all segments
  output_str[1] = 0xFF; // Display all segments

  // Format years
  if (yrs_left < 10) {
    output_str[2] = '0' + 0;
    output_str[3] = '0' + yrs_left;
  } else {
    output_str[2] = '0' + yrs_left / 10;
    output_str[3] = '0' + yrs_left % 10;
  }

  // Format weeks
  if (wks_left < 10) {
    output_str[4] = '0' + 0;
    output_str[5] = '0' + wks_left;
  } else {
    output_str[4] = '0' + wks_left / 10;
    output_str[5] = '0' + wks_left % 10;
  }

  // Format days
  // Days should always be <7days. We already asserted this above.
  output_str[6] = '0' + 0;
  output_str[7] = '0' + days_left;

  // Format hours
  if (hrs_left < 10) {
    output_str[8] = '0' + 0;
    output_str[9] = '0' + hrs_left;
  } else {
    output_str[8] = '0' + hrs_left / 10;
    output_str[9] = '0' + hrs_left % 10;
  }
  // Format minutes
  if (mins_left < 10) {
    output_str[10] = '0' + 0;
    output_str[11] = '0' + mins_left;
  } else {
    output_str[10] = '0' + mins_left / 10;
    output_str[11] = '0' + mins_left % 10;
  }
  // Format seconds
  if (sec_left < 10) {
    output_str[12] = '0' + 0;
    output_str[13] = '0' + sec_left;
  } else {
    output_str[12] = '0' + sec_left / 10;
    output_str[13] = '0' + sec_left % 10;
  }
  // Format milliseconds
  int ms_left_div_10 = ms_left / 10;
  if (ms_left_div_10 < 10) {
    output_str[14] = '0' + 0;
    output_str[15] = '0' + ms_left_div_10;
  } else {
    output_str[14] = '0' + ms_left_div_10 / 10;
    output_str[15] = '0' + ms_left_div_10 % 10;
  }

  // Update display only if the remaining time changed so that we aren't
  // constantly updating the display. i.e. only update diplay when we need to.
  // if (time_remaining_sec != old_time_remaining_sec) {
  //   display.printf(output_str);
  //   old_time_remaining_sec = time_remaining_sec;
  // }
  display.printf(output_str);
  display.colonOn();

  // Print for debugging purposes
  if (millis() - last_serialUSB_print_time > 1000) {
    last_serialUSB_print_time = millis();
    // Display time remaining in your life
    serialUSB.printf("death_time_since_epoch_sec: %u\r\n",
                     death_time_since_epoch_sec);
    serialUSB.printf("curr_time_since_epoch_sec: %u\r\n",
                     curr_time_since_epoch_sec);
    // For comparing to timeanddate.com countdown timer to check that I did this
    // correctly.
    serialUSB.printf("total_wks_left: %u\r\n",
                     time_remaining_sec / (7 * 24 * 60 * 60));
    serialUSB.printf("total_days_left: %u\r\n",
                     time_remaining_sec / (24 * 60 * 60));
    serialUSB.printf("total_min_left: %u\r\n", time_remaining_sec / 60);
    serialUSB.printf("total_sec_left: %u\r\n", time_remaining_sec);

    serialUSB.printf(
        "millis(): %u yrs_left: %u wks_left: %u days_left: %u hrs_left: "
        "%u min_left: %u sec_left: %u ms_left_div_10: %u\r\n",
        millis(), yrs_left, wks_left, days_left, hrs_left, mins_left, sec_left,
        ms_left_div_10);

    serialUSB.printf("Output String: %s\r\n", output_str);
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

  serialUSB.printf("RTC: %d/%d/%d %d:%d:%d 12hr: "
                   "0x%X pm_time: 0x%X\r\n",
                   yr + 1970, mo, date, hr, min, sec, h12, pm_time);
}

void set_datetime(int year, int month, int date, int hr, int min, int sec) {
  rtc.setYear(year - 1970);
  rtc.setMonth(month);
  rtc.setDate(date);
  rtc.setHour(hr);
  rtc.setMinute(min);
  rtc.setSecond(sec);
  rtc.setClockMode(true);
}

void init_display() {

  serialUSB.printf("Running init_display()...\r\n");
  while (display.begin(0x70, 0x71, 0x72, 0x73, i2c1_bus) == false) {
    serialUSB.printf("Display did not acknowledge!\r\n");
    delay(1000);
  }
  serialUSB.printf("Display acknowledged.\r\n");

  if (!display.initialize()) {
    Error_Handler();
  }
  if (!display.clear()) {
    Error_Handler();
  }
  if (!display.setBrightness(1)) {
    Error_Handler();
  }
  if (!display.colonOn()) {
    Error_Handler();
  }
  // current_draw_test();
  display.printf("JOHN IS SO HOT");
}

void setup() {
  // put your setup code here, to run once:
  serialUSB.begin(115200);
  i2c1_bus.begin();

  init_display();
  // display.clear();
  // display.illuminateChar(1, 1);
  // // display.printChar(1, 1);
  // display.updateDisplay();
  // while (1) {
  // }
  serialUSB.printf("oscillatorCheck(): %d\r\n", rtc.oscillatorCheck());

  rtc.enable32kHz(false);
  if (!rtc.oscillatorCheck()) {
    serialUSB.printf("Oscillator is not enabled. Enabling...\r\n");
    // Turn on using battery
    rtc.enableOscillator(true, true, 3);
  } else {
    serialUSB.printf("Oscillator is enabled.\r\n");
  }
  delay(1000);
  serialUSB.printf("oscillatorCheck(): %d\r\n", rtc.oscillatorCheck());

  // set_datetime(2022, 6, 18, 19, 15, 0);
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