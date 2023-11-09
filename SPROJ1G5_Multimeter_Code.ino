#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
//#include <EEPROM.h>

//pin numbering may change throughout the project
const byte total_modes = 6 , freqPin = 6 , buzzerPin = 4 , INA_ADDR = 0x40 ,
           nextButtonPin = 3 , prevButtonPin = 2 , LCD_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , bounceDelay = 180 ;
const int resistanceSelectPins [ 5 ] = { 12 , 11 , 10 , 9 , 8 } , resistanceRanges = 5 ,  resistanceAdcPin = A1 , resDelay = 2 ;
byte mode_select = 1 , printing_poll = 0 ;
volatile bool next = false , prev = false ;
const unsigned int buzzerTone = 2000 ;
int resistanceMeasuredValues [ 5 ] , val , half = 512 , idx , samples , resHalfIndex ;
float resistanceFactors [ 5 ] = { 100.0 / 1023.0 , 508.0 / 1023.0 , 10000.0 / 1023.0 , 100000.0 / 1023.0 , 1000000.0 / 1023.0 } ;
unsigned long highPeriod , lowPeriod , totalPeriod , timeOut = 100000 ;
float upper_correction_limit = 30000.0 , freq_switch_limit = 100.0 , frequency , readout , resMultiplyFactor , contCutoff = 10.0 ,
      freq_calib_factor = 1.0555 , current_calib_factor = 0.868 , millisecs = 1000000.0 , current , resDivider = 1.0 , resistance ;
String resUnit , resRange ;

LiquidCrystal_I2C LCD ( LCD_ADDR , LCD_ROWS , LCD_COLS ) ;      //SDA - Pin A4
Adafruit_INA219 INA219 ( INA_ADDR ) ;                           //SCL - Pin A5

void setResistancePin ( int onPin ) {
  for ( int g = 0 ; g < resistanceRanges ; g++ ) {
    if ( g == onPin ) { digitalWrite ( resistanceSelectPins [ g ] , LOW ) ; }
    else { digitalWrite ( resistanceSelectPins [ g ] , HIGH ) ; } } }

byte getClosestToHalf ( int * array , const int size ) {
  int minValue = abs ( array [ 0 ] - half ) ;
  int minIndex = 0 ;
  for ( int i = 0 ; i < size ; i++ ) {
    if ( abs ( array [ i ] - half ) < minValue ) {
      minValue = array [ i ] ;
      minIndex = i ; } }
  return minIndex; }

void resistanceTest ( void ) {
  resistance = 0 ;
  samples = 40 ;

  for ( int j = 0 ; j < samples ; j++ ) {
    for ( idx = 0 ; idx < resistanceRanges ; idx++ ) {
      setResistancePin ( idx ) ;
      resistanceMeasuredValues [ idx ] = analogRead ( resistanceAdcPin ) ;
      delay ( resDelay ) ; }

    resHalfIndex = getClosestToHalf ( resistanceMeasuredValues , resistanceRanges ) ;

    switch ( resHalfIndex ) {
      case 0  : resMultiplyFactor = resistanceFactors [ 0 ] * 1.33  ; resUnit = " Ohm"  ; resDivider = 1.0    ; resRange = "100R"  ; break ;
      case 1  : resMultiplyFactor = resistanceFactors [ 1 ] * 1.85  ; resUnit = " Ohm"  ; resDivider = 1.0    ; resRange = "1000R" ; break ;
      case 2  : resMultiplyFactor = resistanceFactors [ 2 ] * 1.18  ; resUnit = " kOhm" ; resDivider = 1000.0 ; resRange = "10k"   ; break ;
      case 3  : resMultiplyFactor = resistanceFactors [ 3 ] * 1.157 ; resUnit = " kOhm" ; resDivider = 1000.0 ; resRange = "100k"  ; break ;
      case 4  : resMultiplyFactor = resistanceFactors [ 4 ] * 0.935 ; resUnit = " kOhm" ; resDivider = 1000.0 ; resRange = "1M"    ; break ;
      default : break ; }

    resistance += ( ( float ) resistanceMeasuredValues [ resHalfIndex ] * resMultiplyFactor ) / resDivider ; }

  readout = resistance / (float) samples ;

  if ( readout > 600.0 && resHalfIndex == 4 ) {
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "                    " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "        OL         " ) ; }

  else {
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "Range: " + resRange ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( String ( readout ) + resUnit + "         " ) ; } }

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

void frequencyTest ( void ) {     //21Hz -> Lower-Bound Frequency
  printing_poll += 1 ;            //~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  frequency = 0 ;
  samples = 10 ;
  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH , timeOut ) ;
    lowPeriod = pulseIn ( freqPin , LOW , timeOut ) ;
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
  val = analogRead ( resistanceAdcPin ) * resistanceFactors [ 0 ] * 1.33 ;

  if ( (float) val <= contCutoff ) {
    tone ( buzzerPin , buzzerTone ) ;
    LCD.setCursor( 0 , 1 ) ;
    LCD.print ( "      CONNECTED     " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( String ( val ) + " Ohm        " ) ; }
  
  else {
      noTone ( buzzerPin ) ;
      LCD.setCursor( 0 , 1 ) ;
      LCD.println ( "                    " ) ;
      LCD.setCursor( 0 , 2 ) ;
      LCD.print ( "         OL        " ) ;
      LCD.setCursor( 0 , 3 ) ;
      LCD.print ( "          " ) ; } }

void setup ( ) {
    for ( int h = 0 ; h < resistanceRanges ; h++ ) {
      pinMode ( resistanceSelectPins [ h ] , OUTPUT ) ;
      digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

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

      for ( int h = 0 ; h < resistanceRanges ; h++ ) {
        digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

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