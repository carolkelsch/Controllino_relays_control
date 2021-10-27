# Controllino_relays_control
This project provides a code for Cotrollino MEGA or MAXI board (https://www.controllino.com/). It enables the relays to be controlled by Serial commands.

The Controllino will parse the packages packages with commands to controll the relays and read digital inputs states.

# Configuring Arduino IDE, installing drivers and packages

In order to use the Controllino boards, some packages need to be configured in the Arduino IDE.
- Install Arduino IDE (https://www.arduino.cc/en/software)
- Add package for Controllino:
Go to **Tools -> Library Manager** and search for Controllino, install the latest version.

- Install Controllino boards:
Open Arduino IDE, then go to **File -> Preferences** and add the URL "https://raw.githubusercontent.com/CONTROLLINO-PLC/CONTROLLINO_Library/master/Boards/package_ControllinoHardware_index.json" in the field **Additional Boards Manager URLs**. Go to **Tools -> Board -> Boards Manager** and search for **Controllino**, install the latest version.

# Steps to run the code

To flash the Controllino boards, you just need to configure which board are you using in the **Tools -> Board -> CONTROLLINO_Boards-avr** and select the COM port. Then just click on the flash buttom, and the code will be flashed to the board and after that be executed.

# Communicating with the board

With the same USB connector to the Controllino board, you can open a terminal with the COM port you used to flash. Through there, you can send the commands to the board. The serial configuration are described bellow.
| Setup | Value |
| --------- | --------- |
|  Baud Rate  |  115200  |
|  Data Bits  |  8  |
|  Parity  |  None  |
|  Stop Bits  |  1  |
|  Flow Control  |  None  |

# Working modes

There are 2 different working modes.

- **Generic**: it enables you to open and close individually each relay.
- **Optimized**: this mode was optimized for a specific functionality. It configures the 4 relays as 2 pairs. The relays 1 and 3 are activated to enable a motor to roll in a direction and the relays 2 and 4 enables the motor to roll in the oposite direction.
1 and 3 respond to the UP command, and 2 and 4 to the DOWN command. Optimized has also a time configuration. The time can be configured from 0 to 20000 (in milliseconds). If set to 0, the states of the relays are totally dependent of the commands. If you set the command UP, it will stay activated until you give the STOP or DOWN command.
If the time set is different than 0, then the commands UP and DOWN will activate the relays, and start a timer, with the specified time. When the timer is triggered the relays will automatically receive STOP command. If another UP or DOWN message is received before the timer get triggered, then the timer is restarted.


**NOTE:** Due to the timer constraints, the timming mode uses a fixed timer of 200 ms, and multiplies it according to the configured time. So if a time for less than 200 ms was configured, the board will just consider 0 s. If a time of 15,9 s was configured, then the board will compute the time for 15,8 s.

# Setting commands for generic working mode
| Command  |  Function  |
| --------- | --------- |
|  A0 00 01 01 A2 |  Open relay 1 |
|  A0 00 01 00 A1 |  Close relay 1 |
|  A0 00 02 01 A3 |  Open relay 2 |
|  A0 00 02 00 A2 |  Close relay 2 |
|  A0 00 03 01 A4 |  Open relay 3 |
|  A0 00 03 00 A3 |  Close relay 3 |
|  A0 00 04 01 A5 |  Open relay 4 |
|  A0 00 04 00 A4 |  Close relay 4 |
|  0A 00 R# C# CS |  Open or close multiple relays |

The last command can change the state of multiple relays, the structure of the command is:
- **0A**: identificator byte;
- **R#**: relays mask byte, if bit is set to 1, then the command will be applied to the relay;


| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| - | - | - | - | Relay 4 | Relay 3 | Relay 2 | Relay 1 |

- **C#**: command mask byte, if bit set to 1 (Open) the respective relay will be opened, if set to 0 (Close), then relay will be closed. Note that this command only works if the relay is set in the relay mask;

| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| - | - | - | - | 1 (Open) / 0 (Close) | 1 (Open) / 0 (Close) | 1 (Open) / 0 (Close) | 1 (Open) / 0 (Close) |

- **ACK**: 0x06, acknowledge byte in ASCII;
- **CS**: simple check sum, aquired by summing all the previous bytes os the message.

The response for the setting commands are the first 3 bytes of the command, plus a 0x06 byte (ACK) and the new CheckSum.

# Setting commands for optimized working mode
| Command  |  Function  |
| --------- | --------- |
|  A0 00 00 00 A0 |  STOP |
|  A0 00 00 01 A1 |  DOWN |
|  A0 00 00 02 A2 |  UP |

The response for the commands are the first 2 bytes of the command, plus a 0x06 byte (ACK) and the new CheckSum.

# Getting commands for both modes
| Command  |  Function  |
| --------- | --------- |
|  B0 00 00 00 B0 |  Status of connection |
|  B0 00 00 01 B1 |  Status of relay 1 |
|  B0 00 00 02 B1 |  Status of relay 2 |
|  B0 00 00 03 B1 |  Status of relay 3 |
|  B0 00 00 04 B1 |  Status of relay 4 |
|  B0 00 00 05 B1 |  Status of functional mode |
|  B0 00 00 06 B1 |  Status of top end switch |
|  B0 00 00 07 B1 |  Status of bottom end switch |
|  0B 00 R# S# CS |  Request for multiple status |

The last command can aquire the state of multiple relays and inputs, the structure of the command is:
- **0B**: identificator byte;
- **R#**: relays mask byte, if bit is set to 1, then the status of the respective relay will be returned;

| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| - | - | - | - | Relay 4 | Relay 3 | Relay 2 | Relay 1 |

- **S#**: switch (or inputs) mask byte, if bit is set to 1, then the status of the respective switch will be returned;

| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| - | - | - | - | - | Bottom switch | Top switch | Functional mode |

- **CS**: simple check sum, aquired by summing all the previous bytes os the message.

The response for the simple getting commands are the first 2 bytes from the request, plus a byte for the status 0x01 if opened or 0x00 for closed (0x00 in case of connection), plus 0x06 byte (ACK) and the new CheckSum.

The response to the multiple status request is:
- **0B**: identificator byte;
- **R#**: relays mask byte, if bit is set to 1, then the status of the respective relay will be returned;
- **S#**: switch (or inputs) mask byte, if bit is set to 1, then the status of the respective switch will be returned;
- **RS#**: relay status mask, if bit is set to 1 then relay is opened, if bit is set to 0 then relay is closed. The bit status for each relay respect the same position as the relay mask;
- **SS#**: switch (or input) status mask, if bit is set to 1 then input is low (or opened), if bit is set to 0 then input is high (or closed). The bit status for each switch respect the same position as the switch mask;
- **ACK**: 0x06, acknowledge byte in ASCII;
- **CS**: simple check sum, aquired by summing all the previous bytes os the message.

# Configuration commands accepted for both modes
| Command  |  Function  |
| --------- | --------- |
|  C0 00 X# X# CS* |  Set delay for relays |
|  C0 01 00 00 C1 |  Go to generic mode |
|  C0 01 00 01 C2 |  Go to optimized mode |

The **X#** bytes are the time in milliseconds for the optimized mode. The time is passed in two bytes, being the left the most significant one. The **CS*** byte is the last byte from simple CheckSum of the message.

The command for time configuration, has no CheckSum byte. The response for this command are the first 2 bytes, plus 2 bytes for the number of 200 ms interrupts the board will run before opening the relays.
The response for the mode commands are the first 3 bytes of the command, plus a 0x06 byte (ACK) and the new CheckSum.

# Response to unknown commands
If you send a specific optimized mode command, when the program is on generic mode, or the other way around, the Controllino will answer with a 0x15 byte (negative aknowledge in ASCII). If you send a valid command but with wrong CS, the answer will be the same.
