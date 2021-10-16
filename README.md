# XIAO-Exp.-board-Health-Check

There is sparse information about setting-up the Seeedstudio expansion board dedicated to XIAO.
This simple program exercices most of the interfaces of this board:

   - At stat-up via the ON/OFF switch when powered-on from battery, the system reads the SD card to get the last date/time it has been switched-on.
   - This is displayed on the OLED as long as the user button is NOT press.
   - When button is pressed, the current start-up time is stored onto the SD, the buzzer rings and OLED display is switched-off.
   - From that moment, current date and time is displayed each time the button is pressed.
   - The buzzer rings at every O-clock.

For XIAO information, have a look at https://wiki.seeedstudio.com/Seeeduino-XIAO/
