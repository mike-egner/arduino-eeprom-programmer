
#define DATA_0 22
#define DATA_7 29
#define ADDRESS_0 32
#define ADDRESS_11 43
#define OUTPUT_ENABLE 52
#define WRITE_ENABLE 53

/* ------------------------------------ Global Variables ---------------------------*/

// set up program parameters
byte dummy_data = 12;
char hex_data;
int current_address = 255;
byte current_data = 0;
int block_start = 0;
int block_end = 1023;
String user_input;
int user_choice;
byte virtual_chip[2048];

/* ---------------------------------- ADDRESSING -----------------------------------*/

void setAddress(int address)
{
  // set address pins by incrementing through bits in the address integer
  for (int pin = ADDRESS_0; pin <= ADDRESS_11; pin += 1)
  {
    digitalWrite(pin, address & 1);
    address = address >> 1;
  }
}

void cycleAddresses(int start_addr, int end_addr, int cycle_seconds)
{
  for (int count = start_addr; count <= end_addr; count += 1)
  {
    setAddress(count);
    Serial.println("Activating address " + String(count) + ".");
    delay(cycle_seconds * 1000);
  }
}

/* ---------------------------------- READING -------------------------------------*/

byte readChip(int address)
{
  // set Arduino data pins to input (and clears bus)
  for (int pin = DATA_0; pin <= DATA_7; pin += 1)
  {
    pinMode(pin, INPUT);
  }

  // enable chip's outputs
  digitalWrite(OUTPUT_ENABLE, LOW); //enable output because it's an active-low

  setAddress(address);
  delay(1);

  byte data = 0;
  for (int pin = DATA_7; pin >= DATA_0; pin -= 1)
  {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

void printBlockHex(int start, int finish)
{
  for (int base = start; base < finish; base += 16)
  {
    byte data[16];
    for (int offset = 0; offset < 16; offset += 1)
    {
      data[offset] = readChip(base + offset);
    }
    char buf[80];
    sprintf(buf, "%03x: %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    Serial.println(buf);
  }
}

void printBlockDec(int start, int finish)
{
  for (int base = start; base < finish; base += 16)
  {
    byte data[16];
    for (int offset = 0; offset < 16; offset += 1)
    {
      data[offset] = readChip(base + offset);
    }
    char buf[80];
    sprintf(buf, "%u: %u %u %u %u %u %u %u %u   %u %u %u %u %u %u %u %u",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    Serial.println(buf);
  }
}

void printVirtualBlockHex(int start, int finish)
{
  for (int base = start; base < finish; base += 16)
  {
    byte data[16];
    for (int offset = 0; offset < 16; offset += 1)
    {
      data[offset] = virtual_chip[base + offset];
    }
    char buf[80];
    sprintf(buf, "%03x: %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    Serial.println(buf);
  }
}

void printVirtualToActualErrors(int start, int finish)
{
  int error_count = 0;
  for (int base = start; base < finish; base += 16)
  {
    byte chip_data[16];
    byte virtual_data[16];
    byte results[16];
    for (int offset = 0; offset < 16; offset += 1)
    {
      chip_data[offset] = readChip(base + offset);
      virtual_data[offset] = virtual_chip[base + offset];
      if (chip_data[offset] == virtual_data[offset])
      {
        results[offset] = 0;
      }
      else
      {
        results[offset] = 1;
        error_count += 1;
      }
    }
    char buf[80];
    sprintf(buf, "%03x: %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, results[0], results[1], results[2], results[3], results[4], results[5], results[6], results[7],
            results[8], results[9], results[10], results[11], results[12], results[13], results[14], results[15]);
    Serial.println(buf);
  }
  Serial.println("");
  Serial.println("Total errors: " + String(error_count));
}

/* ---------------------------------- WRITING ---------------------------------------*/

void pulseWrite()
{

  //pulse the Write-Enable pin LOW (it's active-low)
  digitalWrite(WRITE_ENABLE, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_ENABLE, HIGH);
  delay(10);
}

void writeChip(int address, byte data)
{
  // disable chip's outputs to clear the bus
  digitalWrite(OUTPUT_ENABLE, HIGH); //disables output because it's an active-low

  // set Arduino data pins to output onto the bus
  for (int pin = DATA_0; pin <= DATA_7; pin += 1)
  {
    pinMode(pin, OUTPUT);
  }

  //set the address
  setAddress(address);

  // Serial.println("Writing " + String(data) + " to " + String(address)); // DELETE <<<<<<<<<<<<<<<

  // set data pins by incrementing through bits in the byte
  for (int pin = DATA_0; pin <= DATA_7; pin += 1)
  {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }

  delay(1);

  pulseWrite();
}

void writeBlock(int start, int finish, byte data)
{
  for (int address = start; address <= finish; address++)
  {
    writeChip(address, data);
  }
}

/* ---------------------------------- EEPROM ---------------------------------------*/

byte common_anode_display_digits[] = {0x01, 0x4f, 0x12, 0x06, 0x4c, 0x24, 0x20, 0x0f, 0x00, 0x04, 0x08, 0x60, 0x31, 0x42, 0x30, 0x38};

byte common_cathode_display_digits[] = {0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b, 0x77, 0x1f, 0x4e, 0x3d, 0x4f, 0x47};

void flashSingleDisplay(String display_type)
{
  uint32_t time_start;
  uint32_t time_end;
  if (display_type == "common_cathode")
  {
    Serial.println("Now flashing common cathode 7-segment display sequence...");
    delay(500);
    time_start = millis();
    for (int address = 0; address < sizeof(common_cathode_display_digits); address += 1)
    {
      writeChip(address, common_cathode_display_digits[address]);
    }
    time_end = millis();
    Serial.println("Flash complete (in " + String(time_end - time_start) + " milliseconds).");
  }
  else
  {
    Serial.println("Now flashing common anode 7-segment display sequence...");
    delay(500);
    time_start = millis();
    for (int address = 0; address < sizeof(common_anode_display_digits); address += 1)
    {
      writeChip(address, common_anode_display_digits[address]);
    }
    time_end = millis();
    Serial.println("Flash complete (in " + String(time_end - time_start) + " milliseconds).");
  }
}

void flashQuadDisplay()
{
  uint32_t time_start;
  uint32_t time_end;
  Serial.println("Now flashing 4x common cathode 7-segment display sequence...");
  delay(500);
  time_start = millis();

  // standard binary
  for (int value = 0; value < 256; value += 1)
  {
    writeChip(value, common_cathode_display_digits[value % 10]);
    virtual_chip[value] = common_cathode_display_digits[value % 10];
  }
  for (int value = 0; value < 256; value += 1)
  {
    writeChip(value + 256, common_cathode_display_digits[(value / 10) % 10]);
    virtual_chip[value + 256] = common_cathode_display_digits[(value / 10) % 10];
  }
  /*
  for (int value = 0; value < 256; value += 1)
  {
    writeChip(value + 512, common_cathode_display_digits[(value / 100) % 10]);
    virtual_chip[value + 512] = common_cathode_display_digits[(value / 100) % 10];
  }
  for (int value = 0; value < 256; value += 1)
  {
    writeChip(value + 768, 0);
    virtual_chip[value + 768] = 0;
  }
  */

  // two's complement binary

  /*
  for (int value = -128; value < 128; value += 1)
  {
    Serial.println("Writing " + String(common_cathode_display_digits[abs(value) % 10]) + " to " + String((byte)value + 1024));
    writeChip((byte)value + 1024, common_cathode_display_digits[abs(value) % 10]);
  }
  for (int value = -128; value < 128; value += 1)
  {
    Serial.println("Writing " + String(common_cathode_display_digits[abs(value / 10) % 10]) + " to " + String((byte)value + 1280));
    writeChip((byte)value + 1280, common_cathode_display_digits[abs(value / 10) % 10]);
  }
  for (int value = -128; value < 128; value += 1)
  {
    Serial.println("Writing " + String(common_cathode_display_digits[abs(value / 100) % 10]) + " to " + String((byte)value + 1536));
    writeChip((byte)value + 1536, common_cathode_display_digits[abs(value / 100) % 10]);
  }
  for (int value = -128; value < 128; value += 1)
  {
    if (value < 0)
    {
      Serial.println("Writing " + String(0x01) + " to " + String((byte)value + 1792));
      writeChip((byte)value + 1792, 0x01);
    }
    else
    {
      Serial.println("Writing " + String(0) + " to " + String((byte)value + 1792));
      writeChip((byte)value + 1792, 0);
    }
  }
  */

  time_end = millis();
  Serial.println("Flash complete (in " + String(time_end - time_start) + " milliseconds).");
}

void eraseChip(int start_address, int end_address)
{
  for (int address = start_address; address < end_address + 1; address += 1)
  {
    writeChip(address, 0xff);
  }
}

void writeTest(int test_length)
{
  for (int address = 0; address < test_length; address += 1)
  {
    writeChip(address, address);
  }
}

/* ------------------------------------ Input / Output ----------------------------*/

bool isValidNumber(String str)
{
  for (byte i = 0; i < str.length(); i++)
  {
    if (isDigit(str.charAt(i)))
      return true;
  }
  return false;
}

void flushBuffer()
{
  String x;
  while (Serial.available())
  {
    x = Serial.readString();
  }
}

/* ------------------------------------ Menus -------------------------------------*/

void printMainMenu()
{
  Serial.println("|***************************|");
  Serial.println("|**** EEPROM Programmer ****|");
  Serial.println("|******** Main Menu ********|");
  Serial.println("|***************************|");
  Serial.println("");
  Serial.println("Select an option:");
  Serial.println("1 - Write data");
  Serial.println("2 - Read data");
  Serial.println("3 - Erase chip");
  Serial.println("4 - Read a block of data in decimal");
  Serial.println("5 - Read a block of data in hexadecimal");
  Serial.println("6 - Flash digits to drive a single 7-segment display");
  Serial.println("7 - Flash digits to drive four 7-segment displays");
  Serial.println("8 - Compare virtual chip data to EEPROM");
  Serial.println("9 - Perform a memory addressing test");
  Serial.println("0 - Write a block of data");
  Serial.println("");
}

/* -------------------------------- Main Setup -------------------------------------*/

void setup()
{
  // set up control pins
  digitalWrite(WRITE_ENABLE, HIGH);
  pinMode(WRITE_ENABLE, OUTPUT);
  digitalWrite(OUTPUT_ENABLE, HIGH);
  pinMode(OUTPUT_ENABLE, OUTPUT);

  // set up Arduino's address pins
  for (int pin = ADDRESS_0; pin <= ADDRESS_11; pin += 1)
  {
    pinMode(pin, OUTPUT);
  }

  // activate serial interface and print greeting
  Serial.begin(57600);
  Serial.println("Hello World.");
  delay(1000);

  digitalWrite(OUTPUT_ENABLE, LOW);
}

/* ------------------------------ Main Loop ----------------------------------------*/

void loop()
{

  while (Serial.available())
  {
    user_input = Serial.read();
  }

  printMainMenu();

  while (!Serial.available())
  {
    // wait for user input
  }

  user_input = Serial.readString();
  user_choice = user_input.toInt();

  Serial.println("You chose option " + String(user_choice));

  switch (user_choice)
  {
  case 1:
    Serial.println("This operation writes a single byte of data to a specific memory address.");
    Serial.println("");
    Serial.println("Enter memory address to write to: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    current_address = Serial.readString().toInt();
    Serial.println("Enter data to write (an integer): ");
    while (!Serial.available())
    {
      // wait for user input
    }
    current_data = Serial.readString().toInt();
    Serial.println("Writing " + String(current_data) + " to address " + String(current_address));
    writeChip(current_address, current_data);
    if (readChip(current_address) == current_data)
    {
      Serial.println("Data verified. Address " + String(current_address) + " now reads " + String(current_data));
    }
    else
    {
      Serial.println("An error occured. The data could not be verified. Please perform a read operation to verify.");
    }
    break;

  case 2:
    Serial.println("Enter memory address to read from: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    current_address = Serial.readString().toInt();
    current_data = readChip(current_address);
    Serial.println("Data at address " + String(current_address) + ": " + String(current_data) + " - " + String(current_data, HEX));
    break;

  case 3:
    Serial.println("Enter memory address to start erase at: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_start = Serial.readString().toInt();
    Serial.println("Enter memory address to end erase at: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_end = Serial.readString().toInt();
    Serial.println("Erasing...");
    eraseChip(block_start, block_end);
    Serial.println("Erase successful.");
    break;

  case 4:
    Serial.println("Enter memory address to start reading at: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_start = Serial.readString().toInt();
    Serial.println("Enter memory addresses to read: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_end = block_start + Serial.readString().toInt();
    Serial.println("Reading...");
    printBlockDec(block_start, block_end);
    break;

  case 5:
    Serial.println("Enter memory address to start reading at: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_start = Serial.readString().toInt();
    Serial.println("Enter memory addresses to read: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_end = block_start + Serial.readString().toInt();
    Serial.println("Reading...");
    printBlockHex(block_start, block_end);
    break;

  case 6:
    Serial.println("The chip will now be flashed with a sequence of data that can be used to drive a single 7-segment display.");
    Serial.println("Please select the type of display: ");
    Serial.println("1 - Common Anode");
    Serial.println("2 - Common Cathode");
    while (!Serial.available())
    {
      // wait for user input
    }
    user_choice = Serial.readString().toInt();
    switch (user_choice)
    {
    case 1:
      Serial.println("You chose 1 - Common Anode");
      flashSingleDisplay("common_anode");
      break;
    case 2:
      Serial.println("You chose 2 - Common Cathode");
      flashSingleDisplay("common_cathode");
      break;
    default:
      Serial.println("That's not a valid choice. Returning to main menu.");
      break;
    }

    break;

  case 7:
    Serial.println("The chip will now be flashed with a sequence of data that can be used to drive four 7-segment displays.");
    Serial.println("At this stage, only common cathode is supported.");
    flashQuadDisplay();
    break;

  case 8:
    Serial.println("Enter memory address to start reading at:");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_start = Serial.readString().toInt();
    Serial.println("How many memory addresses do you want to read?");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_end = block_start + Serial.readString().toInt();
    Serial.println("");
    Serial.println("---------- EEPROM ----------");
    Serial.println("");
    printBlockHex(block_start, block_end);
    Serial.println("");
    Serial.println("---------- Virtual ---------");
    Serial.println("");
    printVirtualBlockHex(block_start, block_end);
    Serial.println("");
    Serial.println("---------- Errors ----------");
    Serial.println("");
    printVirtualToActualErrors(block_start, block_end);
    break;

  case 9:
    Serial.println("This test will write the last byte of the memory address to each address and print the results.");
    Serial.println("How many addresses do you want to test?");
    while (!Serial.available())
    {
      // wait for user input
    }
    user_choice = Serial.readString().toInt();
    Serial.println("Now testing the first " + String(user_choice) + " addresses...");
    writeTest(user_choice);
    printBlockDec(0, user_choice - 1);
    break;

  case 0:
    Serial.println("This operation writes the same byte of data to a block of memory.");
    Serial.println("Enter memory address to start writing at: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_start = Serial.readString().toInt();
    Serial.println("Enter size of block: ");
    while (!Serial.available())
    {
      // wait for user input
    }
    block_end = block_start + Serial.readString().toInt();
    Serial.println("What data do you want to write? (An integer up to 255)");
    while (!Serial.available())
    {
      // wait for user input
    }
    current_data = Serial.readString().toInt();
    Serial.println("Now writing " + String(current_data) + " to block " + String(block_start) + " - " + String(block_end));
    writeBlock(block_start, block_end, current_data);
    break;

  default:
    Serial.println("Invalid Input. Please input your choice as a number: ");
    break;
  }

  Serial.println("");
}
