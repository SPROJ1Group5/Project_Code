// Include necessary libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LM35.h>

// Defined Constants
const byte total_modes = 6 , LCD_ADDR = 0x27 , LCD_ROWS = 20 , LCD_COLS = 4 , resistanceRanges = 5 ;
const unsigned int buzzerTone = 3000 ;
const int full = 1023 , half = 512 ;
const unsigned long adcDelay = 4 , bounceDelay = 170 ;
const float resistanceFactors [ 5 ] = { 100.0 , 996.0 , 99300.0 , 100000.0 , 1000000.0 } ;
const float voltageFactor = 5.0 / 1023.0 ;

// Button Pinouts
// SDA - Pin A4 | SCL - Pin A5
const byte prevButtonPin = 2 , nextButtonPin = 3 , buzzerPin = 6 , freqPin = 5 , relay2Pin = 7 , resistanceSelectPins [ 5 ] = { 8 , 9 , 10 , 11 , 12 } ;
const byte voltSel = 13 , relay1Pin = A0 , temperatureAdcPin = A1 , currentAdcPin = A2 , batteryAdcPin = A3 , resistanceAdcPin = A6 , voltageAdcPin = A7 ;

// Variables
int resistanceMeasuredValues [ 5 ] ;
byte mode_select = 1 ;
volatile bool next = false , prev = false ;
bool safeOperation ;

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
  byte g ;
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
  byte count = 0 , g ;
  int trigger = 80 ;
  for ( g = 0 ; g < resistanceRanges ; g++ ) {
    if ( getAbsVal ( full - resistanceMeasuredValues [ g ] ) <= trigger ) count++ ; }
  return count == resistanceRanges ; }

void resistanceTest ( void ) {

  float resistance = 0.0 , resMultiplyFactor , resDivider ;
  byte samples = 40 , j , i , resHalfIndex ;
  String resUnit , resRange ;

  // Iterate through the loop to get multiple measurements
  for ( j = 0 ; j < samples ; j++ ) {

    // Select the specified range and save the measured ADC value to the corresponding array element
    for ( i = 0 ; i < resistanceRanges ; i++ ) {
      setResistancePin ( i ) ;
      delay ( adcDelay ) ;
      resistanceMeasuredValues [ i ] = analogRead ( resistanceAdcPin ) ; }

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
    resistance += ( ( ( float ) resistanceMeasuredValues [ resHalfIndex ] * resMultiplyFactor ) / (float) getAbsVal ( full - resistanceMeasuredValues [ resHalfIndex ] ) ) ; }
  
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
  float voltage = 0.0 , value ;
  byte samples = 200 , h ;

  // Take measurement samples and adjust the range according to the input selection pin
  for ( h = 0 ; h < samples ; h++ ) {
    value = (float) analogRead ( voltageAdcPin ) ;

    if ( digitalRead ( voltSel ) ) value *= 5.77455 ;
    if ( !digitalRead ( voltSel ) ) value *= 1.533 ;

    value *= voltageFactor ;
    voltage += value ; }

  // Take the average value
  voltage /= (float) samples ;

  // Print out the voltage value to the LCD display
  LCD.setCursor( 0 , 2 ) ;
  LCD.print ( "     " + String ( voltage ) + " V        " ) ; }

// Interrupt handlers
void nextButtonPressed ( void ) {
  next = true ; }

void prevButtonPressed ( void ) {
  prev = true ; }

void currentTest ( void ) {
  float value = 0.0 , current = 0 , shuntVoltage ;
  const float currentCalibFactor = 1.31 , Vref = 2495.12 ;
  const float currFactor = 1000.0 / 400.0 , unitValue = ( 5.0 / 1023.0 ) * 1000 ; // 1000mA per 400mV | (5V/1023)*1000 ~= 4.887 mV
  byte samples = 100 , idx ;

  for ( idx = 0 ; idx < samples ; idx++ ) {
    value += (float) analogRead ( currentAdcPin ) ;
    delay ( adcDelay ) ; }

  value /= (float) samples ;
  shuntVoltage = unitValue * value ;
  current = ( ( shuntVoltage - Vref ) * currFactor * currentCalibFactor ) ;

  LCD.setCursor ( 0 , 2 ) ;
  if ( current > 0.0 ) LCD.print ( "     " + String ( current ) + " mA        " ) ;
  else LCD.print ( "         OL         " ) ; }

void frequencyTest ( void ) {
  // 25Hz -> Lower-Bound Frequency | ~100 kHz -> Upper-Bound Frequency (+/- 5% error)
  // Reset the frequency and declare the number of samples to be taken from the input

  float frequency = 0.0 ;
  const float millisecs = 1000000.0 , freqCorrectionLimit = 30000.0 , freqCalibFactor = 1.0555 ;
  byte samples = 30 , i ;
  String freqUnit ;
  const unsigned long timeOut = 100000 ;
  unsigned long highPeriod , lowPeriod ;

  // Sample measurements
  for ( i = 0 ; i < samples ; i++  ) {
    highPeriod = pulseIn ( freqPin , HIGH , timeOut ) ;
    lowPeriod = pulseIn ( freqPin , LOW , timeOut ) ;
    frequency += millisecs / (float) ( highPeriod + lowPeriod ) ; }

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

  const float contCutoff = 10.0 ;
  float value ;

  // Select the first range (100R) and get the resistance value
  setResistancePin ( 0 ) ;
  value = (float) analogRead ( resistanceAdcPin ) ;
  value = ( value * resistanceFactors [ 0 ] ) / (float) getAbsVal ( full - (int) value ) ;

  // If the measured resistance is below the threshold value, beep the buzzer and show the resistance value on the display
  if ( value <= contCutoff ) {
    tone ( buzzerPin , buzzerTone ) ;
    LCD.setCursor ( 0 , 1 ) ;
    LCD.print ( "      CONNECTED     " ) ;
    LCD.setCursor ( 0 , 2 ) ;
    LCD.print ( "      " + String ( value ) + " Ohm      " ) ; }
  
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
  const float batHalfThreshVolt = 3.6 ;
  float batVal = (float) analogRead ( batteryAdcPin ) * voltageFactor ;
  return batVal >= batHalfThreshVolt ; }

void temperatureTest ( void ) {
  // This function gets the temperature value in degrees Celsius
  // Reset variables
  float value = 0.0 ;
  byte samples = 30 , j ;

  // Take multiple measurements
  for ( j = 0 ; j < samples ; j++ ) {
    value += LM35_Sensor.getTemp ( CELCIUS ) ;
    delay ( adcDelay ) ; }
  
  // Take the average value
  value /= (float) samples ;

  // Print out the result
  LCD.setCursor ( 0 , 1 ) ;
  LCD.print ( "     " + String ( value ) + " \337C     " ) ; }

void setup ( ) {

  safeOperation = batteryTest ( ) ;
  byte h ;

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
  byte k ;
  // Cycle measurement modes accordingly when a button is pressed
  if ( next || prev ) {
    safeOperation = batteryTest ( ) ;
    delay ( bounceDelay ) ;

    if ( next ) { mode_select++ ; next = false ; }
    if ( prev ) { mode_select-- ; prev = false ; }
      
    for ( k = 0 ; k < resistanceRanges ; k++ ) {
      digitalWrite ( resistanceSelectPins [ k ] , HIGH ) ; }

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