#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
//#include <EEPROM.h>

//pin numbering may change throughout the project
const byte total_modes = 6 , continuityPin = 5 , freqPin = 12 , buzzerPin = 4 , INA_ADDR = 0x40 ,
           buttonPin = 3 , LCD_I2C_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , bounceDelay = 150 ;
byte mode_select = 0 , printing_poll = 0 , idx ;
const byte samples = 5 ;
volatile bool changed = false ;
const unsigned int buzzerTone = 2000 ;
unsigned long highPeriod , lowPeriod ; //for the frequency measurement
float upper_correction_limit = 30000.0 , freq_switch_limit = 100.0 , frequency , readout , voltage_conversion_factor = 20 / 1023 ,
      calib_factor = 1.05 , milliseconds = 1000000.0 , current , amperage_conversion_factor = 500 / 1023 ; //in [V] and [mA]

LiquidCrystal_I2C LCD ( LCD_I2C_ADDR , LCD_ROWS , LCD_COLS ) ; //SDA - Pin A4 | SCL - Pin A5
Adafruit_INA219 INA219 ( INA_ADDR ) ;

void buttonPressed ( void ) {
  changed = true ; }

void currentTest ( void ) {
  current = 0 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    current += INA219.getCurrent_mA ( ) ; }
  readout = current / ( float ) samples ;
  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( String ( readout ) + " mA         " ) ; }

void frequencyTest ( void ) {     //3Hz -> Lower-Bound Frequency
  printing_poll += 1 ;            //~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  frequency = 0 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH ) ;
    lowPeriod = pulseIn ( freqPin , LOW ) ;
    frequency += milliseconds / ( ( ( float ) highPeriod + ( float ) lowPeriod ) ) ; }
  readout = frequency / ( float ) samples ;
  if ( readout >= upper_correction_limit ) readout *= calib_factor ;
  if ( ( printing_poll == 20 && readout > freq_switch_limit ) || readout <= freq_switch_limit ) {
    printing_poll = 0 ;
    LCD.setCursor ( 0 , 2 ) ; 
    LCD.print ( String ( readout ) + " Hz              " ) ; } }

void continuityTest ( void ) {

    if ( digitalRead ( continuityPin ) ) {
      tone ( buzzerPin , buzzerTone ) ; 
      LCD.setCursor( 0 , 2 ) ;
      LCD.print ( "     CONNECTED      " ) ; }

    else {
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
    LCD.clear ( ) ;
    INA219.begin ( ) ; }

void loop ( ) {

    if ( changed ) {
      delay ( bounceDelay ) ;
      changed = false ;
      LCD.clear ( ) ;
      noTone ( buzzerPin ) ;
      mode_select++ ;
      if ( mode_select == total_modes ) mode_select = 0 ; }

    switch ( mode_select ) {
      case 0 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Voltmeter     " ) ; break ;
      case 1 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "     Ampermeter     " ) ; currentTest ( ) ; break ;
      case 2 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Ohm-meter     " ) ; break ;
      case 3 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "   Frequency-meter  " ) ; frequencyTest ( ) ;  break ;
      case 4 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Continuity test   " ) ; continuityTest ( ) ; break ;
      case 5 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Temperature test  " ) ; break ;
      default : break ; }

  }