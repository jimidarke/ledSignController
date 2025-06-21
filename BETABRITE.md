# BetaBrite LED Sign Display Capabilities

This document provides a comprehensive reference for the BetaBrite LED sign display capabilities available through the custom BETABRITE library in this project.

## Overview

The BetaBrite library supports the Alpha Protocol for LED sign communication, providing extensive control over text display, colors, animations, and special effects. This enables rich, dynamic alert presentations that can be customized based on alert type, severity, and content.

## Display Modes

### Basic Display Modes
| Mode | Code | Description | Use Case |
|------|------|-------------|----------|
| Rotate | `a` | Cycles through text files | Default continuous display |
| Hold | `b` | Static display without movement | Important messages that need to stay visible |
| Flash | `c` | Text blinks on/off | Critical alerts requiring immediate attention |
| Scroll | `m` | Text scrolls horizontally | Long messages or continuous information |
| Auto Mode | `o` | Sign determines best display method | General purpose display |

### Motion Display Modes
| Mode | Code | Description | Visual Effect |
|------|------|-------------|---------------|
| Roll Up | `e` | Text rolls up from bottom | Smooth vertical transition |
| Roll Down | `f` | Text rolls down from top | Smooth vertical transition |
| Roll Left | `g` | Text rolls in from right | Horizontal sliding motion |
| Roll Right | `h` | Text rolls in from left | Horizontal sliding motion |
| Roll In | `p` | Text rolls in from edges | Converging motion effect |
| Roll Out | `q` | Text rolls out to edges | Diverging motion effect |

### Wipe Display Modes
| Mode | Code | Description | Visual Effect |
|------|------|-------------|---------------|
| Wipe Up | `i` | Text wipes upward | Clean vertical reveal |
| Wipe Down | `j` | Text wipes downward | Clean vertical reveal |
| Wipe Left | `k` | Text wipes leftward | Clean horizontal reveal |
| Wipe Right | `l` | Text wipes rightward | Clean horizontal reveal |
| Wipe In | `r` | Text wipes in from edges | Converging reveal |
| Wipe Out | `s` | Text wipes out to edges | Diverging reveal |

### Advanced Display Modes
| Mode | Code | Description | Best For |
|------|------|-------------|----------|
| Compressed Rotate | `t` | Compact rotation display | Space-efficient cycling |
| Explode | `u` | Text explodes outward | High-impact announcements |
| Clock | `v` | Time/date display mode | Timestamp information |
| Special | `n` | Enables special effects | Enhanced visual presentations |

## Special Effects (Used with Special Display Mode)

### Atmospheric Effects
| Effect | Code | Description | Mood/Theme |
|--------|------|-------------|------------|
| Twinkle | `0` | Stars twinkling effect | Gentle, pleasant |
| Sparkle | `1` | Sparkling lights effect | Celebratory, bright |
| Snow | `2` | Falling snow effect | Weather, winter theme |
| Spray | `6` | Water spray effect | Dynamic, energetic |
| Starburst | `7` | Explosive star effect | Dramatic, attention-grabbing |
| Fireworks | `X` | Fireworks explosion | Celebration, major events |

### Movement Effects
| Effect | Code | Description | Motion Type |
|--------|------|-------------|-------------|
| Interlock | `3` | Interlocking pattern | Mechanical, systematic |
| Switch | `4` | Switching pattern | Toggle, alternating |
| Slide | `5` | Sliding motion | Smooth, directional |
| Slots | `9` | Slot machine effect | Random, gaming |
| Turballoon | `Y` | Turbulent balloon | Floating, dynamic |

### Themed Effects
| Effect | Code | Description | Theme |
|--------|------|-------------|-------|
| Welcome | `8` | Welcome message effect | Greeting, hospitality |
| News Flash | `A` | Breaking news style | Urgent news, updates |
| Trumpet | `B` | Trumpet fanfare | Announcement, fanfare |
| Thank You | `S` | Thank you message | Gratitude, completion |
| No Smoking | `U` | No smoking symbol | Warning, prohibition |
| Don't Drink & Drive | `V` | Safety message | Safety, warning |
| Fishimal | `W` | Fish animation | Playful, aquatic |
| Bomb | `Z` | Explosion effect | Extreme urgency |

### Color Effects
| Effect | Code | Description | Usage |
|--------|------|-------------|-------|
| Cycle Colors | `C` | Rotates through colors | Dynamic, eye-catching |

## Colors

### Standard Colors
| Color | Code | Description | Best For |
|-------|------|-------------|----------|
| Red | `1` | Bright red | Critical alerts, emergencies |
| Green | `2` | Bright green | Success, normal status |
| Amber | `3` | Amber/orange | Warnings, caution |
| Yellow | `8` | Bright yellow | Attention, information |
| Orange | `7` | Orange | Moderate warnings |

### Dimmed Colors
| Color | Code | Description | Usage |
|-------|------|-------------|-------|
| Dim Red | `4` | Darker red | Subdued alerts |
| Dim Green | `5` | Darker green | Background status |
| Brown | `6` | Brown/dark amber | Neutral information |

### Dynamic Colors
| Color | Code | Description | Effect |
|-------|------|-------------|--------|
| Rainbow 1 | `9` | First rainbow pattern | Multi-color cycling |
| Rainbow 2 | `A` | Second rainbow pattern | Alternative rainbow |
| Color Mix | `B` | Mixed color pattern | Varied color display |
| Auto Color | `C` | Sign determines color | Automatic selection |

## Display Positions

| Position | Code | Description | Usage |
|----------|------|-------------|-------|
| Top Line | `"` | Upper portion of display | Headers, titles |
| Middle Line | ` ` | Center of display | Main content |
| Bottom Line | `&` | Lower portion of display | Status, footers |
| Fill | `0` | Full display area | Maximum visibility |
| Left | `1` | Left-aligned | Standard text alignment |
| Right | `2` | Right-aligned | Numbers, timestamps |

## Character Sets

### Standard Character Sets
| Set | Code | Description | Height | Style |
|-----|------|-------------|--------|-------|
| 5 High | `1` | Basic 5-pixel height | Small | Simple |
| 5 Stroke | `2` | 5-pixel stroke font | Small | Outlined |
| 7 High | `3` | Standard 7-pixel height | Medium | Clean |
| 7 Stroke | `4` | 7-pixel stroke font | Medium | Outlined |
| 7 High Fancy | `5` | Decorative 7-pixel | Medium | Stylized |
| 10 High | `6` | Large 10-pixel height | Large | Bold |

### Enhanced Character Sets
| Set | Code | Description | Style |
|-----|------|-------------|-------|
| 7 Shadow | `7` | Shadow effect | Dimensional |
| Full High Fancy | `8` | Full height decorative | Elegant |
| Full High | `9` | Full height standard | Maximum size |
| 7 Shadow Fancy | `:` | Shadow with decoration | Ornate |

### Wide Character Sets
| Set | Code | Description | Width |
|-----|------|-------------|-------|
| 5 Wide | `;` | 5-pixel wide | Compact wide |
| 7 Wide | `<` | 7-pixel wide | Standard wide |
| 7 Wide Fancy | `=` | Decorative wide | Stylized wide |
| 5 Wide Stroke | `>` | 5-pixel wide outline | Outlined wide |

## Text Formatting Controls

### Character Attributes
| Attribute | Code | Description | Effect |
|-----------|------|-------------|--------|
| Wide | `0` | Wide characters | Expanded width |
| Double Wide | `1` | Double width | Very wide text |
| Double High | `2` | Double height | Very tall text |
| True Descenders | `3` | Proper descenders | Better typography |
| Fixed Width | `4` | Monospace font | Aligned columns |
| Fancy | `5` | Decorative style | Ornate appearance |
| Shadow | `7` | Shadow effect | Dimensional look |

### Speed Controls
| Speed | Code | Description | Rate |
|-------|------|-------------|------|
| Speed 1 | `\025` | Slowest | Deliberate, readable |
| Speed 2 | `\026` | Slow | Comfortable reading |
| Speed 3 | `\027` | Medium | Standard pace |
| Speed 4 | `\030` | Fast | Quick updates |
| Speed 5 | `\031` | Fastest | Rapid attention |

### Special Formatting
| Control | Code | Description | Usage |
|---------|------|-------------|-------|
| New Line | `\015` | Line break | Multi-line messages |
| New Page | `\014` | Page break | Separate content |
| Character Flash | `\007` | Flashing text | Emphasis |
| No Hold Speed | `\011` | Continuous motion | Smooth animation |

## Alert Level Mapping Recommendations

### Critical Alerts
```
Mode: flash (c) or newsflash (A)
Special Effect: starburst (7) or bomb (Z)
Color: red (1)
Speed: 5 (fastest)
Character Set: 10 High (6) or 7 High (3)
Position: fill (0)
```

### Warning Alerts  
```
Mode: scroll (m) or special (n)
Special Effect: sparkle (1) or trumpet (B) 
Color: amber (3) or orange (7)
Speed: 3 (medium)
Character Set: 7 High (3)
Position: top line (")
```

### Info Alerts
```
Mode: rotate (a) or scroll (m)
Special Effect: twinkle (0) or welcome (8)
Color: green (2) or auto color (C)
Speed: 2 (slow)
Character Set: 7 High (3) or 5 High (1)
Position: middle line ( )
```

### Debug Alerts
```
Mode: hold (b)
Special Effect: none
Color: dim green (5)
Speed: 1 (slowest)
Character Set: 5 High (1)
Position: bottom line (&)
```

## Category-Specific Recommendations

### Security Alerts
- **Effects**: starburst, newsflash, bomb
- **Colors**: red, amber for severity
- **Mode**: flash, explode for urgency

### Weather Alerts  
- **Effects**: snow, spray, twinkle
- **Colors**: context-appropriate (blue/white for snow, etc.)
- **Mode**: scroll for continuous updates

### System Status
- **Effects**: sparkle, cycle colors
- **Colors**: green (good), amber (warning), red (error)
- **Mode**: rotate, hold for status

### Personal Reminders
- **Effects**: welcome, thank you
- **Colors**: auto color, rainbow
- **Mode**: gentle transitions (wipe, roll)

## Enhanced Message Format Examples

### Priority Alert (Full Featured)
```
[red,flash,starburst,10high,5,fill]SECURITY ALERT: Motion detected at front door
```

### Weather Update (Atmospheric)
```
[amber,scroll,snow,7high,2,topline]WEATHER: Snow expected tonight, 3-5 inches
```

### System Status (Informational)
```
[green,rotate,twinkle,7high,2,midline]SYSTEM: All services operating normally
```

### Emergency Alert (Maximum Impact)
```
[red,explode,bomb,10high,5,fill]EMERGENCY: Evacuate building immediately
```

## Programming Interface

### Basic Usage
```cpp
// Simple text with color and effect
sign.WriteTextFile('A', "[red,flash]Alert Message", BB_COL_RED, BB_DP_TOPLINE, BB_DM_FLASH, BB_SDM_STARBURST);

// Priority message (overrides normal rotation)
sign.WritePriorityTextFile("URGENT: Emergency Alert", BB_COL_RED, BB_DP_FILL, BB_DM_FLASH, BB_SDM_BOMB);
```

### Advanced Configuration
```cpp
// Configure memory for multiple text files
sign.SetMemoryConfiguration('A', 5, 256);  // Files A-E, 256 bytes each

// Multiple messages in sequence
sign.WriteTextFile('A', "[red,flash,starburst]Critical Alert");
sign.WriteTextFile('B', "[amber,scroll,sparkle]Warning Message"); 
sign.WriteTextFile('C', "[green,rotate,twinkle]Status Update");
```

## Integration with Alert Router

The alert router can map incoming alerts to appropriate BetaBrite display configurations:

```json
{
  "display_config": {
    "position": "fill",
    "mode": "flash", 
    "special_effect": "starburst",
    "color": "red",
    "character_set": "10high",
    "speed": 5,
    "priority": true,
    "duration": 30
  }
}
```

This comprehensive feature set allows for highly expressive and contextually appropriate alert displays that can convey not just the message content, but also the urgency, type, and emotional tone of each alert.