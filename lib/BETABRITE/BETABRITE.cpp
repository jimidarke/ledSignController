/* BETABRITE class for Arduino
   by Tom Stewart (tastewar.com)

   This library provides definitions and functions
   for interfacing with a BetaBrite sign through a
   Serial port

   The documentation for the protocol that this code
   is based on is (was) available at the time at:
   http://www.alpha-american.com/alpha-manuals/M-Protocol.pdf
   (c) 2011
*/
// include <WProgram.h>
#ifdef DATEFUNCTIONS
#include <time.h>
#endif
#include "BETABRITE.h"
#define BB_BETWEEN_COMMAND_DELAY 110

//BETABRITE::BETABRITE ( uint8_t receivePin, uint8_t transmitPin, const char Type, const char Address[2] ) : SoftwareSerial ( receivePin, transmitPin ) {
BETABRITE::BETABRITE ( uint8_t uart_num, uint8_t receivePin, uint8_t transmitPin, const char Type, const char Address[2] ) : HardwareSerial(uart_num)  // Initialize HardwareSerial with UART number
{
  //begin ( 9600 );
  this->begin(9600, SERIAL_7E1, receivePin, transmitPin);  // Set baud rate and pins
  this->_type = Type;
  if ( Address )
  {
    this->_address[0] = Address[0];
    this->_address[1] = Address[1];
  }
  else
  {
    this->_address[0] = '0';
    this->_address[1] = '0';
  }
}

BETABRITE::~BETABRITE ( void )
{
}

void BETABRITE::WriteTextFile ( const char Name, const char *Contents, const char initColor, const char Position, const char Mode, const char Special )
{
  BeginCommand ( );
  BeginNestedCommand ( );
  WriteTextFileNested ( Name, Contents, initColor, Position, Mode, Special );
  EndCommand ( );
}

void BETABRITE::WriteTextFileNested ( const char Name, const char *Contents, const char initColor, const char Position, const char Mode, const char Special )
{
  print ( BB_CC_WTEXT );
  print ( Name );
  print ( BB_ESC ); print ( Position ); print ( Mode );
  if ( BB_DM_SPECIAL == Mode ) print ( Special );
  if ( initColor != BB_COL_AUTOCOLOR )
  {
    print ( BB_FC_SELECTCHARCOLOR );
    print ( initColor );
  }
  print ( (char *)Contents );
}

void BETABRITE::WritePriorityTextFile ( const char *Contents, const char initColor, const char Position, const char Mode, const char Special )
{
  WriteTextFile ( BB_PRIORITY_FILE_LABEL, Contents, initColor, Position, Mode, Special );
}

void BETABRITE::WritePriorityTextFileNested ( const char *Contents, const char initColor, const char Position, const char Mode, const char Special )
{
  WriteTextFileNested ( BB_PRIORITY_FILE_LABEL, Contents, initColor, Position, Mode, Special );
}

void BETABRITE::CancelPriorityTextFile ( void )
{
  BeginCommand ( );
  BeginNestedCommand ( );
  print ( BB_CC_WTEXT );
  print ( BB_PRIORITY_FILE_LABEL );
  EndCommand ( );
}

void BETABRITE::WriteStringFile ( const char Name, const char *Contents )
{
  BeginCommand ( );
  BeginNestedCommand ( );
  WriteStringFileNested ( Name, Contents );
  EndCommand ( );
}

void BETABRITE::WriteStringFileNested ( const char Name, const char *Contents )
{
  print ( BB_CC_WSTRING );
  print ( Name );
  print ( (char *)Contents );
}

void BETABRITE::SetMemoryConfiguration ( const char startingFile, unsigned int numFiles, unsigned int size )
{
  BeginCommand ( );
  BeginNestedCommand ( );
  print ( BB_CC_WSPFUNC );
  print ( BB_SFL_CLEARMEM );

  char sizeBuf[5] = "0100";
  if (size <= 0xffff)
  {
    sprintf(sizeBuf, "%04x", size);
  }

  for (char c = startingFile; c <  startingFile + numFiles; c++)
  {
    print ( c );
    print ( BB_SFFT_TEXT );
    print ( BB_SFKPS_LOCKED );
    print ( sizeBuf );
    print ( "FF00" );    // AlwaysOn for text file
  }

  EndCommand ( );
}

void BETABRITE::BeginCommand ( void )
{
  Sync ( ); print ( BB_SOH ); print ( _type ); print ( _address[0] ); print ( _address[1] );
}

void BETABRITE::BeginNestedCommand ( void )
{
  print ( BB_STX );
}

void BETABRITE::EndCommand ( void )
{
  print ( BB_EOT );
}

void BETABRITE::EndNestedCommand ( void )
{
  print ( BB_ETX );
}

void BETABRITE::DelayBetweenCommands ( void )
{
  delay ( BB_BETWEEN_COMMAND_DELAY );
}

#ifdef DATEFUNCTIONS
void BETABRITE::SetDateTime ( DateTime now, bool UseMilitaryTime )
{
  char		dow, strbuff[3];
  uint8_t		hour, minute, month, day;
  uint16_t	year;

  dow = now.dayOfWeek ( );
  dow += '1';
  hour = now.hour ( );
  minute = now.minute ( );
  month = now.month ( );
  day = now.day ( );
  year = now.year ( );

  BeginCommand ( );
  BeginNestedCommand ( );
  DelayBetweenCommands ( );
  print ( BB_CC_WSPFUNC );
  print ( ' ' );
  if ( hour <= 9 )
  {
    print ( '0' );
  }
  itoa ( hour, strbuff, 10 );
  print ( strbuff );
  if ( minute <= 9 )
  {
    print ( '0' );
  }
  itoa ( minute, strbuff, 10 );
  print ( strbuff );
  EndNestedCommand ( );
  BeginNestedCommand ( );
  DelayBetweenCommands ( );
  print ( BB_CC_WSPFUNC );
  print ( '\047' );
  print ( UseMilitaryTime ? 'M' : 'S' );
  EndNestedCommand ( );
  BeginNestedCommand ( );
  DelayBetweenCommands ( );
  print ( BB_CC_WSPFUNC );
  print ( '&' );
  print ( dow );
  EndNestedCommand ( );
  BeginNestedCommand ( );
  DelayBetweenCommands ( );
  print ( BB_CC_WSPFUNC );
  print ( ';' );
  if ( month <= 9 )
  {
    print ( '0' );
  }
  itoa ( month, strbuff, 10 );
  print ( strbuff );
  if ( day <= 9 )
  {
    print ( '0' );
  }
  itoa ( day, strbuff, 10 );
  print ( strbuff );
  if ( year < 2000 ) print ( "00" );
  else
  {
    year -= 2000;
    if ( year <= 9 )
    {
      print ( '0' );
    }
    itoa ( year, strbuff, 10 ); // 2 digits for the foreseeable future, and positive
    print ( strbuff );
  }
  EndNestedCommand ( );
  EndCommand ( );
}
#endif

int BETABRITE::ReadTextFile ( const char Name, char *buffer, size_t bufferSize, unsigned long timeoutMs )
{
  // Flush any stale data in receive buffer
  while ( this->available() ) this->read();

  BeginCommand();
  BeginNestedCommand();
  print ( BB_CC_RTEXT );
  print ( Name );
  EndCommand();

  DelayBetweenCommands();
  return ReadResponse ( buffer, bufferSize, timeoutMs );
}

int BETABRITE::ReadSpecialFunction ( const char Label, char *buffer, size_t bufferSize, unsigned long timeoutMs )
{
  while ( this->available() ) this->read();

  BeginCommand();
  BeginNestedCommand();
  print ( BB_CC_RSPFUNC );
  print ( Label );
  EndCommand();

  DelayBetweenCommands();
  return ReadResponse ( buffer, bufferSize, timeoutMs );
}

int BETABRITE::ReadStringFile ( const char Name, char *buffer, size_t bufferSize, unsigned long timeoutMs )
{
  while ( this->available() ) this->read();

  BeginCommand();
  BeginNestedCommand();
  print ( BB_CC_RSTRING );
  print ( Name );
  EndCommand();

  DelayBetweenCommands();
  return ReadResponse ( buffer, bufferSize, timeoutMs );
}

bool BETABRITE::PingSign ( unsigned long timeoutMs )
{
  char buf[64];
  int result = ReadSpecialFunction ( ' ', buf, sizeof(buf), timeoutMs );
  return ( result > 0 );
}

int BETABRITE::ReadResponse ( char *buffer, size_t bufferSize, unsigned long timeoutMs )
{
  unsigned long startTime = millis();

  // Wait for first byte (SOH) with timeout
  while ( !this->available() )
  {
    if ( millis() - startTime > timeoutMs ) return -1;
    delay ( 1 );
  }

  // Scan for SOH byte
  bool foundSOH = false;
  while ( millis() - startTime < timeoutMs )
  {
    if ( this->available() )
    {
      int b = this->read();
      if ( b == BB_SOH )
      {
        foundSOH = true;
        break;
      }
    }
    else
    {
      delay ( 1 );
    }
  }

  if ( !foundSOH ) return -1;

  // Read type byte and 2-byte address (skip them)
  for ( int i = 0; i < 3; i++ )
  {
    unsigned long byteStart = millis();
    while ( !this->available() )
    {
      if ( millis() - byteStart > 200 ) return -1;
      delay ( 1 );
    }
    this->read();
  }

  // Wait for STX
  bool foundSTX = false;
  while ( millis() - startTime < timeoutMs )
  {
    if ( this->available() )
    {
      int b = this->read();
      if ( b == BB_STX )
      {
        foundSTX = true;
        break;
      }
    }
    else
    {
      delay ( 1 );
    }
  }

  if ( !foundSTX ) return -1;

  // Read payload bytes until ETX or EOT
  size_t count = 0;
  while ( millis() - startTime < timeoutMs )
  {
    if ( this->available() )
    {
      int b = this->read();
      if ( b == BB_ETX || b == BB_EOT ) break;
      if ( count < bufferSize - 1 )
      {
        buffer[count++] = (char)b;
      }
    }
    else
    {
      // Short inter-byte timeout
      delay ( 1 );
      unsigned long idleStart = millis();
      while ( !this->available() && millis() - idleStart < 200 )
      {
        delay ( 1 );
      }
      if ( !this->available() ) break; // Timed out between bytes
    }
  }

  buffer[count] = '\0'; // Null-terminate
  return (int)count;
}

void BETABRITE::Sync ( void )
{
  for ( char i = 0; i < 5; i++ ) print ( BB_NUL );
}
