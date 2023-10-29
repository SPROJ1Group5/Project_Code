#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//#include <EEPROM.h>

//pin numbering may change throughout the project
const byte total_modes = 5 , continuityPin = 5 , ledPin = 13 , freqPin = 8 , buzzerPin = 4 ,
           buttonPin = 3 , LCD_I2C_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , bounceDelay = 150 ;
byte mode_select = 0 ;
volatile bool changed = false ;
const unsigned int buzzerTone = 2000 ;
unsigned long timeout = 500000 , highPeriod , lowPeriod ; //for the frequency measurement
float frequency , voltage_conversion_factor = 20 / 1023 , amperage_conversion_factor = 500 / 1023 ; //in [Hz], [V] and [mA]

LiquidCrystal_I2C LCD ( LCD_I2C_ADDR , LCD_ROWS , LCD_COLS ) ; //SDA - Pin A4 | SCL - Pin A5

void buttonPressed ( void ) {
  changed = true ; }

void frequencyTest ( void ) {
  highPeriod = pulseIn ( freqPin , HIGH , timeout ) ;
  lowPeriod = pulseIn ( freqPin , LOW , timeout ) ;
  frequency = 1000000.0 / ( highPeriod + lowPeriod ) ;         //in Hz
  LCD.setCursor ( 0 , 2 ) ; 
  LCD.print ( String ( ( int ) frequency ) + " Hz                " ) ; }

void continuityTest ( void ) {

    if ( digitalRead ( continuityPin ) ) {
      digitalWrite ( ledPin , HIGH ) ;
      tone ( buzzerPin , buzzerTone ) ; 
      LCD.setCursor( 0 , 2 ) ;
      LCD.print ( "     CONNECTED      " ) ; }

    else {
      digitalWrite ( ledPin , LOW ) ; 
      noTone ( buzzerPin ) ;
      LCD.setCursor( 0 , 2 ) ;
      LCD.print ( "                    " ) ; } }

void setup ( ) {
    pinMode ( buttonPin , INPUT_PULLUP ) ;
    attachInterrupt ( digitalPinToInterrupt ( buttonPin ) , buttonPressed , FALLING ) ;
    pinMode ( continuityPin , INPUT ) ;
    pinMode ( freqPin , INPUT ) ;
    pinMode ( buzzerPin , OUTPUT ) ;
    LCD.init ( ) ;
    LCD.backlight ( ) ;
    LCD.clear ( ) ; }

void loop ( ) {

    if ( changed ) {
      delay ( bounceDelay ) ;
      changed = false ;
      digitalWrite ( ledPin , LOW ) ;
      LCD.clear ( ) ;
      noTone ( buzzerPin ) ;
      mode_select++ ;
      if ( mode_select == total_modes ) mode_select = 0 ; }

    switch ( mode_select ) {
      case 0 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Voltmeter     " ) ; break ;
      case 1 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "     Ampermeter     " ) ; break ;
      case 2 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Ohm-meter     " ) ; break ;
      case 3 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "   Frequency-meter  " ) ; frequencyTest ( ) ;  break ;
      case 4 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Continuity test   " ) ; continuityTest ( ) ; break ;
      default : break ; }

  }