#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
//#include <EEPROM.h>

//pin numbering may change throughout the project
const byte total_modes = 6 , continuityPin = 5 , freqPin = 12 , buzzerPin = 4 , INA_ADDR = 0x40 ,
           buttonPin = 3 , LCD_I2C_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , bounceDelay = 170 ;
byte mode_select = 0 , printing_poll = 0 , samples , idx ;
volatile bool changed = false ;
const unsigned int buzzerTone = 2000 ;
unsigned long highPeriod , lowPeriod ;
float upper_correction_limit = 30000.0 , freq_switch_limit = 100.0 , frequency , readout , volt_conv_fact = 20 / 1023 ,
      freq_calib_factor = 1.0555 , current_calib_factor = 0.86 , millisecs = 1000000.0 , current , amp_conv_fact = 500 / 1023 ; //in [V] and [mA]

LiquidCrystal_I2C LCD ( LCD_I2C_ADDR , LCD_ROWS , LCD_COLS ) ;      //SDA - Pin A4
Adafruit_INA219 INA219 ( INA_ADDR ) ;                               //SCL - Pin A5

void buttonPressed ( void ) {
  changed = true ; }

void currentTest ( void ) {     //min 3 mA
  current = 0 ;                 //max. 2A
  samples = 255 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    current += INA219.getCurrent_mA ( ) ; }
  readout = ( current / ( float ) samples ) * current_calib_factor ;
  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( String ( readout ) + " mA         " ) ; }

void frequencyTest ( void ) {     //3Hz -> Lower-Bound Frequency
  printing_poll += 1 ;            //~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  frequency = 0 ;
  samples = 5 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH ) ;
    lowPeriod = pulseIn ( freqPin , LOW ) ;
    frequency += millisecs / ( ( ( float ) highPeriod + ( float ) lowPeriod ) ) ; }
  readout = frequency / ( float ) samples ;
  if ( readout >= upper_correction_limit ) readout *= freq_calib_factor ;
  if ( ( printing_poll == 20 && readout > freq_switch_limit ) || readout <= freq_switch_limit ) {
    printing_poll = 0 ;
    LCD.setCursor ( 0 , 2 ) ; 
    LCD.print ( String ( readout ) + " Hz              " ) ; } }

void continuityTest ( void ) {      //14.2kΩ cutoff resistance

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
      default : break ; } }