#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
//#include <EEPROM.h>

//pin numbering may change throughout the project
const byte total_modes = 6 , freqPin = 6 , buzzerPin = 4 , INA_ADDR = 0x40 , resistanceAdcPin = A0 ,
           nextButtonPin = 3 , prevButtonPin = 2 , LCD_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , bounceDelay = 160 , resistanceRanges = 5 ;
const byte resistanceSelectPins [ 5 ] = { 12 , 11 , 10 , 9 , 8 } ;
byte mode_select = 1 , printing_poll = 0 , idx , samples , resHalfIndex ;
volatile bool next = false , prev = false ;
const unsigned int buzzerTone = 2000 ;
int resistanceMeasuredValues [ 5 ] , val , half = 512 ;
float resistanceFactors [ 5 ] = { 100 / 1023 , 1000 / 1023 , 10000 / 1023 , 100000 / 1023 , 1000000 / 1023 } ;
unsigned long highPeriod , lowPeriod , totalPeriod ;
float upper_correction_limit = 30000.0 , freq_switch_limit = 100.0 , frequency , readout , resMultiplyFactor , contCutoff = 10.0 ,
      freq_calib_factor = 1.0555 , current_calib_factor = 0.86 , millisecs = 1000000.0 , current , resDivider = 1.0 ;
String resUnit ;

LiquidCrystal_I2C LCD ( LCD_ADDR , LCD_ROWS , LCD_COLS ) ;      //SDA - Pin A4
Adafruit_INA219 INA219 ( INA_ADDR ) ;                           //SCL - Pin A5

/*unsigned long freqTime [ 2 ] = { 0 , 0 } ;
bool sel = false;                        

void getHalfPeriod() {
  freqTime[sel ? 1:0] = millis();
  sel != sel;
}*/

void setResistancePin ( byte onPin ) {
  for ( byte g = 0 ; g < 5 ; g++ ) {
    if ( g == onPin ) digitalWrite ( resistanceSelectPins [ g ] , LOW ) ;
    else digitalWrite ( resistanceSelectPins [ g ] , HIGH ) ; } }

byte getClosestToHalf ( int * array , const byte size ) {
  int minValue = abs ( array [ 0 ] - half ) ;
  byte minIndex = 0 ;
  for ( byte i = 1 ; i < size ; i++ ) {
    if ( abs ( array [ i ] - half ) < minValue ) {
      minValue = array [ i ] ;
      minIndex = i ; } }
  return minIndex; }

void resistanceTest ( void ) {
  samples = 5 ;
  for ( idx = 0 ; idx < samples ; idx++ ) {
    setResistancePin ( idx ) ;
    resistanceMeasuredValues [ idx ] = analogRead ( resistanceAdcPin ) ; }

  resHalfIndex = getClosestToHalf ( resistanceMeasuredValues , resistanceRanges ) ;

  switch ( resHalfIndex ) {
    case 0  : resMultiplyFactor = resistanceFactors [ 0 ] ; resUnit = "Ω"  ; resDivider = 1.0       ; break ;
    case 1  : resMultiplyFactor = resistanceFactors [ 1 ] ; resUnit = "Ω"  ; resDivider = 1.0       ; break ;
    case 2  : resMultiplyFactor = resistanceFactors [ 2 ] ; resUnit = "kΩ" ; resDivider = 1000.0    ; break ;
    case 3  : resMultiplyFactor = resistanceFactors [ 3 ] ; resUnit = "kΩ" ; resDivider = 1000.0    ; break ;
    case 4  : resMultiplyFactor = resistanceFactors [ 4 ] ; resUnit = "MΩ" ; resDivider = 1000000.0 ; break ;
    default : break ; }

  readout = ( ( float ) resistanceMeasuredValues [ resHalfIndex ] * resMultiplyFactor ) / resDivider ;
  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( String ( readout ) + resUnit + "        " ) ; }

void nextButtonPressed ( void ) {
  next = true ; }

void prevButtonPressed ( void ) {
  prev = true ; }

void currentTest ( void ) {     //min 3 mA
  current = 0 ;                 //max. 2A
  samples = 255 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    current += INA219.getCurrent_mA ( ) ; }
  readout = ( current / (float) samples ) * current_calib_factor ;
  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( String ( readout ) + " mA         " ) ; }

void frequencyTest ( void ) {     //3Hz -> Lower-Bound Frequency
  printing_poll += 1 ;            //~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  frequency = 0 ;
  samples = 8 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH ) ;
    lowPeriod = pulseIn ( freqPin , LOW ) ;
    totalPeriod = highPeriod + lowPeriod ;
    frequency += millisecs / (float) totalPeriod ; }
  readout = frequency / (float) samples ;
  if ( readout >= upper_correction_limit ) readout *= freq_calib_factor ;
  if ( ( printing_poll == 20 && readout > freq_switch_limit ) || readout <= freq_switch_limit ) {
    printing_poll = 0 ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( String ( readout ) + " Hz              " ) ; } }

void continuityTest ( void ) {

  setResistancePin ( 0 ) ;
  val = analogRead ( resistanceAdcPin ) * resistanceFactors [ 0 ] ;

  if ( (float) val <= contCutoff && (float) val != 0.0 ) {
    tone ( buzzerPin , buzzerTone ) ;
    LCD.setCursor( 0 , 1 ) ;
    LCD.print ( "      CONNECTED     " ) ;
    LCD.setCursor( 0 , 2 ) ;
    LCD.print ( String ( val ) + "Ω      " ) ; }
  
  else {
      noTone ( buzzerPin ) ;
      LCD.setCursor( 0 , 2 ) ;
      LCD.print ( "         OL         " ) ; } }

void setup ( ) {
    pinMode ( nextButtonPin , INPUT_PULLUP ) ;
    pinMode ( prevButtonPin , INPUT_PULLUP ) ;
    attachInterrupt ( digitalPinToInterrupt ( nextButtonPin ) , nextButtonPressed , FALLING ) ;
    attachInterrupt ( digitalPinToInterrupt ( prevButtonPin ) , prevButtonPressed , FALLING ) ;
    pinMode ( freqPin , INPUT ) ;
    pinMode ( buzzerPin , OUTPUT ) ;
    LCD.init ( ) ;
    LCD.backlight ( ) ;
    LCD.clear ( ) ;
    INA219.begin ( ) ; }

void loop ( ) {

    if ( next || prev ) {
      delay ( bounceDelay ) ;
      if ( next ) mode_select++ ;
      if ( prev ) mode_select-- ;
      next = false ;
      prev = false ;
      LCD.clear ( ) ;
      noTone ( buzzerPin ) ;
      if ( mode_select == total_modes + 1 ) mode_select = 1 ;
      if ( mode_select == 0 ) mode_select = total_modes ; }

    switch ( mode_select ) {
      case 1 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Voltmeter     " ) ; break ;
      case 2 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "     Ampermeter     " ) ; currentTest ( ) ; break ;
      case 3 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Ohm-meter     " ) ; resistanceTest ( ) ; break ;
      case 4 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "   Frequency-meter  " ) ; frequencyTest ( ) ;  break ;
      case 5 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Continuity test   " ) ; continuityTest ( ) ; break ;
      case 6 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Temperature test  " ) ; break ;
      default : break ; } }