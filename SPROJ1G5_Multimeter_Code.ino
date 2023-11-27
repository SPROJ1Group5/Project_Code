// Include necessary libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LM35.h>

// Defined Constants
const byte total_modes = 6 , LCD_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , resistanceRanges = 5 ;
const unsigned int buzzerTone = 3000 ;
const int full = 1023 , half = 512 ;
const unsigned long adcDelay = 4 , bounceDelay = 170 , timeOut = 100000 ;
const float resistanceFactors [ 5 ] = { 100.0 , 996.0 , 99300.0 , 100000.0 , 1000000.0 } ;
const float voltageFactor = 5.0 / 1023.0 , batHalfThreshVolt = 3.6 ; //7.5V is the minimum input voltage of the 7805
const float freqCorrectionLimit = 30000.0 , contCutoff = 10.0 , freqCalibFactor = 1.0555 , currentCalibFactor = 1.31 , Vref = 2495.12 , currentOffset = 155.0 ;
const float millisecs = 1000000.0 , currFactor = 1000.0 / 400.0 , unitValue = ( 5.0 / 1023.0 ) * 1000 ; // 1000mA per 400mV | (5V/1023)*1000 ~= 4.887 mV
const String currentUnit = " mA" ;

// Button Pinouts
// SDA - Pin A4 | SCL - Pin A5
const byte prevButtonPin = 2 , nextButtonPin = 3 , buzzerPin = 6 , freqPin = 5 , relay2Pin = 7 , resistanceSelectPins [ 5 ] = { 8 , 9 , 10 , 11 , 12 } ;
const byte voltSel = 13 , relay1Pin = A0 , temperatureAdcPin = A1 , currentAdcPin = A2 , batteryAdcPin = A3 , resistanceAdcPin = A6 , voltageAdcPin = A7 ;

// Variables
byte mode_select = 1 , idx , samples , g , j , h , resHalfIndex ;
volatile bool next = false , prev = false ;
bool safeOperation ;
int resistanceMeasuredValues [ 5 ] ;
unsigned long highPeriod , lowPeriod ;
float frequency , readout , resMultiplyFactor , resDivider , current , resistance , contVal , shuntVoltage , currSensorVal , voltageVal , tempVal ;
String resUnit , resRange , freqUnit ;

// Declaring the LCD Display object
LiquidCrystal_I2C LCD ( LCD_ADDR , LCD_ROWS , LCD_COLS ) ;
LM35 LM35_Sensor ( (int) temperatureAdcPin ) ;

void selectRelayCombination ( byte selector ) {
  // This function manipulates the output pins that connect to the relays in order to switch the input to the correct circuit
  switch ( selector ) {
    /*VOLTAGE*/    case 0 : digitalWrite ( relay2Pin , LOW )  ; digitalWrite ( relay1Pin , LOW )  ; break ;
    /*FREQUENCY*/  case 1 : digitalWrite ( relay2Pin , HIGH ) ; digitalWrite ( relay1Pin , LOW )  ; break ;
    /*RESISTANCE*/ case 2 : digitalWrite ( relay2Pin , LOW )  ; digitalWrite ( relay1Pin , HIGH ) ; break ;
                   default : break ; } }

void setResistancePin ( byte onPin ) {
  // This function selects the specified MOSFET to be active, while switching off the others
  for ( g = 0 ; g < resistanceRanges ; g++ ) {
    if ( g == onPin ) { digitalWrite ( resistanceSelectPins [ g ] , LOW ) ; }
    else { digitalWrite ( resistanceSelectPins [ g ] , HIGH ) ; } } }

int getAbsVal ( int num  ) {
  // This function returns the absolute value of the input parameter
  if ( num >= 0 ) return num ;
  return - num ; }

byte getClosestToFull ( int * array , const byte size ) {
  // This function iterates through an array to find the index of the element that is closest to the value 512 and returns this index
  int minValue = getAbsVal ( array [ 0 ] - half ) , currentAbsVal ;
  byte minIndex = 0 ;
  for ( byte i = 1 ; i < size ; i++ ) {
    currentAbsVal = getAbsVal ( array [ i ] - half ) ;
    if ( currentAbsVal < minValue ) {
      minValue = currentAbsVal ;
      minIndex = i ; } }
  return minIndex; }

bool checkDisconnectedRes ( void ) {
  // This function checks whether there is something connected to the terminals
  byte count = 0 ;
  int trigger = 80 ;
  for ( g = 0 ; g < resistanceRanges ; g++ ) {
    if ( getAbsVal ( full - resistanceMeasuredValues [ g ] ) <= trigger ) count++ ; }
  return count == resistanceRanges ; }

void resistanceTest ( void ) {

  // Reset variables
  resistance = 0.0 ;
  samples = 40 ;

  // Iterate through the loop to get multiple measurements
  for ( j = 0 ; j < samples ; j++ ) {

    // Select the specified range and save the measured ADC value to the corresponding array element
    for ( idx = 0 ; idx < resistanceRanges ; idx++ ) {
      setResistancePin ( idx ) ;
      delay ( adcDelay ) ;
      resistanceMeasuredValues [ idx ] = analogRead ( resistanceAdcPin ) ; } //end for

    // Switch off the last MOSFET after taking the final measurement
    digitalWrite ( resistanceSelectPins [ 4 ] , HIGH ) ;

    // Get the index of the range whose measured ADC value is closest to 512, i.e. the measured voltage is closest to 2.5V
    resHalfIndex = getClosestToFull ( resistanceMeasuredValues , resistanceRanges ) ;

    // Adjust variables according to the selected range
    switch ( resHalfIndex ) {
      case 0  : resMultiplyFactor = resistanceFactors [ 0 ] ; resUnit = " Ohm"   ; resRange = "100R" ; resDivider = 1.0    ; break ;
      case 1  : resMultiplyFactor = resistanceFactors [ 1 ] ; resUnit = " Ohm"   ; resRange = "1K"   ; resDivider = 1.0    ; break ;
      case 2  : resMultiplyFactor = resistanceFactors [ 2 ] ; resUnit = " kOhm"  ; resRange = "10K"  ; resDivider = 1000.0 ; break ;
      case 3  : resMultiplyFactor = resistanceFactors [ 3 ] ; resUnit = " kOhm"  ; resRange = "100K" ; resDivider = 1000.0 ; break ;
      case 4  : resMultiplyFactor = resistanceFactors [ 4 ] ; resUnit = " kOhm"  ; resRange = "1M"   ; resDivider = 1000.0 ; break ;
      default : break ; }

    // Calculate the resistance value and add it to the variable
    resistance += ( ( ( float ) resistanceMeasuredValues [ resHalfIndex ] * resMultiplyFactor ) / (float) getAbsVal ( full - resistanceMeasuredValues [ resHalfIndex ] ) ) ; } //end for
  
  // Take the average value
  resistance /= ( (float) samples * resDivider ) ;

  // Check whether there is nothing connected to the input terminals
  if ( checkDisconnectedRes ( ) ) {
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "                    " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "      Overload      " ) ; }

  else {

    // Display resistance in Mega Ohms
    if ( resHalfIndex == 4 && resistance > 1000.0 ) {
      resistance /= 1000.0 ;
      resUnit = " MOhm" ; }

    // Print out the automatically selected measurement range and the resistance value
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "     Range: " + resRange + "    " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "    " + String ( resistance ) + resUnit + "        " ) ; } }

void voltageTest ( void ) {
  // Reset the variables
  readout = 0.0 ;
  voltageVal = 0.0 ;
  samples = 200 ;

  // Take measurement samples and adjust the range according to the input selection pin
  for ( h = 0 ; h < samples ; h++ ) {
    voltageVal = (float) analogRead ( voltageAdcPin ) ;

    if ( digitalRead ( voltSel ) ) voltageVal *= 5.77455 ;
    if ( !digitalRead ( voltSel ) ) voltageVal *= 1.533 ;

    voltageVal *= voltageFactor ;

    //if ( voltageVal < 0.39 ) voltageVal = 0.0 ;
    readout += voltageVal ; }

  // Take the average value
  readout /= (float) samples ;

  //if ( readout >= 13.5 ) readout *= 1.015 ;
  //if ( readout <= 4.9 ) readout *= 0.97 ;

  // Print out the voltage value to the LCD display
  LCD.setCursor( 0 , 2 ) ;
  LCD.print ( "     " + String ( readout ) + " V        " ) ; }

// Interrupt handlers
void nextButtonPressed ( void ) {
  next = true ; }

void prevButtonPressed ( void ) {
  prev = true ; }

void currentTest ( void ) {
  currSensorVal = 0.0 ;
  current = 0 ;
  samples = 100 ;

  for ( idx = 0 ; idx < samples ; idx++ ) {
    currSensorVal += (float) analogRead ( currentAdcPin ) ;
    delay ( adcDelay ) ; }

  currSensorVal /= (float) samples ;
  shuntVoltage = unitValue * currSensorVal ;
  current = ( ( shuntVoltage - Vref ) * currFactor * currentCalibFactor ) ;

  LCD.setCursor ( 0 , 2 ) ;

  if ( current > 0.0 ) LCD.print ( "     " + String ( current ) + currentUnit + "           " ) ;
  else LCD.print ( "         OL         " ) ; }

void frequencyTest ( void ) {
  // 25Hz -> Lower-Bound Frequency | ~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  // Reset the frequency and declare the number of samples to be taken from the input
  frequency = 0.0 ;
  samples = 30 ;

  // Sample measurements
  for ( idx = 0 ; idx < samples ; idx++  ) {
    highPeriod = pulseIn ( freqPin , HIGH , timeOut ) ;
    lowPeriod = pulseIn ( freqPin , LOW , timeOut ) ;
    frequency += millisecs / (float) ( highPeriod + lowPeriod ) ; } //end for

  // Take the average for a more stable measurement
  frequency /= (float) samples ;

  // If the frequency is above the adjusting limit, calibrate the frequency with a certain mulitplier
  if ( frequency >= freqCorrectionLimit ) frequency *= freqCalibFactor ;

  if ( frequency >= 1000.0 ) {
    frequency /= 1000.0 ;
    freqUnit = " kHz" ; }

  else freqUnit = " Hz" ;

  LCD.setCursor ( 0 , 2 ) ;
  LCD.print ( "    " + String ( frequency ) + freqUnit + "        " ) ; }

void continuityTest ( void ) {

  // Select the first range (100R) and get the resistance value
  setResistancePin ( 0 ) ;
  contVal = (float) analogRead ( resistanceAdcPin ) ;
  contVal = ( contVal * resistanceFactors [ 0 ] ) / (float) getAbsVal ( full - (int) contVal ) ;

  // If the measured resistance is below the threshold value, beep the buzzer and show the resistance value on the display
  if ( contVal <= contCutoff ) {
    resUnit = " Ohm" ;
    tone ( buzzerPin , buzzerTone ) ;
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "      CONNECTED     " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "      " + String ( contVal ) + resUnit + "         " ) ; }
  
  // Stop the beeping and show OL
  else {
      noTone ( buzzerPin ) ;
      LCD.setCursor ( 0 , 1 ) ;
      LCD.print ( "                    " ) ;
      LCD.setCursor ( 0 , 2 ) ;
      LCD.print ( "     Open Loop      " ) ; } }

bool batteryTest ( void ) {
  // This function checks whether the voltage of the supply 9V battery is above 7.5V,
  // which is the safe minimum input voltage of the 7805 linear voltage regulator IC.
  float batVal = (float) analogRead ( batteryAdcPin ) * voltageFactor ;
  return batVal >= batHalfThreshVolt ; }

void temperatureTest ( void ) {
  // This function gets the temperature value in degrees Celsius
  // Reset variables
  tempVal = 0.0 ;
  samples = 30 ;

  // Take multiple measurements
  for ( j = 0 ; j < samples ; j++ ) {
    tempVal += LM35_Sensor.getTemp ( CELCIUS ) ;
    delay ( adcDelay ) ; }
  
  // Take the average value
  tempVal /= (float) samples ;

  // Print out the result
  LCD.setCursor ( 0 , 1 ) ;
  LCD.print ( "     " + String ( tempVal ) + " \337C     " ) ; }

void setup ( ) {

  safeOperation = batteryTest ( ) ;

  for ( h = 0 ; h < resistanceRanges ; h++ ) {
    pinMode ( resistanceSelectPins [ h ] , OUTPUT ) ;
    digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

  // Button Pins
  pinMode ( nextButtonPin , INPUT_PULLUP ) ;
  pinMode ( prevButtonPin , INPUT_PULLUP ) ;

  // ADC Pins
  pinMode ( resistanceAdcPin  , INPUT ) ;
  pinMode ( currentAdcPin     , INPUT ) ;
  pinMode ( voltageAdcPin     , INPUT ) ;
  pinMode ( temperatureAdcPin , INPUT ) ;
  pinMode ( batteryAdcPin     , INPUT ) ;

  // Input Pins
  pinMode ( freqPin , INPUT ) ;
  pinMode ( voltSel , INPUT ) ;

  // Output Pins
  pinMode ( buzzerPin , OUTPUT ) ;
  pinMode ( relay1Pin , OUTPUT ) ;
  pinMode ( relay2Pin , OUTPUT ) ;

  // Interrupt handlers for switching modes
  attachInterrupt ( digitalPinToInterrupt ( nextButtonPin ) , nextButtonPressed , FALLING ) ;
  attachInterrupt ( digitalPinToInterrupt ( prevButtonPin ) , prevButtonPressed , FALLING ) ;

  // Initializing the LCD Display
  LCD.init ( ) ;
  LCD.backlight ( ) ;
  LCD.clear ( ) ; }

void loop ( ) {

  // Cycle measurement modes accordingly when a button is pressed
  if ( next || prev ) {
    safeOperation = batteryTest ( ) ;
    delay ( bounceDelay ) ;

    if ( next ) { mode_select++ ; next = false ; }
    if ( prev ) { mode_select-- ; prev = false ; }
      
    for ( h = 0 ; h < resistanceRanges ; h++ ) {
      digitalWrite ( resistanceSelectPins [ h ] , HIGH ) ; }

    LCD.clear ( ) ;
    noTone ( buzzerPin ) ;

    if ( mode_select == total_modes + 1 ) mode_select = 1 ;
    if ( mode_select == 0 ) mode_select = total_modes ; }

  LCD.setCursor ( 0 , 0 ) ;

  if ( safeOperation ) {
    // Switch to the next measurement mode
    switch ( mode_select ) {
      case 1 : LCD.print ( "      Voltmeter     " ) ; selectRelayCombination ( 0 ) ; voltageTest     ( ) ; break ;
      case 2 : LCD.print ( "     Ampermeter     " ) ; selectRelayCombination ( 0 ) ; currentTest     ( ) ; break ;
      case 3 : LCD.print ( "     Ohm-meter      " ) ; selectRelayCombination ( 2 ) ; resistanceTest  ( ) ; break ;
      case 4 : LCD.print ( "   Frequency-meter  " ) ; selectRelayCombination ( 1 ) ; frequencyTest   ( ) ; break ;
      case 5 : LCD.print ( "  Continuity test   " ) ; selectRelayCombination ( 2 ) ; continuityTest  ( ) ; break ;
      case 6 : LCD.print ( "  Temperature test  " ) ; selectRelayCombination ( 0 ) ; temperatureTest ( ) ; break ; 
      default : break ; } }
  
  else LCD.print ( "  Change Battery!   " ) ; }