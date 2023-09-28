#include <LiquidCrystal_I2C.h>
//#include <EEPROM.h>

//pin numbering may change throughout the project
const byte total_modes = 5 , continuityPin = 5 , freqPin = 8 , buzzerPin = 4 , buttonPin = 13 , LCD_I2C_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , bounce_off_delay = 130 ;
byte mode_select = 0 , previous_mode = 0 ;
const unsigned int buzzerTone = 2000 ;
bool LCD_cleared = false ;
unsigned long highPeriod , lowPeriod ; //for the frequency measurement
float frequency_value , voltage_conversion_factor = 20 / 1023 , amperage_conversion_factor = 500 / 1023 ; //in [Hz], [V] and [mA]
String to_show ;

LiquidCrystal_I2C LCD ( LCD_I2C_ADDR , LCD_ROWS , LCD_COLS ) ; //SDA - Pin A4 | SCL - Pin A5

void checkButtonState ( void ) {
    //this function polls the button and increments the mode_select variable based on the state
    if ( !digitalRead ( buttonPin ) ) {
      mode_select++ ;
      if ( mode_select > total_modes ) {
        mode_select = 1 ; }
      delay ( bounce_off_delay ) ; } }

void continuityTest ( void ) {
    //there is some minor bug regarding switching mode when buzzer is on, checking the state with a boolean variable solves it, but interferes with the sound of the buzzer
    if ( !digitalRead ( continuityPin ) ) tone ( buzzerPin , buzzerTone ) ; 
    else noTone ( buzzerPin ) ; }

void setup ( ) {
    pinMode ( continuityPin , INPUT_PULLUP ) ;
    //pinMode ( freqPin , INPUT ) ;
    pinMode ( buttonPin , INPUT_PULLUP ) ; //built-in pull-up resistor
    pinMode ( buzzerPin , OUTPUT ) ;
    LCD.init ( ) ;
    LCD.backlight ( ) ;
    LCD.clear ( ) ;
    LCD.setCursor ( 0 , 0 ) ;    
    LCD.print ( "SPROJ1G5  MULTIMETER" ) ;
    LCD.setCursor ( 0 , 1 ) ;    
    LCD.print ( "      MAIN MENU      " ) ;
    LCD.setCursor ( 0 , 2 ) ;    
    LCD.print ( "Cycle through modes " ) ;
    LCD.setCursor ( 0 , 3 ) ;    
    LCD.print ( "  with the button.  " ) ; }

void loop ( ) {
    checkButtonState ( ) ;

    if ( mode_select != 0 && mode_select != previous_mode ) {
      previous_mode = mode_select ;
      if ( !LCD_cleared ) {
        LCD_cleared = true ;
        LCD.clear ( ) ; }
      switch ( mode_select ) {
        case 1 : to_show = "      Voltmeter     " ; break ;
        case 2 : to_show = "     Ampermeter     " ; break ;
        case 3 : to_show = "      Ohm-meter     " ; break ;
        case 4 : to_show = "   Frequency-meter  " ; break ;
        case 5 : to_show = "  Continuity test   " ; break ; }
      LCD.setCursor ( 0 , 0 ) ;
      LCD.print ( to_show ) ; }

    switch ( mode_select ) {
        case 5: continuityTest ( ) ; break ;


    }


    //Have to try out this one
    /*highPeriod = pulseIn ( freqPin , HIGH ) ; 
    lowPeriod = pulseIn ( freqPin , LOW ) ;
    frequency = 1 / ( ( highTime + lowTime ) * 1000000 ) ; //given in Microseconds, so it needs to be converted to seconds*/
    }


