/**
*  Polargraph Server. - CORE
*  Written by Sandy Noble
*  Released under GNU License version 3.
*  http://www.polargraph.co.uk
*  https://github.com/euphy/polargraph_server_a1

Comms.

This is one of the core files for the polargraph server program.  
Comms can mean "communications" or "commands", either will do, since
it contains methods for reading commands from the serial port.

*/

boolean comms_waitForNextCommand(char *buf)
{
  // send ready
  // wait for instruction
  long idleTime = millis();
  int bufPos = 0;
  for (int i = 0; i<INLENGTH; i++) {
    buf[i] = 0;
  }
  long lastRxTime = 0L;
  // loop while there's there isn't a terminated command.
  // (Note this might mean characters ARE arriving, but just
  //  that the command hasn't been finished yet.)
  boolean terminated = false;
  while (!terminated)
  {
		COMMS_DEBUG_PRINTLN(F("."));
    long timeSince = millis() - lastRxTime;
    
    // If the buffer is being filled, but hasn't received a new char in less than 100ms,
    // just cancel it. It's probably just junk.
    if (bufPos != 0 && timeSince > 100)
    {
      COMMS_DEBUG_PRINT("Timed out:");
      COMMS_DEBUG_PRINTLN(timeSince);
      // Clear the buffer and reset the position if it took too long
      for (int i = 0; i<INLENGTH; i++) {
        buf[i] = 0;
      }
      bufPos = 0;
    }
    
    // idle time is mostly spent in this loop.
    impl_runBackgroundProcesses();
    timeSince = millis() - idleTime;
    if (timeSince > rebroadcastReadyInterval)
    {
      // issue a READY every 5000ms of idling
			COMMS_DEBUG_PRINTLN("");
      comms_ready();
      idleTime = millis();
    }
    
    // And now read the command if one exists.
    if (Serial.available() > 0)
    {
      // Get the char
      char ch = Serial.read();
      COMMS_DEBUG_PRINT("ch: ");
      COMMS_DEBUG_PRINTLN(ch);
      
      // look at it, if it's a terminator, then lets terminate the string
      if (ch == INTERMINATOR || ch == SEMICOLON) {
        buf[bufPos] = 0; // null terminate the string
        terminated = true;
				COMMS_DEBUG_PRINT(F("Term'd"));
        for (int i = bufPos; i<INLENGTH-1; i++) {
          buf[i] = 0;
        }
        
      } else {
        // otherwise, just add it into the buffer
        buf[bufPos] = ch;
        bufPos++;
      }
			COMMS_DEBUG_PRINT(F("buf: "));
			COMMS_DEBUG_PRINTLN(buf);
			COMMS_DEBUG_PRINT(F("Bufpos: "));
			COMMS_DEBUG_PRINTLN(bufPos);
      lastRxTime = millis();
    }
  }

  idleTime = millis();
  lastOperationTime = millis();
  lastInteractionTime = lastOperationTime;
	COMMS_DEBUG_PRINT(F("xbuf: "));
	COMMS_DEBUG_PRINTLN(buf);
  return true;
}

void comms_parseAndExecuteCommand(char *inS)
{
	COMMS_DEBUG_PRINT("3inS: ");
	COMMS_DEBUG_PRINTLN(inS);

  boolean commandParsed = comms_parseCommand(inS);
  if (commandParsed)
  {
    impl_processCommand(lastCommand);
    for (int i = 0; i<INLENGTH; i++) { inS[i] = 0; }  
    commandConfirmed = false;
    comms_ready();
  }
  else
  {
    Serial.print(MSG_E_STR);
    Serial.print(F("Comm ("));
    Serial.print(inS);
    Serial.println(F(") not parsed."));
  }
  inNoOfParams = 0;
  
}

boolean comms_parseCommand(char *inS)
{

	COMMS_DEBUG_PRINT(F("1inS: "));
	COMMS_DEBUG_PRINTLN(inS);
  // strstr returns a pointer to the location of ",END" in the incoming string (inS).
  char* sub = strstr(inS, CMD_END);
	COMMS_DEBUG_PRINT(F("2inS: "));
	COMMS_DEBUG_PRINTLN(inS);
  sub[strlen(CMD_END)] = 0; // null terminate it directly after the ",END"
	COMMS_DEBUG_PRINT(F("4inS: "));
	COMMS_DEBUG_PRINTLN(inS);
	COMMS_DEBUG_PRINT(F("2Sub: "));
	COMMS_DEBUG_PRINTLN(sub);
	COMMS_DEBUG_PRINTLN(strcmp(sub, CMD_END));
  if (strcmp(sub, CMD_END) == 0) 
  {
    comms_extractParams(inS);
    return true;
  }
  else
    return false;
}  

void comms_extractParams(char* inS) 
{
  char in[strlen(inS)];
  strcpy(in, inS);
  char * param;
  
	COMMS_DEBUG_PRINT(F("In: "));
	COMMS_DEBUG_PRINT(in);
	COMMS_DEBUG_PRINTLN("...");
  byte paramNumber = 0;
  param = strtok(in, COMMA);
  
  inParam1[0] = 0;
  inParam2[0] = 0;
  inParam3[0] = 0;
  inParam4[0] = 0;
  
  for (byte i=0; i<6; i++) {
      if (i == 0) {
        strcpy(inCmd, param);
      }
      else {
        param = strtok(NULL, COMMA);
        if (param != NULL) {
          if (strstr(CMD_END, param) == NULL) {
            // It's not null AND it wasn't 'END' either
            paramNumber++;
          }
        }
        
        switch(i)
        {
          case 1:
            if (param != NULL) strcpy(inParam1, param);
            break;
          case 2:
            if (param != NULL) strcpy(inParam2, param);
            break;
          case 3:
            if (param != NULL) strcpy(inParam3, param);
            break;
          case 4:
            if (param != NULL) strcpy(inParam4, param);
            break;
          default:
            break;
        }
      }
			COMMS_DEBUG_PRINT(F("P: "));
			COMMS_DEBUG_PRINT(i);
			COMMS_DEBUG_PRINT(F("-"));
			COMMS_DEBUG_PRINT(paramNumber);
			COMMS_DEBUG_PRINT(F(":"));
			COMMS_DEBUG_PRINTLN(param);
  }

  inNoOfParams = paramNumber;
	#ifdef USE_LCD
  if (echoingStoredCommands)
     {
    lcd.setCursor(0,0); lcd.print(cleanline);  
    lcd.setCursor(0,0); lcd.print(inCmd);   lcd.print(" "); 
    if ((paramNumber>1) && strlen(inParam1)<10) {lcd.setCursor(4,0); lcd.print(inParam1); } 
    if (paramNumber>2) {lcd.print(","); lcd.print(inParam2); }
//   if (paramNumber>3) {lcd.print(","); lcd.print(inParam3); }
//   lcd.print("("); lcd.print(paramNumber); lcd.print(")");
    }
#endif //USE_LCD
	COMMS_DEBUG_PRINT(F("Command:"));
	COMMS_DEBUG_PRINT(inCmd);
	COMMS_DEBUG_PRINT(F(", p1:"));
	COMMS_DEBUG_PRINT(inParam1);
	COMMS_DEBUG_PRINT(F(", p2:"));
	COMMS_DEBUG_PRINT(inParam2);
	COMMS_DEBUG_PRINT(F(", p3:"));
	COMMS_DEBUG_PRINT(inParam3);
	COMMS_DEBUG_PRINT(F(", p4:"));
	COMMS_DEBUG_PRINTLN(inParam4);
	COMMS_DEBUG_PRINT(F("Params:"));
	COMMS_DEBUG_PRINTLN(inNoOfParams);
}


void comms_ready()
{
  Serial.println(READY_STR);
#ifdef DEBUG_STATE
  Serial.print(MSG_D_STR);
 #endif
}

void comms_drawing()
{
  Serial.println(DRAWING_STR);
}
void comms_requestResend()
{
  Serial.println(RESEND_STR);
}
void comms_unrecognisedCommand(String &com)
{
  Serial.print(MSG_E_STR);
  Serial.print(com);
  Serial.println(F(" not recognised."));
}  




