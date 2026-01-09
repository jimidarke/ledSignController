/**
 * @file bbdefs.h
 * @brief BetaBrite Alpha Protocol definitions for ESPHome
 *
 * Protocol constants for BetaBrite/Alpha LED signs.
 * Independent implementation based on publicly available
 * Alpha-American Protocol documentation.
 *
 * MIT License - see LICENSE file.
 */

#pragma once

namespace esphome {
namespace betabrite {

// Common ASCII character definitions used by the protocol
static const char BB_NUL = '\000';
static const char BB_SOH = '\001';
static const char BB_STX = '\002';
static const char BB_ETX = '\003';
static const char BB_EOT = '\004';
static const char BB_ESC = '\033';

// Sign Types
enum SignType : char {
  ST_ALL_VV = '\041',
  ST_SER_CLK = '\042',
  ST_ALPHA_VISION = '\043',
  ST_ALPHA_VISION_FM = '\044',
  ST_ALPHA_VISION_CM = '\045',
  ST_ALPHA_VISION_LM = '\046',
  ST_RESPONSE = '\060',
  ST_1LINE = '\061',
  ST_2LINE = '\062',
  ST_ALL = '\077',
  ST_430I = '\103',
  ST_440I = '\104',
  ST_460I = '\105',
  ST_ALPHA_ECLIPSE_3600_DDB = '\106',
  ST_ALPHA_ECLIPSE_3600_TAB = '\107',
  ST_LIGHT_SENSOR = '\114',
  ST_790I = '\125',
  ST_ALPHA_ECLIPSE_3600 = '\126',
  ST_ALPHA_ECLIPSE_TIME_TEMP = '\127',
  ST_ALPHA_PREMIERE = '\130',
  ST_ALL2 = '\132',
  ST_BETABRITE = '\136',
  ST_4120C = '\141',
  ST_4160C = '\142',
  ST_4200C = '\143',
  ST_4240C = '\144',
  ST_215R = '\145',
  ST_215C = '\146',
  ST_4120R = '\147',
  ST_4160R = '\150',
  ST_4200R = '\151',
  ST_4240R = '\152',
  ST_300S = '\153',
  ST_7000S = '\154',
  ST_9616MS = '\155',
  ST_12816MS = '\156',
  ST_16016MS = '\157',
  ST_19216MS = '\160',
  ST_PPD = '\161',
  ST_DIRECTOR = '\162',
  ST_1005DC = '\163',
  ST_4080C = '\164',
  ST_210C_220C = '\165',
  ST_ALPHA_ECLIPSE_3500 = '\166',
  ST_ALPHA_ECLIPSE_TT = '\167',
  ST_ALPHA_PREMIERE_9000 = '\170',
  ST_TEMP_PROBE = '\171',
  ST_ALL_AZ = '\172',
};

// Command codes
enum CommandCode : char {
  CC_WTEXT = 'A',     // Write Text File
  CC_RTEXT = 'B',     // Read Text File
  CC_WSPFUNC = 'E',   // Write Special Function
  CC_RSPFUNC = 'F',   // Read Special Function
  CC_WSTRING = 'G',   // Write String File
  CC_RSTRING = 'H',   // Read String File
  CC_WSDOTS = 'I',    // Write Small Dots Picture
  CC_RSDOTS = 'J',    // Read Small Dots Picture
  CC_WRGBDOTS = 'K',  // Write RGB Dots Picture
  CC_RRGBDOTS = 'L',  // Read RGB Dots Picture
  CC_WLDOTS = 'M',    // Write Large Dots Picture
  CC_RLDOTS = 'N',    // Read Large Dots Picture
  CC_WBULL = 'O',     // Write Bulletin
  CC_SETTO = 'T',     // Set Timeout
};

// Display Positions
enum DisplayPosition : char {
  DP_MIDLINE = '\040',
  DP_TOPLINE = '\042',
  DP_BOTLINE = '\046',
  DP_FILL = '\060',
  DP_LEFT = '\061',
  DP_RIGHT = '\062',
};

// Display Modes
enum DisplayMode : char {
  DM_ROTATE = 'a',
  DM_HOLD = 'b',
  DM_FLASH = 'c',
  DM_ROLLUP = 'e',
  DM_ROLLDOWN = 'f',
  DM_ROLLLEFT = 'g',
  DM_ROLLRIGHT = 'h',
  DM_WIPEUP = 'i',
  DM_WIPEDOWN = 'j',
  DM_WIPELEFT = 'k',
  DM_WIPERIGHT = 'l',
  DM_SCROLL = 'm',
  DM_SPECIAL = 'n',
  DM_AUTOMODE = 'o',
  DM_ROLLIN = 'p',
  DM_ROLLOUT = 'q',
  DM_WIPEIN = 'r',
  DM_WIPEOUT = 's',
  DM_COMPROTATE = 't',
  DM_EXPLODE = 'u',
  DM_CLOCK = 'v',
};

// Special Display Modes (Effects)
enum SpecialMode : char {
  SDM_TWINKLE = '0',
  SDM_SPARKLE = '1',
  SDM_SNOW = '2',
  SDM_INTERLOCK = '3',
  SDM_SWITCH = '4',
  SDM_SLIDE = '5',
  SDM_SPRAY = '6',
  SDM_STARBURST = '7',
  SDM_WELCOME = '8',
  SDM_SLOTS = '9',
  SDM_NEWSFLASH = 'A',
  SDM_TRUMPET = 'B',
  SDM_CYCLECOLORS = 'C',
  SDM_THANKYOU = 'S',
  SDM_NOSMOKING = 'U',
  SDM_DONTDRINKANDDRIVE = 'V',
  SDM_FISHIMAL = 'W',
  SDM_FIREWORKS = 'X',
  SDM_TURBALLOON = 'Y',
  SDM_BOMB = 'Z',
};

// Text file or string formatting characters/commands
enum FormatCode : char {
  FC_DOUBLEHIGH = '\005',
  FC_TRUEDESCENDERS = '\006',
  FC_CHARFLASH = '\007',
  FC_EXTENDEDCHARSET = '\010',
  FC_NOHOLDSPEED = '\011',
  FC_CALLDATE = '\013',
  FC_NEWPAGE = '\014',
  FC_NEWLINE = '\015',
  FC_SPEEDCONTROL = '\017',
  FC_CALLSTRING = '\020',
  FC_DISABLEWIDECHAR = '\021',
  FC_ENABLEWIDECHAR = '\022',
  FC_CALLTIME = '\023',
  FC_CALLSDOTS = '\024',
  FC_SPEED1 = '\025',
  FC_SPEED2 = '\026',
  FC_SPEED3 = '\027',
  FC_SPEED4 = '\030',
  FC_SPEED5 = '\031',
  FC_SELECTCHARSET = '\032',
  FC_SELECTCHARCOLOR = '\034',
  FC_SELECTCHARATTR = '\035',
  FC_SELECTCHARSPACE = '\036',
  FC_CALLPICTURE = '\037',
};

// Character Sets
enum CharSet : char {
  CS_5HIGH = '1',
  CS_5STROKE = '2',
  CS_7HIGH = '3',
  CS_7STROKE = '4',
  CS_7HIGHFANCY = '5',
  CS_10HIGH = '6',
  CS_7SHADOW = '7',
  CS_FHIGHFANCY = '8',
  CS_FHIGH = '9',
  CS_7SHADOWFANCY = ':',
  CS_5WIDE = ';',
  CS_7WIDE = '<',
  CS_7WIDEFANCY = '=',
  CS_5WIDESTROKE = '>',
  CS_5HIGHCUSTOM = 'W',
  CS_7HIGHCUSTOM = 'X',
  CS_10HIGHCUSTOM = 'Y',
  CS_15HIGHCUSTOM = 'Z',
};

// Character Colors
enum CharColor : char {
  COL_RED = '1',
  COL_GREEN = '2',
  COL_AMBER = '3',
  COL_DIMRED = '4',
  COL_DIMGREEN = '5',
  COL_BROWN = '6',
  COL_ORANGE = '7',
  COL_YELLOW = '8',
  COL_RAINBOW1 = '9',
  COL_RAINBOW2 = 'A',
  COL_COLORMIX = 'B',
  COL_AUTOCOLOR = 'C',
};

// Character Attributes
enum CharAttribute : char {
  CA_WIDE = '0',
  CA_DOUBLEWIDE = '1',
  CA_DOUBLEHIGH = '2',
  CA_TRUEDESCENDERS = '3',
  CA_FIXEDWIDTH = '4',
  CA_FANCY = '5',
  CA_AUXPORT = '6',
  CA_SHADOW = '7',
};

// Attribute (and other) Switch
enum AttrSwitch : char {
  ATTR_OFF = '0',
  ATTR_ON = '1',
};

// Picture Types
enum PictureType : char {
  PT_QUICKFLICK = 'C',
  PT_FASTERFLICKS = 'G',
  PT_DOTSPICTURE = 'L',
};

// Date Formats
enum DateFormat : char {
  DF_MMDDYYSLASH = '0',
  DF_DDMMYYSLASH = '1',
  DF_MMDDYYHYPHEN = '2',
  DF_DDMMYYHYPHEN = '3',
  DF_MMDDYYPERIOD = '4',
  DF_DDMMYYPERIOD = '5',
  DF_MMDDYYSPACE = '6',
  DF_DDMMYYSPACE = '7',
  DF_MMMDDYYYY = '8',
  DF_DAYOFWEEK = '9',
};

// Temperature Format
enum TempFormat : char {
  TF_CELSIUS = '\034',
  TF_FAHRENHEIT = '\035',
};

// Character Spacing
enum CharSpacing : char {
  SP_PROPORTIONAL = '0',
  SP_FIXEDWIDTH = '1',
};

// Special Function Labels
static const char SFL_CLEARMEM = '$';

// Special Function File Types
enum FileType : char {
  SFFT_TEXT = 'A',
  SFFT_STRING = 'B',
  SFFT_DOTS = 'D',
};

// Special Function Keyboard Protection Status
enum KeyboardProtection : char {
  SFKPS_LOCKED = 'L',
  SFKPS_UNLOCKED = 'U',
};

// Priority file label
static const char PRIORITY_FILE_LABEL = '0';

// Timing constants
static const uint32_t BETWEEN_COMMAND_DELAY_MS = 110;
static const uint32_t DEFAULT_BAUD_RATE = 9600;

// Helper functions for string to enum conversion
inline CharColor color_from_string(const std::string &color) {
  if (color == "red") return COL_RED;
  if (color == "green") return COL_GREEN;
  if (color == "amber") return COL_AMBER;
  if (color == "dimred") return COL_DIMRED;
  if (color == "dimgreen") return COL_DIMGREEN;
  if (color == "brown") return COL_BROWN;
  if (color == "orange") return COL_ORANGE;
  if (color == "yellow") return COL_YELLOW;
  if (color == "rainbow1") return COL_RAINBOW1;
  if (color == "rainbow2") return COL_RAINBOW2;
  if (color == "colormix") return COL_COLORMIX;
  if (color == "autocolor") return COL_AUTOCOLOR;
  return COL_GREEN;  // default
}

inline DisplayMode mode_from_string(const std::string &mode) {
  if (mode == "rotate") return DM_ROTATE;
  if (mode == "hold") return DM_HOLD;
  if (mode == "flash") return DM_FLASH;
  if (mode == "rollup") return DM_ROLLUP;
  if (mode == "rolldown") return DM_ROLLDOWN;
  if (mode == "rollleft") return DM_ROLLLEFT;
  if (mode == "rollright") return DM_ROLLRIGHT;
  if (mode == "wipeup") return DM_WIPEUP;
  if (mode == "wipedown") return DM_WIPEDOWN;
  if (mode == "wipeleft") return DM_WIPELEFT;
  if (mode == "wiperight") return DM_WIPERIGHT;
  if (mode == "scroll") return DM_SCROLL;
  if (mode == "special") return DM_SPECIAL;
  if (mode == "automode") return DM_AUTOMODE;
  if (mode == "rollin") return DM_ROLLIN;
  if (mode == "rollout") return DM_ROLLOUT;
  if (mode == "wipein") return DM_WIPEIN;
  if (mode == "wipeout") return DM_WIPEOUT;
  if (mode == "comprotate") return DM_COMPROTATE;
  if (mode == "explode") return DM_EXPLODE;
  if (mode == "clock") return DM_CLOCK;
  return DM_ROTATE;  // default
}

inline SpecialMode effect_from_string(const std::string &effect) {
  if (effect == "twinkle") return SDM_TWINKLE;
  if (effect == "sparkle") return SDM_SPARKLE;
  if (effect == "snow") return SDM_SNOW;
  if (effect == "interlock") return SDM_INTERLOCK;
  if (effect == "switch") return SDM_SWITCH;
  if (effect == "slide") return SDM_SLIDE;
  if (effect == "spray") return SDM_SPRAY;
  if (effect == "starburst") return SDM_STARBURST;
  if (effect == "welcome") return SDM_WELCOME;
  if (effect == "slots") return SDM_SLOTS;
  if (effect == "newsflash") return SDM_NEWSFLASH;
  if (effect == "trumpet") return SDM_TRUMPET;
  if (effect == "cyclecolors") return SDM_CYCLECOLORS;
  if (effect == "thankyou") return SDM_THANKYOU;
  if (effect == "nosmoking") return SDM_NOSMOKING;
  if (effect == "dontdrinkanddrive") return SDM_DONTDRINKANDDRIVE;
  if (effect == "fishimal") return SDM_FISHIMAL;
  if (effect == "fireworks") return SDM_FIREWORKS;
  if (effect == "turballoon") return SDM_TURBALLOON;
  if (effect == "bomb") return SDM_BOMB;
  return SDM_TWINKLE;  // default
}

inline CharSet charset_from_string(const std::string &charset) {
  if (charset == "5high") return CS_5HIGH;
  if (charset == "5stroke") return CS_5STROKE;
  if (charset == "7high") return CS_7HIGH;
  if (charset == "7stroke") return CS_7STROKE;
  if (charset == "7highfancy") return CS_7HIGHFANCY;
  if (charset == "10high") return CS_10HIGH;
  if (charset == "7shadow") return CS_7SHADOW;
  if (charset == "fhighfancy") return CS_FHIGHFANCY;
  if (charset == "fhigh") return CS_FHIGH;
  if (charset == "7shadowfancy") return CS_7SHADOWFANCY;
  if (charset == "5wide") return CS_5WIDE;
  if (charset == "7wide") return CS_7WIDE;
  if (charset == "7widefancy") return CS_7WIDEFANCY;
  if (charset == "5widestroke") return CS_5WIDESTROKE;
  return CS_7HIGH;  // default
}

inline DisplayPosition position_from_string(const std::string &position) {
  if (position == "midline") return DP_MIDLINE;
  if (position == "topline") return DP_TOPLINE;
  if (position == "botline") return DP_BOTLINE;
  if (position == "fill") return DP_FILL;
  if (position == "left") return DP_LEFT;
  if (position == "right") return DP_RIGHT;
  return DP_TOPLINE;  // default
}

inline char speed_code_from_int(int speed) {
  switch (speed) {
    case 1: return FC_SPEED1;
    case 2: return FC_SPEED2;
    case 3: return FC_SPEED3;
    case 4: return FC_SPEED4;
    case 5: return FC_SPEED5;
    default: return FC_SPEED3;
  }
}

}  // namespace betabrite
}  // namespace esphome
