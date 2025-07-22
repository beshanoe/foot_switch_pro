#include <Bounce.h>
#include <Wire.h>
#include <I2C_eeprom.h>
#include <LiquidCrystal_I2C.h>

// Adjust to your EEPROM's I2C address and size
#define EEPROM_I2C_ADDRESS 0x50
#define EEPROM_SIZE 262144  // 256KB
I2C_eeprom eeprom(EEPROM_I2C_ADDRESS, EEPROM_SIZE);

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed
unsigned long lcdClearTime = 0;
bool lcdMessageActive = false;

// Constants
#define SWITCHES 6
#define VELOCITY 127
#define BOUNCE_TIME 15
#define MIDI_CHANNEL 1
#define NUM_BANKS 16
#define BANK_NAME_LENGTH 16
#define DEVICE_ID "foot_switch_pro-v1.0"

// Pins
const int DIGITAL_SWITCHES[SWITCHES] = {0, 1, 2, 3, 4, 5};

// Action Types
enum ActionType { NONE, MIDI_NOTE, MIDI_CC, KEYBOARD, BANK_SEL, BANK_NEXT, BANK_PREV };

// Action Struct
struct Action {
  ActionType type;
  union {
    struct { byte note; byte velocity; } midiNote;
    struct { byte ccNum; byte ccVal; } midiCC;
    struct { uint16_t modifiers; uint16_t key; } keyboard;
    struct { byte bankIndex; } bankSel;
  };
};

// Bank System
struct Bank {
  Action actions[SWITCHES];
  char name[BANK_NAME_LENGTH + 1]; // +1 for null terminator
};

// Current bank data
int currentBankIndex = 0;
Bank currentBank;

// Bank switching state
bool bankSwitchInProgress = false;
byte bankSwitchIgnoreIndex = 0;

// Bounce
Bounce digital[SWITCHES] = {
  Bounce(DIGITAL_SWITCHES[0], BOUNCE_TIME),
  Bounce(DIGITAL_SWITCHES[1], BOUNCE_TIME),
  Bounce(DIGITAL_SWITCHES[2], BOUNCE_TIME),
  Bounce(DIGITAL_SWITCHES[3], BOUNCE_TIME),
  Bounce(DIGITAL_SWITCHES[4], BOUNCE_TIME),
  Bounce(DIGITAL_SWITCHES[5], BOUNCE_TIME)
};

// Setup
void setup() {
  Serial.begin(9600);
  Wire.begin();
  eeprom.begin();

  for (int i = 0; i < SWITCHES; i++) {
    pinMode(DIGITAL_SWITCHES[i], INPUT_PULLUP);
  }
  
  // Initialize current bank
  initializeBank();

  lcd.init();
  lcd.backlight();

  loadBankFromEEPROM(currentBankIndex);
  displayBankName();
}

// Loop
void loop() {
  handleSerial();
  readSwitches();

  while (usbMIDI.read()) {
    // Discard incoming MIDI
  }

  if (lcdMessageActive && millis() > lcdClearTime) {
    displayBankName();
    lcdMessageActive = false;
  }
}

// Read Switches
void readSwitches() {
  static byte input = 0;
  digital[input].update();

  if (digital[input].fallingEdge()) {
    triggerAction(input, true);
  } else if (digital[input].risingEdge()) {
    triggerAction(input, false);
  }

  input = (input + 1) % SWITCHES;
}

// Helper function to get key name for display
String getKeyName(uint16_t key) {
  switch (key) {
    case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C"; case KEY_D: return "D";
    case KEY_E: return "E"; case KEY_F: return "F"; case KEY_G: return "G"; case KEY_H: return "H";
    case KEY_I: return "I"; case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
    case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O"; case KEY_P: return "P";
    case KEY_Q: return "Q"; case KEY_R: return "R"; case KEY_S: return "S"; case KEY_T: return "T";
    case KEY_U: return "U"; case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
    case KEY_Y: return "Y"; case KEY_Z: return "Z";
    case KEY_0: return "0"; case KEY_1: return "1"; case KEY_2: return "2"; case KEY_3: return "3";
    case KEY_4: return "4"; case KEY_5: return "5"; case KEY_6: return "6"; case KEY_7: return "7";
    case KEY_8: return "8"; case KEY_9: return "9";
    case KEY_F1: return "F1"; case KEY_F2: return "F2"; case KEY_F3: return "F3"; case KEY_F4: return "F4";
    case KEY_F5: return "F5"; case KEY_F6: return "F6"; case KEY_F7: return "F7"; case KEY_F8: return "F8";
    case KEY_F9: return "F9"; case KEY_F10: return "F10"; case KEY_F11: return "F11"; case KEY_F12: return "F12";
    case KEY_ENTER: return "ENTER"; case KEY_ESC: return "ESC"; case KEY_TAB: return "TAB";
    case KEY_SPACE: return "SPACE"; case KEY_BACKSPACE: return "BACKSPACE"; case KEY_DELETE: return "DELETE";
    case KEY_INSERT: return "INSERT"; case KEY_HOME: return "HOME"; case KEY_END: return "END";
    case KEY_PAGE_UP: return "PAGEUP"; case KEY_PAGE_DOWN: return "PAGEDOWN";
    case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN"; case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
    default: return "KEY(" + String(key, HEX) + ")";
  }
}

// Trigger Action
void triggerAction(byte index, bool press) {
  // Check if we're ignoring actions on this switch after a bank change
  if (bankSwitchInProgress && bankSwitchIgnoreIndex == index && !press) {
    // Clear the flag on release - next press will be normal
    bankSwitchInProgress = false;
    return;
  }
  
  // If bank switch is in progress and this is a press on the same switch, ignore
  if (bankSwitchInProgress && bankSwitchIgnoreIndex == index && press) {
    return; // Don't execute any action from new bank until switch is released
  }

  Action &act = currentBank.actions[index];
  
  // Display on LCD first (before executing action)
  if (press) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pressed S");
    lcd.print(index);

    lcd.setCursor(0, 1);
    switch (act.type) {
      case MIDI_NOTE:
        lcd.print("NOTE ");
        lcd.print(act.midiNote.note);
        break;
      case MIDI_CC:
        lcd.print("CC ");
        lcd.print(act.midiCC.ccNum);
        lcd.print(":");
        lcd.print(act.midiCC.ccVal);
        break;
      case KEYBOARD: {
        String modStr = "";
        if ((act.keyboard.modifiers & 0xFF) & 0x01) modStr += "Ctrl+";
        if ((act.keyboard.modifiers & 0xFF) & 0x02) modStr += "Shift+";
        if ((act.keyboard.modifiers & 0xFF) & 0x04) modStr += "Alt+";
        String display = modStr + getKeyName(act.keyboard.key);
        if (display.length() > 16) display = display.substring(0, 16);
        lcd.print(display);
        break;
      }
      case BANK_SEL:
        lcd.print("BANK ");
        lcd.print(act.bankSel.bankIndex + 1); // Display 1-based bank number
        break;
      case BANK_NEXT:
        lcd.print("NEXT BANK");
        break;
      case BANK_PREV:
        lcd.print("PREV BANK");
        break;
      default:
        lcd.print("NONE");
    }
    lcdMessageActive = true;
    lcdClearTime = millis() + 2000; // Show bank action for 2 seconds
  }
  
  switch (act.type) {
    case MIDI_NOTE:
      if (press)
        usbMIDI.sendNoteOn(act.midiNote.note, act.midiNote.velocity, MIDI_CHANNEL);
      else
        usbMIDI.sendNoteOff(act.midiNote.note, 0, MIDI_CHANNEL);
      break;
    case MIDI_CC:
      if (press)
        usbMIDI.sendControlChange(act.midiCC.ccNum, act.midiCC.ccVal, MIDI_CHANNEL);
      break;
    case KEYBOARD:
      if (press) {
        Keyboard.set_modifier(act.keyboard.modifiers);
        Keyboard.press(act.keyboard.key);
      } else {
        Keyboard.set_modifier(0);
        Keyboard.release(act.keyboard.key);
      }
      Keyboard.send_now();
      break;
    case BANK_SEL:
      if (press) {
        selectBank(act.bankSel.bankIndex);
        // Set flag to ignore actions on this switch until released
        bankSwitchInProgress = true;
        bankSwitchIgnoreIndex = index;
      }
      break;
    case BANK_NEXT:
      if (press) {
        selectBank((currentBankIndex + 1) % NUM_BANKS);
        // Set flag to ignore actions on this switch until released
        bankSwitchInProgress = true;
        bankSwitchIgnoreIndex = index;
      }
      break;
    case BANK_PREV:
      if (press) {
        selectBank((currentBankIndex + NUM_BANKS - 1) % NUM_BANKS);
        // Set flag to ignore actions on this switch until released
        bankSwitchInProgress = true;
        bankSwitchIgnoreIndex = index;
      }
      break;
  }
}

// Serial Command Parsing
void handleSerial() {
  static String input;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCommand(input);
      input = "";
    } else {
      input += c;
    }
  }
}

// Helper to parse keyboard modifier strings
uint16_t parseModifiers(const String &str) {
  uint16_t mod = 0;
  if (str.indexOf("CTRL") >= 0) mod |= MODIFIERKEY_CTRL;
  if (str.indexOf("SHIFT") >= 0) mod |= MODIFIERKEY_SHIFT;
  if (str.indexOf("ALT") >= 0) mod |= MODIFIERKEY_ALT;
  return mod;
}

// String to keycode (simplified)
uint16_t parseKey(const String &key) {
  // Named keys
   // Multi-character strings
  if (key == "ENTER") return KEY_ENTER;
  if (key == "ESC") return KEY_ESC;
  if (key == "ESCAPE") return KEY_ESC;
  if (key == "TAB") return KEY_TAB;
  if (key == "SPACE") return KEY_SPACE;
  if (key == "BACKSPACE") return KEY_BACKSPACE;
  if (key == "DELETE") return KEY_DELETE;
  if (key == "INSERT") return KEY_INSERT;
  if (key == "HOME") return KEY_HOME;
  if (key == "END") return KEY_END;
  if (key == "PAGEUP") return KEY_PAGE_UP;
  if (key == "PAGEDOWN") return KEY_PAGE_DOWN;
  if (key == "UP") return KEY_UP;
  if (key == "DOWN") return KEY_DOWN;
  if (key == "LEFT") return KEY_LEFT;
  if (key == "RIGHT") return KEY_RIGHT;
  if (key == "CAPSLOCK") return KEY_CAPS_LOCK;
  if (key == "NUMLOCK") return KEY_NUM_LOCK;
  if (key == "SCROLLLOCK") return KEY_SCROLL_LOCK;
  if (key == "PRINTSCREEN") return KEY_PRINTSCREEN;
  if (key == "PAUSE") return KEY_PAUSE;

  // Function keys
  if (key == "F1") return KEY_F1;
  if (key == "F2") return KEY_F2;
  if (key == "F3") return KEY_F3;
  if (key == "F4") return KEY_F4;
  if (key == "F5") return KEY_F5;
  if (key == "F6") return KEY_F6;
  if (key == "F7") return KEY_F7;
  if (key == "F8") return KEY_F8;
  if (key == "F9") return KEY_F9;
  if (key == "F10") return KEY_F10;
  if (key == "F11") return KEY_F11;
  if (key == "F12") return KEY_F12;

  if (key.length() == 1) {
    char c = key.charAt(0);
    if (c >= 'a' && c <= 'z') return c - 'a' + KEY_A;
    if (c >= 'A' && c <= 'Z') return c - 'A' + KEY_A;
    if (c >= '0' && c <= '9') return c - '0' + KEY_0;
    if (c == ' ') return KEY_SPACE;
  }
  return 0;
}

// Serial Command Processor
void processCommand(String cmd) {
  cmd.trim();
  
  if (cmd == "INFO") {
    Serial.println(DEVICE_ID);
  } else if (cmd.startsWith("SET")) {
    // SET {bank_index} {switch_id} {action}
    int s1 = cmd.indexOf(' ');
    int s2 = cmd.indexOf(' ', s1 + 1);
    int s3 = cmd.indexOf(' ', s2 + 1);
    int s4 = cmd.indexOf(' ', s3 + 1);

    int bankIdx = cmd.substring(s1 + 1, s2).toInt();
    int switchIdx = cmd.substring(s2 + 1, s3).toInt();
    String type = cmd.substring(s3 + 1, s4);
    String args = (s4 > 0) ? cmd.substring(s4 + 1) : "";

    if (bankIdx >= 0 && bankIdx < NUM_BANKS && switchIdx >= 0 && switchIdx < SWITCHES) {
      // Load target bank if not current
      Bank targetBank;
      if (bankIdx == currentBankIndex) {
        targetBank = currentBank;
      } else {
        loadBankFromEEPROM(bankIdx, targetBank);
      }
      
      if (type == "NOTE") {
        targetBank.actions[switchIdx].type = MIDI_NOTE;
        targetBank.actions[switchIdx].midiNote.note = args.toInt();
        targetBank.actions[switchIdx].midiNote.velocity = VELOCITY;
      } else if (type == "CC") {
        int space = args.indexOf(' ');
        targetBank.actions[switchIdx].type = MIDI_CC;
        targetBank.actions[switchIdx].midiCC.ccNum = args.substring(0, space).toInt();
        targetBank.actions[switchIdx].midiCC.ccVal = args.substring(space + 1).toInt();
      } else if (type == "KEY") {
        targetBank.actions[switchIdx].type = KEYBOARD;
        targetBank.actions[switchIdx].keyboard.modifiers = parseModifiers(args);
        int lastPlus = args.lastIndexOf('+');
        String key = (lastPlus >= 0) ? args.substring(lastPlus + 1) : args;
        targetBank.actions[switchIdx].keyboard.key = parseKey(key);
      } else if (type == "BANK_SEL") {
        targetBank.actions[switchIdx].type = BANK_SEL;
        targetBank.actions[switchIdx].bankSel.bankIndex = args.toInt();
      } else if (type == "BANK_NEXT") {
        targetBank.actions[switchIdx].type = BANK_NEXT;
      } else if (type == "BANK_PREV") {
        targetBank.actions[switchIdx].type = BANK_PREV;
      } else if (type == "NONE") {
        targetBank.actions[switchIdx].type = NONE;
      }
      
      // Save bank and update current if needed
      saveBankToEEPROM(bankIdx, targetBank);
      if (bankIdx == currentBankIndex) {
        currentBank = targetBank;
      }
      Serial.println("OK");
    }
  } else if (cmd.startsWith("GET ALL")) {
    int space = cmd.indexOf(' ', 4);
    if (space > 0) {
      int bankIdx = cmd.substring(space + 1).toInt();
      if (bankIdx >= 0 && bankIdx < NUM_BANKS) {
        Bank bank;
        loadBankFromEEPROM(bankIdx, bank);
        for (int i = 0; i < SWITCHES; i++) {
          printAction(i, bank.actions[i]);
        }
      }
    }
  } else if (cmd.startsWith("SEL_BANK")) {
    int bankIdx = cmd.substring(9).toInt();
    if (bankIdx >= 0 && bankIdx < NUM_BANKS) {
      selectBank(bankIdx);
      Serial.println("OK");
    }
  } else if (cmd == "NEXT_BANK") {
    selectBank((currentBankIndex + 1) % NUM_BANKS);
    Serial.println("OK");
  } else if (cmd == "PREV_BANK") {
    selectBank((currentBankIndex + NUM_BANKS - 1) % NUM_BANKS);
    Serial.println("OK");
  } else if (cmd.startsWith("CLEAR_BANK")) {
    int bankIdx = cmd.substring(11).toInt();
    if (bankIdx >= 0 && bankIdx < NUM_BANKS) {
      clearBank(bankIdx);
      Serial.println("OK");
    }
  } else if (cmd.startsWith("NAME_BANK")) {
    int space = cmd.indexOf(' ', 10);
    if (space > 0) {
      int bankIdx = cmd.substring(10, space).toInt();
      String name = cmd.substring(space + 1);
      if (bankIdx >= 0 && bankIdx < NUM_BANKS) {
        setBankName(bankIdx, name);
        Serial.println("OK");
      }
    }
  } else if (cmd == "NAMES") {
    // Return all bank names
    for (int i = 0; i < NUM_BANKS; i++) {
      Bank bank;
      loadBankFromEEPROM(i, bank);
      Serial.print("BANK ");
      Serial.print(i);
      Serial.print(": ");
      if (strlen(bank.name) > 0) {
        Serial.println(bank.name);
      } else {
        Serial.print("Bank ");
        Serial.println(i);
      }
    }
  }
}

// Print Action for a Switch
void printAction(int idx) {
  printAction(idx, currentBank.actions[idx]);
}

void printAction(int idx, Action &action) {
  Serial.print("SWITCH ");
  Serial.print(idx);
  Serial.print(": ");
  switch (action.type) {
    case MIDI_NOTE:
      Serial.print("NOTE ");
      Serial.println(action.midiNote.note);
      break;
    case MIDI_CC:
      Serial.print("CC ");
      Serial.print(action.midiCC.ccNum);
      Serial.print(" ");
      Serial.println(action.midiCC.ccVal);
      break;
    case KEYBOARD: {
      Serial.print("KEY ");
      String modStr = "";      
      if ((action.keyboard.modifiers & 0xFF) & 0x01) modStr += "CTRL+";
      if ((action.keyboard.modifiers & 0xFF) & 0x02) modStr += "SHIFT+";
      if ((action.keyboard.modifiers & 0xFF) & 0x04) modStr += "ALT+";
      Serial.println(modStr + getKeyName(action.keyboard.key));
      break;
    }
    case BANK_SEL:
      Serial.print("BANK_SEL ");
      Serial.println(action.bankSel.bankIndex);
      break;
    case BANK_NEXT:
      Serial.println("BANK_NEXT");
      break;
    case BANK_PREV:
      Serial.println("BANK_PREV");
      break;
    default:
      Serial.println("NONE");
  }
}

// EEPROM Storage Layout
#define EEPROM_BASE_ADDR 0
#define ACTION_SIZE 17
#define BANK_SIZE (SWITCHES * ACTION_SIZE + BANK_NAME_LENGTH + 1) // Actions + name + padding
#define BANK_ADDR(bank) (EEPROM_BASE_ADDR + (bank) * BANK_SIZE)

// Bank Management Functions
void initializeBank() {
  for (int i = 0; i < SWITCHES; i++) {
    currentBank.actions[i].type = NONE;
  }
  strcpy(currentBank.name, "");
}

void selectBank(int bankIndex) {
  if (bankIndex >= 0 && bankIndex < NUM_BANKS) {
    currentBankIndex = bankIndex;
    loadBankFromEEPROM(bankIndex);
    displayBankName();
  }
}

void loadBankFromEEPROM(int bankIndex) {
  loadBankFromEEPROM(bankIndex, currentBank);
}

void loadBankFromEEPROM(int bankIndex, Bank &bank) {
  int baseAddr = BANK_ADDR(bankIndex);
  
  // Load actions
  for (int i = 0; i < SWITCHES; i++) {
    int addr = baseAddr + i * ACTION_SIZE;
    bank.actions[i].type = (ActionType)eeprom.readByte(addr);
    delay(2);
    
    switch (bank.actions[i].type) {
      case MIDI_NOTE:
        bank.actions[i].midiNote.note = eeprom.readByte(addr + 1);
        bank.actions[i].midiNote.velocity = eeprom.readByte(addr + 2);
        break;
      case MIDI_CC:
        bank.actions[i].midiCC.ccNum = eeprom.readByte(addr + 1);
        bank.actions[i].midiCC.ccVal = eeprom.readByte(addr + 2);
        break;
      case KEYBOARD:
        // Clear the entire union first to avoid garbage data
        memset(&bank.actions[i], 0, sizeof(Action));
        bank.actions[i].type = KEYBOARD;
        bank.actions[i].keyboard.modifiers = ((uint16_t)eeprom.readByte(addr + 1) << 8) | eeprom.readByte(addr + 2);
        bank.actions[i].keyboard.key = ((uint16_t)eeprom.readByte(addr + 3) << 8) | eeprom.readByte(addr + 4);
        break;
      case BANK_SEL:
        bank.actions[i].bankSel.bankIndex = eeprom.readByte(addr + 1);
        break;
      case BANK_NEXT:
      case BANK_PREV:
        // No additional data needed
        break;
      default:
        bank.actions[i].type = NONE;
        break;
    }
    delay(2);
  }
  
  // Load bank name
  int nameAddr = baseAddr + SWITCHES * ACTION_SIZE;
  for (int i = 0; i < BANK_NAME_LENGTH; i++) {
    bank.name[i] = eeprom.readByte(nameAddr + i);
    delay(1);
  }
  bank.name[BANK_NAME_LENGTH] = '\0';
  
  // Check if bank name is valid (contains only printable characters)
  bool validName = true;
  if (bank.name[0] == 0xFF || bank.name[0] == 0x00) {
    validName = false;
  } else {
    for (int i = 0; i < BANK_NAME_LENGTH && bank.name[i] != '\0'; i++) {
      if (bank.name[i] < 32 || bank.name[i] > 126) {
        validName = false;
        break;
      }
    }
  }
  
  // If invalid, set empty name
  if (!validName) {
    bank.name[0] = '\0';
  }
}

void saveBankToEEPROM(int bankIndex, Bank &bank) {
  int baseAddr = BANK_ADDR(bankIndex);
  
  // Save actions
  for (int i = 0; i < SWITCHES; i++) {
    int addr = baseAddr + i * ACTION_SIZE;
    eeprom.writeByte(addr, bank.actions[i].type);
    delay(5);
    
    switch (bank.actions[i].type) {
      case MIDI_NOTE:
        eeprom.writeByte(addr + 1, bank.actions[i].midiNote.note);
        eeprom.writeByte(addr + 2, bank.actions[i].midiNote.velocity);
        break;
      case MIDI_CC:
        eeprom.writeByte(addr + 1, bank.actions[i].midiCC.ccNum);
        eeprom.writeByte(addr + 2, bank.actions[i].midiCC.ccVal);
        break;
      case KEYBOARD:
        eeprom.writeByte(addr + 1, (bank.actions[i].keyboard.modifiers >> 8) & 0xFF);
        eeprom.writeByte(addr + 2, bank.actions[i].keyboard.modifiers & 0xFF);
        eeprom.writeByte(addr + 3, (bank.actions[i].keyboard.key >> 8) & 0xFF);
        eeprom.writeByte(addr + 4, bank.actions[i].keyboard.key & 0xFF);
        break;
      case BANK_SEL:
        eeprom.writeByte(addr + 1, bank.actions[i].bankSel.bankIndex);
        break;
      case BANK_NEXT:
      case BANK_PREV:
        // No additional data needed
        break;
    }
    delay(5);
  }
  
  // Save bank name
  int nameAddr = baseAddr + SWITCHES * ACTION_SIZE;
  for (int i = 0; i < BANK_NAME_LENGTH; i++) {
    eeprom.writeByte(nameAddr + i, bank.name[i]);
    delay(2);
  }
}

void clearBank(int bankIndex) {
  Bank emptyBank;
  initializeBank();
  emptyBank = currentBank; // Copy initialized empty bank
  saveBankToEEPROM(bankIndex, emptyBank);
  
  if (bankIndex == currentBankIndex) {
    currentBank = emptyBank;
    displayBankName();
  }
}

void setBankName(int bankIndex, String name) {
  Bank bank;
  if (bankIndex == currentBankIndex) {
    bank = currentBank;
  } else {
    loadBankFromEEPROM(bankIndex, bank);
  }
  
  // Copy name, truncate if too long
  strncpy(bank.name, name.c_str(), BANK_NAME_LENGTH);
  bank.name[BANK_NAME_LENGTH] = '\0';
  
  saveBankToEEPROM(bankIndex, bank);
  if (bankIndex == currentBankIndex) {
    currentBank = bank;
    displayBankName();
  }
}

void displayBankName() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (strlen(currentBank.name) > 0) {
    lcd.print(currentBank.name);
  } else {
    lcd.print("Bank ");
    lcd.print(currentBankIndex);
  }
  lcd.setCursor(0, 1);
  lcd.print("Ready");
}
