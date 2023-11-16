#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Defined Constants
const byte total_modes = 6 , LCD_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , resistanceRanges = 5 ;
const unsigned int buzzerTone = 3000 ;
const int full = 1023 , half = 512 ;
const unsigned long adcDelay = 4 , bounceDelay = 170 , timeOut = 100000 ;
const float resistanceFactors [ 5 ] = { 99.8 , 1197.0 , 10000.0 , 100800.0 , 1011000.0 } ;
const float voltageFactor = 5.0 / 1023.0 ;
const float freqCorrectionLimit = 30000.0 , contCutoff = 10.0 , freqCalibFactor = 1.0555 , currentCalibFactor = 1.31 , Vref = 2495.12 , currentOffset = 155.0 ;
const float millisecs = 1000000.0 , currFactor = 1000.0 / 400.0 , unitValue = ( 5.0 / 1023.0 ) * 1000 ; //   1000mA per 400mV | (5V/1023)*1000 ~= 4.887 mV

//Button Pinouts - Pin numbering may change throughout the project
const byte freqPin = 5 , buzzerPin = 4 , nextButtonPin = 3 , prevButtonPin = 2 , voltSel = 7 , resistanceAdcPin = A0 , voltageAdcPin = A1 ;
const byte currentAdcPin = A2 , resistanceSelectPins [ resistanceRanges ] = { 12 , 11 , 10 , 9 , 8 } ;

//Variables
byte mode_select = 1 , idx , samples , g , j , h , resHalfIndex ;
volatile bool next = false , prev = false ;
int resistanceMeasuredValues [ resistanceRanges ] ;
unsigned long highPeriod , lowPeriod ;
float frequency , readout , resMultiplyFactor , resDivider , current , resistance , contVal , shuntVoltage , currSensorVal , voltageVal ;
String resUnit , resRange , freqUnit , currentUnit ;

LiquidCrystal_I2C LCD ( LCD_ADDR , LCD_ROWS , LCD_COLS ) ;      //    SDA - Pin A4 | SCL - Pin A5

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

bool checkDisconnectedRes ( void ) {
  byte count = 0 ;
  int trigger = 80 ;
  for ( g = 0 ; g < resistanceRanges ; g++ ) {
    if ( getAbsVal ( full - resistanceMeasuredValues [ g ] ) <= trigger ) count++ ; }
  return count == resistanceRanges ; }

void resistanceTest ( void ) {
  resistance = 0.0 ;
  samples = 40 ;

  for ( j = 0 ; j < samples ; j++ ) {

    for ( idx = 0 ; idx < resistanceRanges ; idx++ ) {
      setResistancePin ( idx ) ;
      delay ( adcDelay ) ;
      resistanceMeasuredValues [ idx ] = analogRead ( resistanceAdcPin ) ; } //end for

    digitalWrite ( resistanceSelectPins [ 4 ] , HIGH ) ;
    resHalfIndex = getClosestToFull ( resistanceMeasuredValues , resistanceRanges ) ;

    switch ( resHalfIndex ) {
      case 0  : resMultiplyFactor = resistanceFactors [ 0 ] ; resUnit = " Ohm"   ; resRange = "100R" ; resDivider = 1.0    ; break ;
      case 1  : resMultiplyFactor = resistanceFactors [ 1 ] ; resUnit = " Ohm"   ; resRange = "1K"   ; resDivider = 1.0    ; break ;
      case 2  : resMultiplyFactor = resistanceFactors [ 2 ] ; resUnit = " kOhm"  ; resRange = "10K"  ; resDivider = 1000.0 ; break ;
      case 3  : resMultiplyFactor = resistanceFactors [ 3 ] ; resUnit = " kOhm"  ; resRange = "100K" ; resDivider = 1000.0 ; break ;
      case 4  : resMultiplyFactor = resistanceFactors [ 4 ] ; resUnit = " kOhm"  ; resRange = "1M"   ; resDivider = 1000.0 ; break ;
      default : break ; } //end switch

    resistance += ( ( ( float ) resistanceMeasuredValues [ resHalfIndex ] * resMultiplyFactor ) / (float) getAbsVal ( full - resistanceMeasuredValues [ resHalfIndex ] ) ) ; } //end for
  
  resistance /= (float) samples * resDivider ;    //take the average

  if ( checkDisconnectedRes ( ) ) {
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "                    " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "      Overload      " ) ; }

  else {
    if ( resHalfIndex == 4 && resistance > 1000.0 ) {
      resistance /= 1000.0 ;
      resUnit = " MOhm" ; }

    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "     Range: " + resRange + "    " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "     " + String ( resistance ) + resUnit + "       " ) ; } } //end resistanceTest

void voltageTest ( void ) {
  readout = 0.0 ;
  voltageVal = 0.0 ;
  samples = 200 ;

  for ( h = 0 ; h < samples ; h++ ) {
    voltageVal = (float) analogRead ( voltageAdcPin ) ;

    if ( digitalRead ( voltSel ) ) voltageVal *= 5.77455 ;
    if ( !digitalRead ( voltSel ) ) voltageVal *= 1.533 ;

    voltageVal *= voltageFactor ;

    if ( voltageVal < 0.39 ) voltageVal = 0.0 ;
    readout += voltageVal ; } //end for

  readout /= (float) samples ;    //take the average

  if ( readout >= 13.5 ) readout *= 1.015 ;
  if ( readout <= 4.9 ) readout *= 0.97 ;

  LCD.setCursor( 0 , 2 ) ;
  LCD.print ( "      " + String ( readout ) + " V       " ) ; } //end voltageTest

void nextButtonPressed ( void ) {
  next = true ; }

void prevButtonPressed ( void ) {
  prev = true ; }

void currentTest ( void ) {     //min 3 mA | max. 2A
  currSensorVal = 0.0 ;
  current = 0 ;
  samples = 100 ;

  for ( idx = 0 ; idx < samples ; idx++ ) {
    currSensorVal += (float) analogRead ( currentAdcPin ) ;
    delay ( adcDelay ) ; }

  currSensorVal /= (float) samples ;
  //Serial.print ( currSensorVal ) ;
  //Serial.print ( "  " ) ;
  shuntVoltage = unitValue * currSensorVal ;
  //Serial.println ( shuntVoltage ) ;
  current = ( ( shuntVoltage - Vref ) * currFactor * currentCalibFactor ) /*- currentOffset*/ ;

  LCD.setCursor ( 0 , 2 ) ;

  if ( current <= 1000.0 ) currentUnit = " mA" ;

  if ( current > 1000.0 ) {
    currentUnit = " A" ;
    current /= 1000.0 ; }

  if ( current > 0.0 ) LCD.print ( "     " + String ( current ) + currentUnit + "           " ) ;
  else LCD.print ( "         OL         " ) ; }

void frequencyTest ( void ) {     //25Hz -> Lower-Bound Frequency
                                  //~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  frequency = 0.0 ;
  samples = 30 ;

  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH , timeOut ) ;
    lowPeriod = pulseIn ( freqPin , LOW , timeOut ) ;
    frequency += millisecs / (float) ( highPeriod + lowPeriod ) ; } //end for

  frequency /= (float) samples ;    //take the average

  if ( frequency >= freqCorrectionLimit ) frequency *= freqCalibFactor ;

  if ( frequency >= 1000.0 ) {
    frequency /= 1000.0 ;
    freqUnit = " kHz" ; }
  
  else freqUnit = " Hz" ;

  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( "    " + String ( frequency ) + freqUnit + "        " ) ; } //end frequencyTest

void continuityTest ( void ) {

  setResistancePin ( 0 ) ;
  contVal = (float) analogRead ( resistanceAdcPin ) ;
  contVal = ( contVal * resistanceFactors [ 0 ] ) / (float) getAbsVal ( full - (int) contVal ) ;

  if ( contVal <= contCutoff ) {
    resUnit = " Ohm" ;
    tone ( buzzerPin , buzzerTone ) ;
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "      CONNECTED     " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "      " + String ( contVal ) + resUnit + "         " ) ; } //end if
  
  else {
      noTone ( buzzerPin ) ;
      LCD.setCursor ( 0 , 1 ) ;
      LCD.print ( "                    " ) ;
      LCD.setCursor ( 0 , 2 ) ;
      LCD.print ( "     Open Loop      " ) ; } } //end continuityTest

void setup ( ) {
  for ( h = 0 ; h < resistanceRanges ; h++ ) {
    pinMode ( resistanceSelectPins [ h ] , OUTPUT ) ;
    digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

  attachInterrupt ( digitalPinToInterrupt ( nextButtonPin ) , nextButtonPressed , FALLING ) ;
  attachInterrupt ( digitalPinToInterrupt ( prevButtonPin ) , prevButtonPressed , FALLING ) ;

  pinMode ( nextButtonPin , INPUT_PULLUP ) ;
  pinMode ( prevButtonPin , INPUT_PULLUP ) ;
  pinMode ( resistanceAdcPin , INPUT ) ;
  pinMode ( currentAdcPin , INPUT ) ;
  pinMode ( voltageAdcPin , INPUT ) ;
  pinMode ( freqPin , INPUT ) ;
  pinMode ( voltSel , INPUT ) ;
  pinMode ( buzzerPin , OUTPUT ) ;

  LCD.init ( ) ;
  LCD.backlight ( ) ;
  LCD.clear ( ) ; } //end setup

void loop ( ) {

  if ( next || prev ) {
    delay ( bounceDelay ) ;
    if ( next ) { mode_select++ ; next = false ; }
    if ( prev ) { mode_select-- ; prev = false ; }
      
    for ( h = 0 ; h < resistanceRanges ; h++ ) {
      digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

    LCD.clear ( ) ;
    noTone ( buzzerPin ) ;

    if ( mode_select == total_modes + 1 ) mode_select = 1 ;
    if ( mode_select == 0 ) mode_select = total_modes ; } //end if

  switch ( mode_select ) {
    case 1 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "      Voltmeter     " ) ; voltageTest ( ) ; break ;
    case 2 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "     Ampermeter     " ) ; currentTest ( ) ; break ;
    case 3 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "     Ohm-meter      " ) ; resistanceTest ( ) ; break ;
    case 4 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "   Frequency-meter  " ) ; frequencyTest ( ) ;  break ;
    case 5 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Continuity test   " ) ; continuityTest ( ) ; break ;
    case 6 : LCD.setCursor ( 0 , 0 ) ; LCD.print ( "  Temperature test  " ) ; break ;
    default : break ; } } //end loop