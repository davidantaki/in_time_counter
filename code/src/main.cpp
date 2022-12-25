#include <Arduino.h>
#include <DS3231.h>
#include <SparkFun_Alphanumeric_Display.h>
#include <IWatchdog.h>

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

TwoWire rtc_i2c_bus(PB7, PB6); // SDA, SCL
TwoWire display_i2c_bus(PB4, PA7); // SDA, SCL
HT16K33 display;
HardwareSerial serialUSB(PA3, PA2); // rx, tx
DS3231 rtc(rtc_i2c_bus);

static const uint32_t DISPLAY_UPDATE_TIME = 500;

// Last RTC read
DateTime last_rtc_read;
uint32_t last_rtc_read_since_program_start_ms = 0;

// Birth Day
static const uint32_t birth_yr = 2000;
static const uint32_t birth_mo = 3;
static const uint32_t birth_day = 27;
// Age you will die
static const uint32_t useful_lifetime_age = 70;

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void _Error_Handler(const char *msg, int val) {
  /* User can add his own implementation to report the HAL error return state */
  serialUSB.printf("Error: %s (%i)\n\r", msg, val);
  display.printf("ERROR");
  while (1) {
  }
}

bool get_RTC_datetime() {
  bool success = true;

  // Get current date time
  bool last_rtc_century, last_rtc_h12, last_rtc_pm_time;
  uint32_t last_rtc_yr = 1970 + rtc.getYear();
  uint32_t last_rtc_mo = rtc.getMonth(last_rtc_century);
  uint32_t last_rtc_day = rtc.getDate();
  uint32_t last_rtc_hr = rtc.getHour(last_rtc_h12, last_rtc_pm_time);
  if(last_rtc_h12 && last_rtc_pm_time) {
    last_rtc_hr += 12;
  }
  uint32_t last_rtc_min = rtc.getMinute();
  uint32_t last_rtc_sec = rtc.getSecond();

  // Store the last read time since program start.
  last_rtc_read_since_program_start_ms = millis();

  // Store the last RTC read in a DateTime object.
  last_rtc_read = DateTime(last_rtc_yr, last_rtc_mo, last_rtc_day, last_rtc_hr,
                          last_rtc_min, last_rtc_sec);

  serialUSB.printf("DateTime: %u\n\r", last_rtc_read.unixtime());
  serialUSB.printf(
      "get_RTC_datetime(): last_rtc_yr: %u\tlast_rtc_mo: %u\tlast_rtc_day: last_rtc_hr: %u\t"
      "%u\tlast_rtc_min: %u\tlast_rtc_sec: %u\n\r",
      last_rtc_yr, last_rtc_mo, last_rtc_day, last_rtc_hr, last_rtc_min,
      last_rtc_sec);

  return success;
}

/*
3 displays with all segments on draws 0.572A.
Don't think the 4th turned on because not enough power.
*/
void current_draw_test() { display.write("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"); }

// Given a date and seconds, return the new date.
void add_sec_to_date(uint32_t yr, uint32_t mo, uint32_t day, uint32_t hr,
                     uint32_t min, uint32_t sec, uint32_t add_sec,
                     uint32_t *ret_yr, uint32_t *ret_mo, uint32_t *ret_day,
                     uint32_t *ret_hr, uint32_t *ret_min, uint32_t *ret_sec) {
  uint32_t sec_temp = sec + add_sec;
  uint32_t min_temp = min;
  uint32_t hr_temp = hr;
  uint32_t day_temp = day;
  uint32_t mo_temp = mo;
  uint32_t yr_temp = yr;
  while (sec_temp >= 60) {
    sec_temp -= 60;
    min_temp += 1;
    if (min_temp == 60) {
      min_temp = 0;
      hr_temp += 1;
      if (hr_temp == 24) {
        hr_temp = 0;
        day_temp += 1;
        // 31 days
        if (mo == 1 || mo == 3 || mo == 5 || mo == 7 || mo == 8 || mo == 10 ||
            mo == 12) {
          if (day_temp == 32) {
            day_temp = 0;
            mo_temp += 1;
          }
          // 30 Days
        } else if (mo == 4 || mo == 6 || mo == 9 || mo == 11) {
          if (day_temp == 31) {
            day_temp = 0;
            mo_temp += 1;
          }
          // 28/29 days
        } else if (mo == 2) {
          // Check leap year
          if (yr % 4 == 0) {
            if (day_temp == 30) {
              day_temp = 0;
              mo_temp += 1;
            }
          } else {
            if (day_temp == 29) {
              day_temp = 0;
              mo_temp += 1;
            }
          }
        }

        // Check month overflow
        if (mo_temp == 13) {
          mo_temp = 1;
          yr_temp += 1;
        }
      }
    }
  }

  *ret_yr = yr_temp;
  *ret_mo = mo_temp;
  *ret_day = day_temp;
  *ret_hr = hr_temp;
  *ret_min = min_temp;
  *ret_sec = sec_temp;
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

uint32_t get_num_days_in_mo(uint32_t mo, uint32_t yr) {
  if (mo == 1) {
    return 31;
  } else if (mo == 2) {
    // Check if current year is a leap year
    if (yr % 4 == 0) {
      return 29;
    } else {
      return 28;
    }
  } else if (mo == 3) {
    return 31;
  } else if (mo == 4) {
    return 30;
  } else if (mo == 5) {
    return 31;
  } else if (mo == 6) {
    return 30;
  } else if (mo == 7) {
    return 31;
  } else if (mo == 8) {
    return 31;
  } else if (mo == 9) {
    return 30;
  } else if (mo == 10) {
    return 31;
  } else if (mo == 11) {
    return 30;
  } else if (mo == 12) {
    return 31;
  }else{
    Error_Handler();
  }
}

uint32_t secondsToMonthsIncludingLeapYear(bool leapYear, uint32_t seconds) {
  uint32_t months = 0;
  uint32_t days = seconds / 86400;
  uint32_t years = days / 365;
  months += years * 12;
  days -= years * 365;
  if (leapYear) {
    if (days > 60) {
      days -= 1;
    }
  }
  months += days / 30;
  return months;
}

bool isLeapYear(uint32_t year) {
  if (year % 4 == 0) {
    return true;
  } else {
    return false;
  }
}

uint32_t mo_left_in_year_to_sec2(uint32_t mo_left, uint32_t yr) {
  uint32_t sec = 0;
  for (uint32_t i = 1; i <= mo_left; i++) {
    sec += get_num_days_in_mo(i, yr) * 24 * 60 * 60;
  }
  return sec;
}

uint32_t mo_left_in_year_to_sec(uint32_t mo_left, uint32_t yr) {
  uint32_t days_left = 0;
  for (int i = mo_left; i > 0; i--) {
    if (mo_left == 1) {
      days_left += 31;
    } else if (mo_left == 2) {
      // Check if current year is a leap year
      if (yr % 4 == 0) {
        days_left += 29;
      } else {
        days_left += 28;
      }
    } else if (mo_left == 3) {
      days_left += 31;
    } else if (mo_left == 4) {
      days_left += 30;
    } else if (mo_left == 5) {
      days_left += 31;
    } else if (mo_left == 6) {
      days_left += 30;
    } else if (mo_left == 7) {
      days_left += 31;
    } else if (mo_left == 8) {
      days_left += 31;
    } else if (mo_left == 9) {
      days_left += 30;
    } else if (mo_left == 10) {
      days_left += 31;
    } else if (mo_left == 11) {
      days_left += 30;
    } else if (mo_left == 12) {
      days_left += 31;
    }
  }

  return days_left * 24 * 60 * 60;
}

uint32_t get_num_leap_years_btw_dates(uint32_t start_yr, uint32_t start_mo,
                                      uint32_t start_day, uint32_t end_yr,
                                      uint32_t end_mo, uint32_t end_day) {
  uint32_t temp_start_yr = start_yr;
  uint32_t temp_end_yr = end_yr;
  // If start month is after February (the month that a leap day gets added to),
  // then don't count the start year as a leap year.
  if (start_mo > 2) {
    temp_start_yr++;
  }
  // Also, if end month is before February, don't count that year.
  if (end_mo < 2) {
    temp_end_yr--;
  }
  uint32_t num_leap_yrs = 0;
  for (int i = temp_start_yr; i <= temp_end_yr; i++) {
    if (i % 4 == 0) {
      num_leap_yrs++;
    }
  }

  return num_leap_yrs;
}

/**
 * @brief Counter using a real time clock.
 * Compare to https://www.tickcounter.com/
 */
bool rtc_counter() {
  bool success = true;

  static uint32_t last_serialUSB_print_time;
  static uint32_t last_display_update_time_ms;

  // Calculate expected death date
  static const DateTime death_date(birth_yr + useful_lifetime_age, birth_mo, birth_day, 12,
                                   0, 0);

  /*
  Calc time remaining in life in seconds
  Do this by:
  Get the last RTC read since epoch in sec.
  Add the number of seconds since the last RTC read to the last RTC read
  to get the current time since epoch in sec.
  -----
  Get death time since epoch in sec.
  -----
  Do death_time_since_epoch - current_time_since_epoch.
  Put into our desired display format of yrs, weks, days, hrs, mins, secs,
  ms yy,ww,d,hh,mm,ss,ms
  */
  const uint32_t last_RTC_read_since_epoch_sec = last_rtc_read.unixtime();
  const uint32_t curr_time_since_epoch_sec =
      last_RTC_read_since_epoch_sec + ((millis() - last_rtc_read_since_program_start_ms) / 1000);
  const DateTime curr_datetime = DateTime(curr_time_since_epoch_sec);
  const uint32_t death_time_since_epoch_sec = death_date.unixtime();
  const uint32_t time_remaining_sec = death_time_since_epoch_sec - curr_time_since_epoch_sec;

  // Put time into new format
  uint32_t time_remaining_sec_temp = time_remaining_sec;

  // Get years left
  // Divide by number of seconds in a year, etc.
  uint32_t yrs_left = time_remaining_sec_temp / (365 * 24 * 60 * 60);
  uint32_t leap_yrs_left = get_num_leap_years_btw_dates(
      curr_datetime.year(), curr_datetime.month(), curr_datetime.day(), death_date.year(), death_date.month(), death_date.day());
  time_remaining_sec_temp -= yrs_left * 365 * 24 * 60 * 60;
  // ((yrs_left - leap_yrs_left) * 365 * 24 * 60 * 60) +
                            //  (leap_yrs_left * 366 * 24 * 60 * 60);

  uint32_t mo_left = secondsToMonthsIncludingLeapYear(isLeapYear(curr_datetime.year()), time_remaining_sec_temp);
  // time_remaining_sec_temp / (30 * 24 * 60 * 60);
  time_remaining_sec_temp -= mo_left_in_year_to_sec2(mo_left, curr_datetime.year());
  
  uint32_t days_left = time_remaining_sec_temp / (24 * 60 * 60);
  time_remaining_sec_temp -= days_left * 24 * 60 * 60;
  // ASSERT(days_left<7)
  if (!(days_left < 31)) {
    serialUSB.printf("Days Left: %d\n\r", days_left);
    Error_Handler();
  }
  uint32_t hrs_left = time_remaining_sec_temp / (60 * 60);
  time_remaining_sec_temp -= hrs_left * 60 * 60;
  // ASSERT(hrs_left<24)
  if (!(hrs_left < 24)) {
    serialUSB.printf("hrs_left: %d\n\r", hrs_left);
    Error_Handler();
  }
  uint32_t mins_left = time_remaining_sec_temp / 60;
  time_remaining_sec_temp -= mins_left * 60;
  // ASSERT(mins_left<60)
  if (!(mins_left < 60)) {
    serialUSB.printf("mins_left: %d\n\r", mins_left);
    Error_Handler();
  }
  uint32_t sec_left = time_remaining_sec_temp;
  // ASSERT(sec_left<60)
  if (!(sec_left < 60)) {
    serialUSB.printf("sec_left: %d\n\r", sec_left);
    Error_Handler();
  }

  // Format output string
  // Format: yy,ww,d,hh,mm,ss,ms
  /*
    Useful to get a single digit from a number:
    Let N be the input number.
    If N is a one-digit number, return it.
    Set N = N / 10. This step removes the last digit of N.
    N % 10 gives us the last digit of N. Since we have already removed the
    last digit of N in the previous step, N % 10 is equal to the second last
    digit of the input number. Return N % 10.
  */
  /*
    To get ASCII number of a digit just add the digit to '0' char.
  */
  char output_str[16];  // Output display is 16 characters long.
  uint32_t output_str_i = 0;
  output_str[output_str_i++] = 0xFF; // Display all segments
  output_str[output_str_i++] = 0xFF; // Display all segments
  output_str[output_str_i++] = 0xFF; // Display all segments
  output_str[output_str_i++] = 0xFF; // Display all segments

  // Format years
  if (yrs_left < 10) {
    output_str[output_str_i++] = '0' + 0;
    output_str[output_str_i++] = '0' + yrs_left;
  } else {
    output_str[output_str_i++] = '0' + yrs_left / 10;
    output_str[output_str_i++] = '0' + yrs_left % 10;
  }

  // Format months
  if (mo_left < 10) {
    output_str[output_str_i++] = '0' + 0;
    output_str[output_str_i++] = '0' + mo_left;
  } else {
    output_str[output_str_i++] = '0' + mo_left / 10;
    output_str[output_str_i++] = '0' + mo_left % 10;
  }

  // Format days
  if (days_left < 10) {
    output_str[output_str_i++] = '0' + 0;
    output_str[output_str_i++] = '0' + days_left;
  } else {
    output_str[output_str_i++] = '0' + days_left / 10;
    output_str[output_str_i++] = '0' + days_left % 10;
  }

  // Format hours
  if (hrs_left < 10) {
    output_str[output_str_i++] = '0' + 0;
    output_str[output_str_i++] = '0' + hrs_left;
  } else {
    output_str[output_str_i++] = '0' + hrs_left / 10;
    output_str[output_str_i++] = '0' + hrs_left % 10;
  }
  // Format minutes
  if (mins_left < 10) {
    output_str[output_str_i++] = '0' + 0;
    output_str[output_str_i++] = '0' + mins_left;
  } else {
    output_str[output_str_i++] = '0' + mins_left / 10;
    output_str[output_str_i++] = '0' + mins_left % 10;
  }
  // Format seconds
  if (sec_left < 10) {
    output_str[output_str_i++] = '0' + 0;
    output_str[output_str_i++] = '0' + sec_left;
  } else {
    output_str[output_str_i++] = '0' + sec_left / 10;
    output_str[output_str_i++] = '0' + sec_left % 10;
  }

  // Update display
  int ret = 0;
  if (millis() - last_display_update_time_ms > DISPLAY_UPDATE_TIME) {
    last_display_update_time_ms = millis();
    ret = display.printf(output_str);  // Not sure why it returns 22 when output_str is 16 characters long.
    serialUSB.printf("display.printf ret: %d\r\n", ret);
    // success = success && (ret == 22);
  }

  // Print for debugging purposes
  if (millis() - last_serialUSB_print_time > 1000) {

    last_serialUSB_print_time = millis();
    // Display time remaining in your life
    serialUSB.printf("death_time_since_epoch_sec: %u\r\n",
                     death_time_since_epoch_sec);
    serialUSB.printf("curr_time_since_epoch_sec: %u\r\n",
                     curr_time_since_epoch_sec);
    // For comparing to timeanddate.com countdown timer to check that I did
    // this correctly.
    serialUSB.printf("total_wks_left: %u\r\n",
                     time_remaining_sec / (7 * 24 * 60 * 60));
    serialUSB.printf("total_days_left: %u\r\n",
                     time_remaining_sec / (24 * 60 * 60));
    serialUSB.printf("total_min_left: %u\r\n", time_remaining_sec / 60);
    serialUSB.printf("total_sec_left: %u\r\n", time_remaining_sec);

    serialUSB.printf(
        "millis(): %u yrs_left: %u mo_left: %u days_left: %u hrs_left: "
        "%u min_left: %u sec_left: %u\r\n",
        millis(), yrs_left, mo_left, days_left, hrs_left, mins_left, sec_left);

    serialUSB.printf(
        "millis(): %u yrs_left: %u mo_left: %u days_left: %u hrs_left: "
        "%u min_left: %u sec_left: %u ms_left_div_10: %u\r\n",
        millis(), yrs_left, mo_left, days_left, hrs_left, mins_left,
        sec_left);

    // serialUSB.printf("Current DateTime (Internal Clock): %u:%u:%u %u:%u:%u\r\n",
    //                  curr_yr, curr_mo, curr_day, curr_hr, curr_min, curr_sec);

    serialUSB.printf("Output String: %s\r\n", output_str);
  }

  // Check RTC datetime.
  // If the year is off, then we might have lost I2C connect to RTC.
  // Try to recover.
  if (rtc.getYear() + 1970 < 2021) {
    serialUSB.printf("ERROR: RTC Year is off\r\n");
    NVIC_SystemReset();
  }

  return success;
}

bool counter_mode() {
  return rtc_counter();
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

bool init_display() {
  bool success = true;
  serialUSB.printf("Initializing display...\r\n");
  success = success && display.begin(0x70, 0x71, 0x72, 0x73, display_i2c_bus);

  if(success) {
    serialUSB.printf("Display connected.\r\n");
    success = success && display.setBrightness(1);
    success = success && display.clear();
    success = success && display.displayOn();
  }

  if(success) {
    serialUSB.printf("Display initialization SUCCEEDED.\r\n");
  } else {
    serialUSB.printf("Display initialization FAILED.\r\n");
  }
  return success;
}

bool init_i2c_buses() {
  bool success = true;

  rtc_i2c_bus.begin();
  rtc_i2c_bus.setClock(100000);
  rtc_i2c_bus.setTimeout(100);

  display_i2c_bus.begin();
  display_i2c_bus.setClock(100000);
  display_i2c_bus.setTimeout(100);

  return success;
}

bool init_watchdog_timer() {
  bool success = true;
  IWatchdog.begin(4000000);
  return success;
}

bool init_console() {
  bool success = true;
  serialUSB.begin(115200);
  return success;
}

bool init_rtc() {
  bool success = true;
  delay(1000);
  

  bool oscCheck = rtc.oscillatorCheck();
  serialUSB.printf("oscillatorCheck(): %d\r\n", oscCheck);
  
  rtc.enable32kHz(false);
  if (!oscCheck) {
    serialUSB.printf("Oscillator is not enabled. Enabling...\r\n");
    // Turn on using battery
    rtc.enableOscillator(true, true, 3);
    delay(1000);
  } else {
    serialUSB.printf("Oscillator is enabled.\r\n");
  }
  oscCheck = rtc.oscillatorCheck();
  serialUSB.printf("oscillatorCheck(): %d\r\n", oscCheck);

  success = success && oscCheck;

  return success;
}

void setup() {
  bool success = true;
  success = success && init_console();
  success = success && init_i2c_buses();
  success = success && init_display();
  success = success && init_rtc();
  success = success && init_watchdog_timer();
  success = success && get_RTC_datetime();

  // Use the below to set the current date time if it has not be set yet.
  // set_datetime(2022, 12, 25, 14, 39, 0);

  if(!success) {
    serialUSB.printf("ERROR: Initialization failed. Resetting...\r\n");
    delay(1000);
    NVIC_SystemReset();
  }
}

void displayFakeColon(uint8_t digit) {
  display.illuminateChar(0b0001001000000000, digit);
  display.updateDisplay();
}

void loop() {
  bool success = true;
  success = success && counter_mode();
  IWatchdog.reload();
  
  // Print stuff
  // static int last_print_time;
  // if (millis() - last_print_time > 1000) {
  //   last_print_time = millis();
  //   serialUSB.printf("main loop\r\n");
  //   print_rtc_date_and_time();
  //   serialUSB.printf(
  //       "Last RTC Read %d/%d/%d %d:%d:%d 12hr: "
  //       "0x%X pm_time: 0x%X\r\n-------------------------------------\r\n",
  //       last_rtc_yr, last_rtc_mo, last_rtc_day, last_rtc_hr, last_rtc_min,
  //       last_rtc_sec, last_rtc_h12, last_rtc_pm_time);
  // }

  if(!success) {
    serialUSB.printf("ERROR: Something went wrong. Resetting...\r\n");
    delay(1000);
    NVIC_SystemReset();
  }
}