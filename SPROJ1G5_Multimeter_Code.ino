#include <LiquidCrystal_I2C.h>

//pin numbering may change throughout the project
const byte freqPin = 8 , buzzerPin = 12, buttonPin = 13 , LCD_I2C_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 ;
const int buzzerTone = 3000 ;
byte button_bounceoff_delay = 180 , mode_select = 0 , previous_mode = 0 , button_value ;
bool  LCD_cleared = false ;
unsigned long highPeriod , lowPeriod ; //for the frequency measurement
float frequency_value , voltage_conversion_factor = 20 / 1023 , amperage_conversion_factor = 500 / 1023 ; //in [Hz], [V] and [mA]
String to_show ;

LiquidCrystal_I2C LCD ( LCD_I2C_ADDR , LCD_ROWS , LCD_COLS ) ; //SDA - Pin A4 | SCL - Pin A5

void setup ( ) {
    pinMode ( freqPin , INPUT ) ;
    pinMode ( buttonPin , INPUT_PULLUP ) ; //built-in pull-up resistor
    pinMode ( buzzerPin , OUTPUT ) ;
    LCD.init ( ) ;
    LCD.backlight ( ) ;
    LCD.clear ( ) ;
    LCD.setCursor ( 0 , 0 ) ;    
    LCD.print ( "SPROJ1 -- MULTIMETER" ) ;
    LCD.setCursor ( 0 , 1 ) ;    
    LCD.print ( "      MAIN MENU      " ) ;
    LCD.setCursor ( 0 , 2 ) ;    
    LCD.print ( "Cycle through modes " ) ;
    LCD.setCursor ( 0 , 3 ) ;    
    LCD.print ( "  with the button.  " ) ; }

void loop ( ) {
    button_value = digitalRead ( buttonPin ) ;

    if ( button_value == LOW ) {
      mode_select++ ;
      if ( mode_select == 6 ) {
        mode_select = 1 ; }
      delay ( button_bounceoff_delay ) ; }

    if ( mode_select != 0 && mode_select != previous_mode ) {
      previous_mode = mode_select ;
      if ( LCD_cleared == false ) {
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




    //Have to try out this one
    /*highPeriod = pulseIn ( freqPin , HIGH ) ; 
    lowPeriod = pulseIn ( freqPin , LOW ) ;
    frequency = 1 / ( ( highTime + lowTime ) * 1000000 ) ; //given in Microseconds, so it needs to be converted to seconds*/
    }