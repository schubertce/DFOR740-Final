
# Honeypot with FTP Server Emulation

## Project Description

This project simulates an FTP server using C++ and Boost.Asio to handle network operations. It is designed to mimic a simple FTP server environment, supporting basic commands like `LIST`, `CD`, `MKDIR`, `RETR`, and `STOR`. The server uses a simulated filesystem to safely emulate directory changes and file operations without accessing the actual filesystem. This makes it suitable for educational purposes, software testing, or network protocol training.

This tool was developed as a final exam project for a graduate course (DFOR740-Advanced Offensive and Defensive Strategies) for the Digital Forensics program at George Mason University under the supervision of Professor Gordon Long.

## Features

- Simulates basic FTP commands.
- Virtual filesystem to manage directories and file operations.
- Detailed logging of client-server interactions.
- Customizable port and directory settings.

## Getting Started

### Prerequisites

- Microsoft Visual Studio 2019 or later
- C++17 support
- Boost libraries (specifically Boost.Asio)

### Installation

1. **Clone the repository:**

   Open a command prompt or PowerShell, and run:
   ```
   git clone https://github.com/schubertce/DFOR740-Final.git
   ```

2. **Open the project in Visual Studio:**

   - Open Visual Studio.
   - Select `File > Open > Project/Solution` and navigate to the cloned project folder.
   - Open the project file (`.sln`).

3. **Configure Boost libraries:**

   - Right-click on your project in the Solution Explorer.
   - Go to `Properties > C/C++ > General > Additional Include Directories`.
   - Add the path to your Boost installation include folder.
   - Go to `Linker > General > Additional Library Directories`.
   - Add the path to your Boost installation lib folder.

4. **Build the project:**

   - Right-click on the solution in the Solution Explorer and select `Build`.

### Usage

Run the server directly from Visual Studio by setting command arguments in:
- `Project > Properties > Configuration Properties > Debugging > Command Arguments`
Enter the port and log file path here. Example:
```
8080 C:\path\to\log.txt
```
Press `F5` to start debugging and run the server.

Connect to the server using any standard FTP client by specifying the host as `localhost` and the port as configured.


Alternatively, I've included the prebuilt executable in the root of this github repository which can be downloaded and run as is.

Simply navigate to the directory where the exe is located and run the following command:
- ```FinalProject <port1> <port2>...<log_file_location>```

Example code:
```C:\Dev> FinalProject 21 22 443 C:\Dev\log.txt```

