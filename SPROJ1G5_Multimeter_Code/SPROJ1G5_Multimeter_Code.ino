#include <LiquidCrystal_I2C.h>
#include <String.h>

//pin numbering may change throughout the project
const byte freqPin = 8 , buzzerPin = 12, buttonPin = 13 , LCD_I2C_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , delayer = 255 ;
const int buzzerTone = 3000 ; //in Hz
unsigned long highPeriod , lowPeriod ; //for the frequency measurement
float frequency , voltage_conversion_factor = 20 / 1023 , amperage_conversion_factor = 500 / 1023 ; //in [Hz], [V] and [mA]
String project_name = "SPROJ1 -- MULTIMETER" , menu = "      MAIN MENU      " ,
       message_upper = " Select measurement " , message_lower = "  with the button.  " ;

LiquidCrystal_I2C LCD ( LCD_I2C_ADDR , LCD_ROWS , LCD_COLS ) ; //SDA - Pin A4 | SCL - Pin A5

void setup ( ) {
    pinMode ( freqPin , INPUT ) ;
    pinMode ( buttonPin , INPUT_PULLUP ) ; //built-in pull-up resistor
    pinMode ( buzzerPin , OUTPUT ) ;
    LCD.init ( ) ;
    LCD.backlight ( ) ;
    LCD.clear ( ) ;
    LCD.setCursor ( 0 , 0 ) ;    
    LCD.print ( project_name ) ;
    LCD.setCursor ( 0 , 1 ) ;    
    LCD.print ( menu ) ;
    LCD.setCursor ( 0 , 2 ) ;    
    LCD.print ( message_upper ) ;
    LCD.setCursor ( 0 , 3 ) ;    
    LCD.print ( message_lower ) ; }

void loop ( ) {
    //Have to try out this one
    /*highPeriod = pulseIn ( freqPin , HIGH ) ; 
    lowPeriod = pulseIn ( freqPin , LOW ) ;
    frequency = 1 / ( ( highTime + lowTime ) * 1000000 ) ; //given in Microseconds, so it needs to be converted to seconds
    delay ( delayer ) ;*/ }