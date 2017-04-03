/*
 * The_Eye_Interface by Jonathan Zepp - @DaJMasta
 * Version 1.1 - April 1, 2017
 * For the Eye project
 * 
 *
 *ver 1.1 changes:
 *Added auto gamma button and reported data
 *Changes to better data transmission rates
 *Added individual control for lights
 *
 *Built in Processing 3.0.1
 */


import processing.serial.* ;

Serial comPort ;

long frameCounter ;
int minimum, maximum, average, frameTimeAverage, calibrationScore, smoothestOffsetMax ;
boolean bAutoBrightness, bAutoContrast, bAutoGamma, bMouseClicked, bOverBright, bOverContrast, bOverGamma, bUpdateStats, bOverOff, bOverOn ;
int[] adjFrame = new int[64];
int[] rawFrame = new int[64];
int[] minsFrame = new int[64];
int[] maxsFrame = new int[64];
int[] offsetFrame = new int[64];
int[] adjustedGrays = new int[64];
int[] rawGrays = new int[64];
int[] minsGrays = new int[64];
int[] maxsGrays = new int[64];
int[] offsetGrays = new int[64];
int[] input = new int[64] ;
boolean[] lights = new boolean[9];
boolean[] lightsHover = new boolean[9] ;
PFont f;
String inBuffer, cmdReply ;
byte commandCounter ;
long responseTimeout ;
float smoothestAverageOffset ;

void setup() 
{
  size(800, 600);
  String portName = Serial.list()[0];
  println(Serial.list()) ;
  comPort = new Serial(this, portName, 230400);
  
  f = createFont("Georgia", 18);
  textFont(f);
  textAlign(CENTER, CENTER);
  
  frameCounter = 0 ;
  minimum = 0 ;
  maximum = 1 ;
  average = 0 ;
  frameTimeAverage = 0 ;
  calibrationScore = 0 ;
  smoothestOffsetMax = 10000 ;
  smoothestAverageOffset = 0.0 ;
  bAutoBrightness = false ;
  bAutoContrast = false ;
  bAutoGamma = false ;
  bMouseClicked = false ;
  bOverBright = false ;
  bOverContrast = false ;
  bOverGamma = false ;
  bUpdateStats = false ;
  bOverOff = false ;
  bOverOn = false ;
  commandCounter = 0 ;
  responseTimeout = 0 ;
  cmdReply = "" ;
  
  for(int i = 0; i < 64; i++) {
    adjustedGrays[i] = 0 ;
    rawGrays[i] = 0 ;
    minsGrays[i] = 0 ;
    maxsGrays[i] = 0 ;
    offsetGrays[i] = 0 ;
    adjFrame[i] = 0 ;
    rawFrame[i] = 0 ;
    minsFrame[i] = 0 ;
    maxsFrame[i] = 0 ;
    offsetFrame[i] = 0 ;
    input[i] = 0 ;
  }
  
  for(byte i = 0; i < 9; i++) {
    lights[i] = false ;
    lightsHover[i] = false ;
  }
}

void draw()
{
  update(mouseX, mouseY) ;
  
  background(255) ;             // Set background to white
  
  fill(0) ;
  
  inBuffer = "" ;
  
  do {
    if(responseTimeout < millis()) {
      if(bMouseClicked)
        sendCommand() ;
      else
        requestFrame() ;
      responseTimeout = millis() + 50 ;
    }
  }while(comPort.available() == 0) ;
  
  while(comPort.available() > 0){
    inBuffer = comPort.readStringUntil(10) ;
  }
  
  if(!bMouseClicked) {
    commandCounter++ ;
    if(commandCounter > 40)
      commandCounter = 0 ;
  }
  
  bMouseClicked = false ;
  
  packetTokenizer() ;
  
  grayscaleCompute() ;
  
  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      fill(adjustedGrays[x*8 + y]) ;
      rect(200 + (y * 50), 75 + (x * 50), 50, 50) ;
    }
  }
  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      fill(rawGrays[x*8 + y]) ;
      rect(650 + (y * 15), 10 + (x * 15), 15, 15) ;
    }
  }
  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      fill(minsGrays[x*8 + y]) ;
      rect(650 + (y * 15), 156 + (x * 15), 15, 15) ;
    }
  }
  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      fill(maxsGrays[x*8 + y]) ;
      rect(650 + (y * 15), 302 + (x * 15), 15, 15) ;
    }
  }
  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      fill(offsetGrays[x*8 + y]) ;
      rect(650 + (y * 15), 452 + (x * 15), 15, 15) ;
    }
  }
  for(int x = 0; x < 3; x++) {
    for(int y = 0; y < 3; y++) {
      int tempFill = (lights[x*3 + y] ? 1 : 0) * 255 + (lightsHover[x*3 + y] ? 1 : 0) * 50 ;
      if(tempFill > 255)
        tempFill = 205 ;
      fill(tempFill) ;
      rect(60 + (y * 25), 475 + (x * 25), 25, 25) ;
    }
  }
  
  fill(0) ;
  
  text("Frame: " + frameCounter, 400, 490) ;
  text("Raw", 710, 138) ;
  text("Minimums", 710, 284) ;
  text("Maximums", 710, 430) ;
  text("Offsets", 710, 582) ;
  text("Lights", 95, 560) ;
  
  text("Minimum: " + minimum, 100, 60) ;
  text("Maximum: " + maximum, 100, 100) ;
  text("Average: " + average, 100, 140) ;
  
  text("Frame time:\n" + frameTimeAverage + "ms", 100, 200) ;
  
  text("Max smoothing: " + smoothestOffsetMax, 97, 255) ;
  
  text("Avg best offset: " + smoothestAverageOffset, 95, 290) ;
  
  text("Calibration score: " + calibrationScore, 95, 325) ;
  
  if(bOverBright)
    fill(205) ;
  else
    fill(255) ;
  rect(147, 360, 40, 25) ;
  
  if(bOverContrast)
    fill(205) ;
  else
    fill(255) ;
  rect(142, 395, 40, 25) ;
  
  if(bOverGamma)
    fill(205) ;
  else
    fill(255) ;
  rect(139, 430, 40, 25) ;
  
 if(bOverOff)
    fill(205) ;
  else
    fill(255) ;
  rect(160, 500, 60, 25) ;
  
  if(bOverOn)
    fill(205) ;
  else
    fill(255) ;
  rect(160, 530, 60, 25) ;
  
  fill(0) ;
  
  text("Auto Brightness: ", 80, 370) ;
  if(bAutoBrightness)
    text("On", 168, 370) ;
  else
    text("Off", 165, 370) ;
  text("Auto Contrast: ", 80, 405) ;
  if(bAutoContrast)
    text("On", 163, 405) ;
  else
    text("Off", 160, 405) ;
  text("Auto Gamma: ", 80, 440) ;
  if(bAutoGamma)
    text("On", 160, 440) ;
  else
    text("Off", 157, 440) ;
  
  text("All off ", 190, 510) ;
  text("All on ", 190, 540) ;
}

void update(int x, int y) {
  for(int a = 0; a < 3; a++) {
    for(int b = 0; b < 3; b++) {
    lightsHover[a * 3 + b] = overRect(60 + (b * 25), 475 + (a * 25), 25, 25) ;
    }
  }
  
  bOverBright = overRect(147, 359, 40, 25) ;
  bOverContrast = overRect(142, 399, 40, 25) ;
  bOverGamma = overRect(139, 429, 40, 25) ;
  bOverOff = overRect(160, 500, 60, 25) ;
  bOverOn = overRect(160, 530, 60, 25) ;
}

void mousePressed() {
  bMouseClicked = true ;
}

void sendCommand() {
  for(int i = 0; i < 9; i++) {
    if(lightsHover[i])
      if(i < 8){
        if(lights[i])
          comPort.write("<CMD:LGX" + (7 - i) + ">") ;
        else
          comPort.write("<CMD:LGO" + (7 - i) + ">") ;
        comPort.write("\n") ;
        delay(5) ;
        bUpdateStats = true ;
        }
      else{
        if(lights[i])
          comPort.write("<CMD:LGX8>") ;
        else
          comPort.write("<CMD:LGO8>") ;
        comPort.write("\n") ;
        delay(5) ;
        bUpdateStats = true ;
      }
    }
  
  if(bOverOff) {
    comPort.write("<CMD:LGT0>") ;
    comPort.write("\n") ;
    delay(5) ;
    bUpdateStats = true ;
  } 
  else if(bOverOn) {
    comPort.write("<CMD:LGT1>") ;
    comPort.write("\n") ;
    delay(5) ;
    bUpdateStats = true ;
  } 
  else if(bOverBright) {
    if(bAutoBrightness)
      comPort.write("<CMD:ABR0>") ;
    else
      comPort.write("<CMD:ABR1>") ;
    comPort.write("\n") ;
    delay(5) ;
    bUpdateStats = true ;
  }
  else if(bOverContrast) {
    if(bAutoContrast)
      comPort.write("<CMD:ACN0>") ;
    else
      comPort.write("<CMD:ACN1>") ;
    comPort.write("\n") ;
    delay(5) ;
    bUpdateStats = true ;
  }
  else if(bOverGamma) {
    if(bAutoGamma)
      comPort.write("<CMD:AGM0>") ;
    else
      comPort.write("<CMD:AGM1>") ;
    comPort.write("\n") ;
    delay(5) ;
    bUpdateStats = true ;
  }
}

boolean overRect(int x, int y, int width, int height)  {
  if (mouseX >= x && mouseX <= x+width && 
      mouseY >= y && mouseY <= y+height) {
    return true;
  } else {
    return false;
  }
}

void grayscaleCompute() {
  for(int i = 0; i < 64; i++) {
    adjustedGrays[i] = adjFrame[i] / 16 ;
    rawGrays[i] = rawFrame[i] / 16 ;
    minsGrays[i] = minsFrame[i] / 16 ;
    maxsGrays[i] = maxsFrame[i] / 16 ;
    offsetGrays[i] = (offsetFrame[i] * (127 / smoothestOffsetMax)) + 128 ;
  }
}

void requestFrame() {
  if(bUpdateStats) {
    comPort.write("<CMD:RFM4>") ;
    bUpdateStats = false ;
    commandCounter-- ;
  }
  else if(commandCounter == 9)
    comPort.write("<CMD:RFM3>") ;
  else if(commandCounter == 19)
    comPort.write("<CMD:RFM2>") ;
  else if(commandCounter == 29)
    comPort.write("<CMD:RFM4>") ;
  else if(commandCounter == 39)
    comPort.write("<CMD:RFM5>") ;
  else if(commandCounter == 4 || commandCounter == 14 || commandCounter == 24 || commandCounter == 34)
    comPort.write("<CMD:RFM0>") ;
  else
    comPort.write("<CMD:RFM1>") ;
  comPort.write('\n') ;
  delay(5) ;
}

void packetTokenizer() {
  int x, y ;
  String frameType ;
  
  x = -1 ;
  y = -1 ;
  
  y = inBuffer.indexOf("[") ;
  if(y == -1)
    return ;
  inBuffer = inBuffer.substring(y+1) ;
  
  x = inBuffer.indexOf("FM") ;
  if(x == -1) {
    otherReply() ;
    return ;
  }
  frameType = inBuffer.substring(0, x) ;
  inBuffer = inBuffer.substring(x+2) ;
  
  if(frameType.compareTo("S") == 0) {
    
    y = -1 ;
    y = inBuffer.indexOf("I") ;
    if(y == -1) 
      return ;
    frameCounter = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("X") ;
    if(y == -1) 
      return ;
    minimum = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("V") ;
    if(y == -1) 
      return ;
    maximum = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("U") ;
    if(y == -1) 
      return ;
    average = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("I") ;
    if(y == -1) 
      return ;
    frameTimeAverage = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("S") ;
    if(y == -1) 
      return ;
    calibrationScore = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("M") ;
    if(y == -1) 
      return ;
    smoothestAverageOffset = (float)(Integer.parseInt(inBuffer.substring(0,y), 16) / 100) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("R") ;
    if(y == -1) 
      return ;
    smoothestOffsetMax = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("N") ;
    if(y == -1) 
      return ;
    bAutoBrightness = Integer.parseInt(inBuffer.substring(0,y)) > 0 ? true : false ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("G") ;
    if(y == -1) 
      return ;
    bAutoContrast = Integer.parseInt(inBuffer.substring(0,y)) > 0 ? true : false ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("L") ;
    if(y == -1) 
      return ;
    bAutoGamma = Integer.parseInt(inBuffer.substring(0,y)) > 0 ? true : false ;
    inBuffer = inBuffer.substring(y+1) ;
    
    y = -1 ;
    y = inBuffer.indexOf("]") ;
    if(y == -1) 
      return ;
    inBuffer = inBuffer.substring(0,y) ;
    if(inBuffer.length() < 9)
      return ;
    for(byte i = 0; i < 8; i++) {
      if(inBuffer.charAt(i) == '0')
        lights[7-i] = false ;
      else if(inBuffer.charAt(i) == '1')
        lights[7-i] = true ;
    }
    if(inBuffer.charAt(8) == '0')
      lights[8] = false ;
    else if(inBuffer.charAt(8) == '1')
      lights[8] = true ;
  }
  else if(frameType.compareTo("R") == 0 || frameType.compareTo("A") == 0 || frameType.compareTo("M") == 0 || frameType.compareTo("X") == 0  || frameType.compareTo("O") == 0) {
    
    y = inBuffer.indexOf(".") ;
    if(y == -1) 
      return ;
    frameCounter = Integer.parseInt(inBuffer.substring(0,y), 16) ;
    inBuffer = inBuffer.substring(y+1) ;
    
    for(int i = 0; i < 64; i++) {
      x = -1 ;
      y = -1 ; 
      x = inBuffer.indexOf(",") ;
      y = inBuffer.indexOf(".") ;
      if(y != -1 && x > y)
        x = y ;
      if(x == -1 && i == 63)
        x = inBuffer.indexOf("]") ;
      if(x == -1) 
        return ;
        
      input[i] = Integer.parseInt(inBuffer.substring(0,x), 16) ;
      inBuffer = inBuffer.substring(x+1) ;
    }
    if(frameType.compareTo("R") == 0) {
      for(byte i = 0; i < 64; i++)
        rawFrame[i] = input[i] ;
    }
    else if(frameType.compareTo("A") == 0) {
      for(byte i = 0; i < 64; i++)
        adjFrame[i] = input[i] ;
    }
    else if(frameType.compareTo("M") == 0) {
      for(byte i = 0; i < 64; i++)
        minsFrame[i] = input[i] ;
    }
    else if(frameType.compareTo("X") == 0) {
      for(byte i = 0; i < 64; i++)
        maxsFrame[i] = input[i] ;
    }
    else if(frameType.compareTo("O") == 0) {
      for(byte i = 0; i < 64; i++)
        offsetFrame[i] = input[i] ;
    }
  }
}

void otherReply() {
  int y = -1 ;
  y = inBuffer.indexOf("]") ;
  if(y == -1) 
    cmdReply = "Reply Error: " + inBuffer ;
  else {
    inBuffer = inBuffer.substring(0,y) ;
    cmdReply = inBuffer ;
  }
}