/*
 * The_Eye by Jonathan Zepp - @DaJMasta
 * Version 1.1 - April 1, 2017
 * For the Eye project
 * 
 *
 *ver 1.1 changes:
 *Added auto gamma functionality
 *Tweaked auto brightness and contrast values
 *Changed the way auto brightness applies to be more useful
 *
 *Built in Arduino iDE 1.6.5
 *Uses Arduino_STM32 for the maple port to the Arduino IDE
 */

#define redLEDPin 20
#define orangeLEDPin 19
#define yellowLEDPin 18
#define greenLEDPin 17

#define col0Pin 3
#define col1Pin 4
#define col2Pin 5
#define col3Pin 6
#define col4Pin 7
#define col5Pin 8
#define col6Pin 9
#define col7Pin 10

#define rowInhibitPin 28
#define rowSelectAPin 25
#define rowSelectBPin 26
#define rowSelectCPin 27

#define lightsDataPin 30
#define lightsClockPin 29
#define light8Pin 31

#define autoContrastFactor 1.35
#define readingScalar 1.15
#define autoBrightnessScalar 0.80
#define autoGammaFactor 1.65
#define autoBrightnessOffset 150

#define orangeCalib 70
#define yellowCalib 130
#define greenCalib 200

int readings[64], minimums[64], maximums[64], adjustedReadings[64], smoothestOffset[64] ;
int minimum, maximum, average, adjustedAverage, frameTime, calibrationScore, frameTimeAverage, smoothestOffsetMax, frameMin, frameMax, adjFrameMin, adjFrameMax ;
boolean bRed, bOrange, bYellow, bGreen, bAutoBrightness, bAutoContrast, bAutoGamma ;
boolean lights[9] ;
long frameCount, lastFrame ;
char hostCommand[11] ;                                             //   <CMD:XXX#>/n
float smoothestFrameAverage ;

void setup() {
  
  pinMode(redLEDPin, OUTPUT) ;
  pinMode(orangeLEDPin, OUTPUT) ;
  pinMode(yellowLEDPin, OUTPUT) ;
  pinMode(greenLEDPin, OUTPUT) ;
  pinMode(col0Pin, INPUT) ;
  pinMode(col1Pin, INPUT) ;
  pinMode(col2Pin, INPUT) ;
  pinMode(col3Pin, INPUT) ;
  pinMode(col4Pin, INPUT) ;
  pinMode(col5Pin, INPUT) ;
  pinMode(col6Pin, INPUT) ;
  pinMode(col7Pin, INPUT) ;
  pinMode(rowInhibitPin, OUTPUT) ;
  pinMode(rowSelectAPin, OUTPUT) ;
  pinMode(rowSelectBPin, OUTPUT) ;
  pinMode(rowSelectCPin, OUTPUT) ;
  pinMode(lightsDataPin, OUTPUT) ;
  pinMode(lightsClockPin, OUTPUT) ;
  pinMode(light8Pin, OUTPUT) ;
  
  minimum = -1 ;
  maximum = -1 ;
  average = 0 ;
  adjustedAverage = 0 ;
  frameCount = 0 ;
  lastFrame = 0 ;
  frameTimeAverage = 0 ;
  frameTime = 0 ;
  calibrationScore = 0 ;
  smoothestFrameAverage = 10000.0 ;
  smoothestOffsetMax = 0 ;
  frameMin = -1 ;
  frameMax = -1 ;
  adjFrameMin = -1 ;
  adjFrameMax = -1 ;
  
  bRed = true ;
  bOrange = true ;
  bYellow = true ;
  bGreen = true ;
  bAutoBrightness = false ;
  bAutoContrast = false ;
  bAutoGamma = false ;
  for(byte i = 0; i < 11; i++)
    hostCommand[i] = ' ' ;
  
  for(byte i = 0; i < 9; i++)
    lights[i] = false ;
  
  for(byte i = 0; i < 64; i++) {
    readings[i] = -1 ;
    minimums[i] = -1 ;
    maximums[i] = -1 ;
    adjustedReadings[i] = -1 ;
    smoothestOffset[i] = 0 ;
  }
  
  lastFrame = millis() ;
  
  setLights(true) ;
  updateIndicators() ;
  readSensor() ;
  analyzeFrame() ;
  
  delay(500) ;
  
  Serial.println("Initialization complete!") ;
  
  bOrange = false ;
  bYellow = false ;
  bGreen = false ;
  
  setLights(false) ;
  updateIndicators() ;
}

void loop() {
  if(Serial.available()) {
    for(byte i = 0; i < 11; i++) {
      if(Serial.available())
        hostCommand[i] = Serial.read() ;
      else
        break ;
    }
  }
  
  receiveCommand() ;
      
  readSensor() ;
  analyzeFrame() ;
  autoBrightnessContrastGamma() ;
  
  for(byte i = 0; i < 11; i++)
    hostCommand[i] = ' ' ;
    
  if(frameCount % 10 == 0) {
    evalCalibration() ;
    updateIndicators() ;
    frameTime = millis() - lastFrame ;
    lastFrame = millis() ;
    frameTimeAverage = frameTime / 10 ;
    if(frameCount % 100 == 0)
      writeLights() ;
  }
}

/*
Commands:
RFM - returnFrame(number)
LGT - lights (off/on)
LGO - light # on (0-8)
LGX - light # off (0-8)
ABR - auto brightness (off/on)
ACN - auto contrast (off/on)
AGM - auto gamma (off/on)

*/


void receiveCommand() {
  if(hostCommand[0] == ' ')
    return ;
  if(hostCommand[0] != '<' || hostCommand[1] != 'C' || hostCommand[2] != 'M' || hostCommand[3] != 'D' || hostCommand[4] != ':' || hostCommand[9] != '>') {
    Serial.print("[BADCMD:") ;
    Serial.print(hostCommand) ;
    Serial.println("]") ;
    delay(5) ;
    return ;
  }
  
  if(hostCommand[5] == 'R' && hostCommand[6] == 'F' && hostCommand[7] == 'M')
    returnFrame(hostCommand[8] - '0') ;
  else if(hostCommand[5] == 'L' && hostCommand[6] == 'G' && hostCommand[7] == 'T') {
    if(hostCommand[8] == '0')
      setLights(false) ;
    else if(hostCommand[8] == '1')
      setLights(true) ;
    else {
      Serial.print("[BADCMD:") ;
      Serial.print(hostCommand) ;
      Serial.println("]") ;
      delay(5) ;
      return ;
    }
  }
  else if(hostCommand[5] == 'L' && hostCommand[6] == 'G' && hostCommand[7] == 'O') {
    if(hostCommand[8] - '0' >= 0 && hostCommand[8] - '0' < 9)
      lights[hostCommand[8] - '0'] = true ;
    else {
      Serial.print("[BADCMD:") ;
      Serial.print(hostCommand) ;
      Serial.println("]") ;
      delay(5) ;
      return ;
    }
  }
  else if(hostCommand[5] == 'L' && hostCommand[6] == 'G' && hostCommand[7] == 'X') {
    if(hostCommand[8] - '0' >= 0 && hostCommand[8] - '0' < 9)
      lights[hostCommand[8] - '0'] = false ;
    else {
      Serial.print("[BADCMD:") ;
      Serial.print(hostCommand) ;
      Serial.println("]") ;
      delay(5) ;
      return ;
    }
  }
  else if(hostCommand[5] == 'A' && hostCommand[6] == 'B' && hostCommand[7] == 'R') {
    if(hostCommand[8] == '0')
      bAutoBrightness = false ;
    else if(hostCommand[8] == '1')
      bAutoBrightness = true ;
    else {
      Serial.print("[BADCMD:") ;
      Serial.print(hostCommand) ;
      Serial.println("]") ;
      delay(5) ;
      return ;
    }
  }
  else if(hostCommand[5] == 'A' && hostCommand[6] == 'C' && hostCommand[7] == 'N') {
    if(hostCommand[8] == '0')
      bAutoContrast = false ;
    else if(hostCommand[8] == '1')
      bAutoContrast = true ;
    else {
      Serial.print("[BADCMD:") ;
      Serial.print(hostCommand) ;
      Serial.println("]") ;
      delay(5) ;
      return ;
    }
  }
  else if(hostCommand[5] == 'A' && hostCommand[6] == 'G' && hostCommand[7] == 'M') {
    if(hostCommand[8] == '0')
      bAutoGamma = false ;
    else if(hostCommand[8] == '1')
      bAutoGamma = true ;
    else {
      Serial.print("[BADCMD:") ;
      Serial.print(hostCommand) ;
      Serial.println("]") ;
      delay(5) ;
      return ;
    }
  }
  Serial.println("[CMDEX]") ;
  delay(1) ;
}

void evalCalibration() {
  calibrationScore = 0 ;
  if(maximum - minimum > 2300)
    calibrationScore += 40 ;
  else if(maximum - minimum > 1900)
    calibrationScore += 25 ;
  else if(maximum - minimum > 1300)
    calibrationScore += 15 ;
  else if(maximum - minimum > 950)
    calibrationScore += 5 ;
    
  for(byte i = 0; i < 64; i++) {
    if(maximums[i] - minimums[i] > 1900)
      calibrationScore += 5 ;
    else if(maximums[i] - minimums[i] > 1200)
      calibrationScore += 2 ;
    else if(maximums[i] - minimums[i] > 950)
      calibrationScore++ ;
  }
  
  calibrationScore += 100 / smoothestFrameAverage ;
}

void writeLights() {
  for(byte i = 0; i < 8; i++) {
    digitalWrite(lightsDataPin, lights[i]) ;
    digitalWrite(lightsClockPin, HIGH) ;
    delayMicroseconds(10) ;
    digitalWrite(lightsClockPin, LOW) ;
    delayMicroseconds(5) ;
  }
  digitalWrite(light8Pin, lights[8]) ;
}

void setLights(boolean toggle) {
  for(byte i = 0; i < 9; i++)
    lights[i] = toggle ;
  writeLights() ;
}

void updateIndicators() {
  bRed = false ;
  bOrange = false ;
  bYellow = false ;
  bRed = false ;
  
  if(calibrationScore < orangeCalib)
    bRed = true ;
  else if(calibrationScore < yellowCalib)
    bOrange = true ;
  else if(calibrationScore < greenCalib)
    bYellow = true ;
  else
    bGreen = true ;
  
  digitalWrite(redLEDPin, !bRed) ;
  digitalWrite(orangeLEDPin, !bOrange) ;
  digitalWrite(yellowLEDPin, !bYellow) ;
  digitalWrite(greenLEDPin, !bGreen) ;
}

void analyzeFrame() {
  long temp = 0 ;
  float tempScalar = 0 ;
  float frameAverageOffset = 0.0 ;
  
  frameMin = readings[0] ;
  frameMax = readings[0] ;
  
  for(byte i = 0; i < 64; i++) {
    if((readings[i] < minimums[i]) || (minimums[i] == -1))
      minimums[i] = readings[i] ;
    if((readings[i] > maximums[i]) || (maximums[i] == -1))
      maximums[i] = readings[i] ;
    
    if((readings[i] < minimum) || (minimum == -1))
      minimum = readings[i] ;
    else if(readings[i] > maximum)
      maximum = readings[i] ;
    
    if(frameMin > readings[i])
      frameMin = readings[i] ;
    else if(frameMax < readings[i])
      frameMax = readings[i] ;
    temp += readings[i] ;    
  }
  average = temp / 64 ;
  
  for(byte i = 0; i < 64; i++) {
    tempScalar = (maximum - minimum) / (maximums[i] - minimums[i]) ;                            //Range scalar pixel vs. all time
    
    adjustedReadings[i] = ((readings[i] - minimums[i]) * tempScalar) + minimum ;      

    adjustedReadings[i] *= (4096 / (maximum - minimum) * readingScalar) ;
    
    if(calibrationScore < orangeCalib)
      adjustedReadings[i] = ((adjustedReadings[i] - readings[i]) * 0.25) + readings[i] ;
    else if(calibrationScore < yellowCalib)
      adjustedReadings[i] = ((adjustedReadings[i] - readings[i]) * 0.5) + readings[i] ;
    else if(calibrationScore < greenCalib)
      adjustedReadings[i] = ((adjustedReadings[i] - readings[i]) * 0.75) + readings[i] ;

    if(adjustedReadings[i] > adjFrameMax || adjFrameMax == -1)
      adjFrameMax = adjustedReadings[i] ;
    if(adjustedReadings[i] < adjFrameMin || adjFrameMin == -1)
      adjFrameMin = adjustedReadings[i] ;
  }
  
  temp = 0 ;
  for(int i = 0; i < 64; i++)
    temp += adjustedReadings[i] ;
  adjustedAverage = temp / 64 ;
  
  temp = 0 ;
  for(int i = 0; i < 64; i++)                                                              //Compare to smoothest frame, save offsets from original readings if smoother
    temp += abs(adjustedAverage - adjustedReadings[i]) ;
  frameAverageOffset = (float)(temp / 64.0) ;
  
  if(smoothestFrameAverage > frameAverageOffset) {
    smoothestFrameAverage = frameAverageOffset ;
    smoothestOffsetMax = 0 ;
    for(int i = 0; i < 64; i++) {
      smoothestOffset[i] = average - readings[i] ;
      if(abs(smoothestOffset[i]) > smoothestOffsetMax)
        smoothestOffsetMax = abs(smoothestOffset[i]) ;
    }
  }
  
  tempScalar = (maximum - minimum) / 2 + minimum ;      
}

void autoBrightnessContrastGamma() {
  double gammaAdjusted = 0.0 ;

  adjFrameMin = -1 ;
  adjFrameMax = -1 ;
  
  if(!bAutoBrightness && !bAutoContrast && !bAutoGamma)
    return ;
    
  for(int i = 0; i < 64; i++) {
    if(bAutoContrast)
      adjustedReadings[i] += (adjustedReadings[i] - adjustedAverage) * autoContrastFactor ;
   
  
    if(bAutoGamma){
      gammaAdjusted = pow(adjustedReadings[i] / 4096.0, autoGammaFactor) * 4096.0 ;
      adjustedReadings[i] = int(gammaAdjusted) ;
    }

    if(bAutoBrightness)
      adjustedReadings[i] += (((((maximum - minimum) / 2) + minimum) - (((adjFrameMax - adjFrameMin) / 2) + adjFrameMin) + autoBrightnessOffset) * autoBrightnessScalar) ;
  
    adjustedReadings[i] = constrain(adjustedReadings[i], 0, 4095) ;
  }
}

void readSensor() {
  digitalWrite(rowInhibitPin, LOW) ;
  
  for(byte i = 0; i < 8; i++) {
    switch(i) {
      case 0:    digitalWrite(rowSelectAPin, LOW) ;
                 digitalWrite(rowSelectBPin, LOW) ;
                 digitalWrite(rowSelectCPin, LOW) ;
                 break ;
      case 1:    digitalWrite(rowSelectAPin, HIGH) ;
                 digitalWrite(rowSelectBPin, LOW) ;
                 digitalWrite(rowSelectCPin, LOW) ;
                 break ;  
      case 2:    digitalWrite(rowSelectAPin, LOW) ;
                 digitalWrite(rowSelectBPin, HIGH) ;
                 digitalWrite(rowSelectCPin, LOW) ;
                 break ;
      case 3:    digitalWrite(rowSelectAPin, HIGH) ;
                 digitalWrite(rowSelectBPin, HIGH) ;
                 digitalWrite(rowSelectCPin, LOW) ;
                 break ;        
      case 4:    digitalWrite(rowSelectAPin, LOW) ;
                 digitalWrite(rowSelectBPin, LOW) ;
                 digitalWrite(rowSelectCPin, HIGH) ;
                 break ;
      case 5:    digitalWrite(rowSelectAPin, HIGH) ;
                 digitalWrite(rowSelectBPin, LOW) ;
                 digitalWrite(rowSelectCPin, HIGH) ;
                 break ;  
      case 6:    digitalWrite(rowSelectAPin, LOW) ;
                 digitalWrite(rowSelectBPin, HIGH) ;
                 digitalWrite(rowSelectCPin, HIGH) ;
                 break ;
      case 7:    digitalWrite(rowSelectAPin, HIGH) ;
                 digitalWrite(rowSelectBPin, HIGH) ;
                 digitalWrite(rowSelectCPin, HIGH) ;
                 break ;          
    default:  break ;
    }  
    readings[i*8] = analogRead(col0Pin) ;
    readings[i*8+1] = analogRead(col1Pin) ;
    readings[i*8+2] = analogRead(col2Pin) ;
    readings[i*8+3] = analogRead(col3Pin) ;
    readings[i*8+4] = analogRead(col4Pin) ;
    readings[i*8+5] = analogRead(col5Pin) ;
    readings[i*8+6] = analogRead(col6Pin) ;
    readings[i*8+7] = analogRead(col7Pin) ;
  }
  
  frameCount++ ;
}

void returnFrame(byte frameType) {        //0 - Raw, 1 - Adjusted, 2 - Max, 3 - Mins, 4 - Stats, 5 - Offsets
  int smoothestAvgOff = 0 ;
  smoothestAvgOff = (int)(smoothestFrameAverage * 100) ;
  
  switch(frameType) {
    case 4:  Serial.print("[SFM") ;
              Serial.print(frameCount, HEX) ;
              Serial.print("I") ;
              Serial.print(minimum, HEX) ;
              Serial.print("X") ;
              Serial.print(maximum, HEX) ;
              Serial.print("V") ;
              Serial.print(average, HEX) ;
              Serial.print("U") ;
              Serial.print(frameTimeAverage, HEX) ;
              Serial.print("I") ;
              Serial.print(calibrationScore, HEX) ;
              Serial.print("S") ;
              Serial.print(smoothestAvgOff, HEX) ;
              Serial.print("M") ;
              Serial.print(smoothestOffsetMax, HEX) ;
              Serial.print("R") ;
              Serial.print(bAutoBrightness, BIN) ;
              Serial.print("N") ;
              Serial.print(bAutoContrast, BIN) ;
              Serial.print("G") ;
              Serial.print(bAutoGamma, BIN) ;
              Serial.print("L") ;
              for(byte i = 0; i < 9; i++)
                Serial.print(lights[i], BIN) ;
              break ;
    case 0:   Serial.print("[RFM") ;
              Serial.print(frameCount, HEX) ;
              for(byte i = 0; i < 8; i++) {
                Serial.print(".") ;
                for(byte j = 0; j < 8; j++) {
                  Serial.print(readings[(i*8) + j], HEX) ;
                  if(j < 7)
                    Serial.print(",") ;
                }
              }
              break ;
     case 1:   Serial.print("[AFM") ;
              Serial.print(frameCount, HEX) ;
              for(byte i = 0; i < 8; i++) {
                Serial.print(".") ;
                for(byte j = 0; j < 8; j++) {
                  Serial.print(adjustedReadings[(i*8) + j], HEX) ;
                  if(j < 7)
                    Serial.print(",") ;
                }
              }
              break ;
     case 2:   Serial.print("[XFM") ;
              Serial.print(frameCount, HEX) ;
              for(byte i = 0; i < 8; i++) {
                Serial.print(".") ;
                for(byte j = 0; j < 8; j++) {
                  Serial.print(maximums[(i*8) + j], HEX) ;
                  if(j < 7)
                    Serial.print(",") ;
                }
              }
              break ;
     case 3:   Serial.print("[MFM") ;
              Serial.print(frameCount, HEX) ;
              for(byte i = 0; i < 8; i++) {
                Serial.print(".") ;
                for(byte j = 0; j < 8; j++) {
                  Serial.print(minimums[(i*8) + j], HEX) ;
                  if(j < 7)
                    Serial.print(",") ;
                }
              }
              break ;
      case 5:   Serial.print("[OFM") ;
              Serial.print(frameCount, HEX) ;
              for(byte i = 0; i < 8; i++) {
                Serial.print(".") ;
                for(byte j = 0; j < 8; j++) {
                  Serial.print(smoothestOffset[(i*8) + j], HEX) ;
                  if(j < 7)
                    Serial.print(",") ;
                }
              }
              break ;
      default:  Serial.print("[RETURNERROR") ;
    }
  Serial.println("]") ;
  delay(25) ;
}
