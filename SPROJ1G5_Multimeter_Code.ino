#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
//#include <EEPROM.h>

//Defined Constants
const byte total_modes = 6 , INA_ADDR = 0x40 , LCD_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , resistanceRanges = 5 ;
const unsigned int buzzerTone = 2000 ;
const int full = 1023 , half = 512 ;
const unsigned long resDelay = 2 , bounceDelay = 170 , timeOut = 100000 ;
const float resistanceFactors [ 5 ] = { 99.8 , 1197.0 , 10000.0 , 100800.0 , 1011000.0 } ;
const float freqCorrectionLimit = 30000.0 , contCutoff = 10.0 , freqCalibFactor = 1.0555 , currentCalibFactor = 0.855 ;
const float currentCorrectionLimit = 90.0 , millisecs = 1000000.0 , resCorrect = 2.0 , currentLimit = 1.0 ;

//Button Pinouts - Pin numbering may change throughout the project
const byte freqPin = 5 , buzzerPin = 4 , nextButtonPin = 3 , prevButtonPin = 2 , voltSel = 13 , resistanceAdcPin = A0 , voltageAdcPin = A1 ;
const byte resistanceSelectPins [ resistanceRanges ] = { 12 , 11 , 10 , 9 , 8 } ;

//Variables
byte mode_select = 1 , idx , samples , g , j , h , resHalfIndex ;
volatile bool next = false , prev = false ;
int resistanceMeasuredValues [ resistanceRanges ] ;
unsigned long highPeriod , lowPeriod ;
float frequency , readout , resMultiplyFactor , resDivider , current , resistance , contVal ;
String resUnit , resRange , freqUnit , currentUnit ;

LiquidCrystal_I2C LCD ( LCD_ADDR , LCD_ROWS , LCD_COLS ) ;      //SDA - Pin A4
Adafruit_INA219 INA219 ( INA_ADDR ) ;                           //SCL - Pin A5

void setResistancePin ( byte onPin ) {
  for ( g = 0 ; g < resistanceRanges ; g++ ) {
    if ( g == onPin ) { digitalWrite ( resistanceSelectPins [ g ] , LOW ) ; }
    else { digitalWrite ( resistanceSelectPins [ g ] , HIGH ) ; } } }

int getAbsVal ( int num  ) {
  if ( num >= 0 ) return num ;
  return - num ; }

byte getClosestToFull ( int * array , const byte size ) {
  int minValue = getAbsVal ( array [ 0 ] - half ) , currentAbsVal ;
  byte minIndex = 0 ;
  for ( byte i = 1 ; i < size ; i++ ) {
    currentAbsVal = getAbsVal ( array [ i ] - half ) ;
    if ( currentAbsVal < minValue ) {
      minValue = currentAbsVal ;
      minIndex = i ; } }
  return minIndex; }

bool checkDisconnectedRes( void ) {
  byte count = 0 ;
  for ( h = 0 ; h < resistanceRanges ; h++ ) {
    if ( getAbsVal ( full - resistanceMeasuredValues [ h ] ) < 80 ) count++ ; }
  return count == resistanceRanges ; }

void resistanceTest ( void ) {

  readout = 0.0 ;
  resistance = 0.0 ;
  samples = 40 ;

  for ( j = 0 ; j < samples ; j++ ) {

    for ( idx = 0 ; idx < resistanceRanges ; idx++ ) {
      setResistancePin ( idx ) ;
      delay ( resDelay ) ;
      resistanceMeasuredValues [ idx ] = analogRead ( resistanceAdcPin ) ; }

    digitalWrite ( resistanceSelectPins [ 4 ] , HIGH ) ;
    resHalfIndex = getClosestToFull ( resistanceMeasuredValues , resistanceRanges ) ;

    switch ( resHalfIndex ) {
      case 0  : resMultiplyFactor = resistanceFactors [ 0 ] ; resUnit = " Ohm"   ; resRange = "100R" ; resDivider = 1.0 ; break ;
      case 1  : resMultiplyFactor = resistanceFactors [ 1 ] ; resUnit = " Ohm"   ; resRange = "1K"   ; resDivider = 1.0 ; break ;
      case 2  : resMultiplyFactor = resistanceFactors [ 2 ] ; resUnit = " kOhm"  ; resRange = "10K"  ; resDivider = 1000.0 ; break ;
      case 3  : resMultiplyFactor = resistanceFactors [ 3 ] ; resUnit = " kOhm"  ; resRange = "100K"  ; resDivider = 1000.0 ; break ;
      case 4  : resMultiplyFactor = resistanceFactors [ 4 ] ; resUnit = " kOhm"  ; resRange = "1M"   ; resDivider = 1000.0 ; break ;
      default : break ; }

    resistance += ( ( ( float ) resistanceMeasuredValues [ resHalfIndex ] * resMultiplyFactor ) / (float) getAbsVal ( full - resistanceMeasuredValues [ resHalfIndex ] ) ) ; }
  
  readout = resistance / ( (float) samples * resDivider ) ;

  if ( checkDisconnectedRes ( ) ) {
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "                    " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "      Overload      " ) ; }

  else {
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "Range: " + resRange ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( String ( readout ) + resUnit + "          " ) ; } }

void voltageTest ( void ) {




}

void nextButtonPressed ( void ) {
  next = true ; }

void prevButtonPressed ( void ) {
  prev = true ; }

void currentTest ( void ) {     //min 3 mA | max. 2A
  readout = 0.0 ;
  current = 0.0 ;
  samples = 250 ;

  for ( idx = 0 ; idx < samples ; idx++  ) {
    current += INA219.getCurrent_mA ( ) ; }

  readout = current / (float) samples ;

  if ( readout >= currentCorrectionLimit ) readout *= currentCalibFactor ;

  if ( ( readout / 1000.0 ) >= currentLimit ) {
    currentUnit = " A" ;
    readout /= 1000.0 ; }

  else currentUnit = " mA" ;

  LCD.setCursor ( 0 , 2 ) ;

  if ( readout > currentLimit ) LCD.print ( String ( readout ) + currentUnit + "         " ) ;

  else LCD.print ( "                    " ) ; }

void frequencyTest ( void ) {     //25Hz -> Lower-Bound Frequency
                                  //~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  frequency = 0.0 ;
  readout = 0.0 ;
  samples = 20 ;

  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH , timeOut ) ;
    lowPeriod = pulseIn ( freqPin , LOW , timeOut ) ;
    frequency += millisecs / (float) ( highPeriod + lowPeriod ) ; }

  readout = frequency / (float) samples ;

  if ( readout >= freqCorrectionLimit ) readout *= freqCalibFactor ;

  if ( ( readout / 1000.0 ) >= 1.0 ) {
    readout /= 1000.0 ;
    freqUnit = " kHz            " ; }
  
  else freqUnit = " Hz          " ;

  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( String ( readout ) + freqUnit ) ; }

void continuityTest ( void ) {

  setResistancePin ( 0 ) ;
  contVal = (float) ( analogRead ( resistanceAdcPin ) * resistanceFactors [ 0 ] ) / (float) getAbsVal ( full - resistanceMeasuredValues [ resHalfIndex ] ) ;

  if ( contVal <= contCutoff ) {
    resUnit = " Ohm           " ;
    tone ( buzzerPin , buzzerTone ) ;
    LCD.setCursor( 0 , 1 ) ;
    LCD.print ( "      CONNECTED     " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( String ( contVal ) + resUnit ) ; }
  
  else {
      noTone ( buzzerPin ) ;
      LCD.setCursor ( 0 , 1 ) ;
      LCD.print ( "                    " ) ;
      LCD.setCursor ( 0 , 2 ) ;
      LCD.print ( "     Open Loop      " ) ; } }

void setup ( ) {

    for ( h = 0 ; h < resistanceRanges ; h++ ) {
      pinMode ( resistanceSelectPins [ h ] , OUTPUT ) ;
      digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

    pinMode ( nextButtonPin , INPUT_PULLUP ) ;
    pinMode ( prevButtonPin , INPUT_PULLUP ) ;
    attachInterrupt ( digitalPinToInterrupt ( nextButtonPin ) , nextButtonPressed , FALLING ) ;
    attachInterrupt ( digitalPinToInterrupt ( prevButtonPin ) , prevButtonPressed , FALLING ) ;

    pinMode ( resistanceAdcPin , INPUT ) ;
    pinMode ( voltageAdcPin , INPUT ) ;
    pinMode ( freqPin , INPUT ) ;
    pinMode ( voltSel , INPUT ) ;
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

      for ( h = 0 ; h < resistanceRanges ; h++ ) {
        digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

      LCD.clear ( ) ;
      noTone ( buzzerPin ) ;

      if ( mode_select == total_modes + 1 ) mode_select = 1 ;
      if ( mode_select == 0 ) mode_select = total_modes ; }

    switch ( mode_select ) {
      case 1 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Voltmeter     " ) ; voltageTest ( ) ; break ;
      case 2 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "     Ampermeter     " ) ; currentTest ( ) ; break ;
      case 3 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Ohm-meter     " ) ; resistanceTest ( ) ; break ;
      case 4 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "   Frequency-meter  " ) ; frequencyTest ( ) ;  break ;
      case 5 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Continuity test   " ) ; continuityTest ( ) ; break ;
      case 6 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Temperature test  " ) ; break ;
      default : break ; } }