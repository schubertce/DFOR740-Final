#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <csignal>
#include <vector>
#include <sstream>
#include <regex>
#include <map>

using boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace chrono = std::chrono;

asio::io_service io_service;

class SimulatedFS {
    std::string currentDirectory;
    std::map<std::string, std::vector<std::string>> directories;

public:
    SimulatedFS() : currentDirectory("/") {
        // Initialize root directory
        directories[currentDirectory];

        // Predefined directories
        std::vector<std::string> predefinedDirs = { "Data", "Incoming", "Outgoing" };
        for (const auto& dir : predefinedDirs) {
            std::string fullPath = currentDirectory + dir; // Create full path for each directory
            directories[fullPath];  // Create the directory in the map
        }
    }

    // Normalize path to handle various path inputs
    std::string normalizePath(const std::string& inputPath) {
        if (inputPath.empty() || inputPath == ".") {
            return currentDirectory;
        }
        if (inputPath == "..") {
            // Handle going up to the parent directory, but never above root
            size_t pos = currentDirectory.rfind('/');
            if (pos != std::string::npos && pos != 0) {
                return currentDirectory.substr(0, pos);
            }
            return "/";  // Root directory
        }

        // Build absolute path if not already absolute
        return inputPath[0] == '/' ? inputPath : currentDirectory + "/" + inputPath;
    }

    std::string cd(const std::string& path) {
        std::string normalizedPath = normalizePath(path);
        if (normalizedPath == "..") {
            // Simplify to navigate to the parent directory
            if (currentDirectory != "/") {
                size_t pos = currentDirectory.find_last_of('/');
                currentDirectory = currentDirectory.substr(0, pos == 0 ? 1 : pos);
            }
        }
        else if (directories.find(normalizedPath) != directories.end()) {
            currentDirectory = normalizedPath;
            return "250 Directory successfully changed.\r\n";
        }
        else {
            return "550 Failed to change directory: Directory does not exist.\r\n";
        }
        return "250 Directory successfully changed.\r\n";
    }

    std::string mkdir(const std::string& dir) {
        std::string normalizedPath = normalizePath(dir);
        if (directories.find(normalizedPath) == directories.end()) {
            directories[normalizedPath];
            return "257 \"" + normalizedPath + "\" directory created.\r\n";
        }
        return "550 Directory already exists.\r\n";
    }

    std::string retr(const std::string& filename) {
        // Assume file exists for emulation
        return "150 Opening binary mode data connection.\r\n226 Transfer complete.\r\n";
    }

    std::string stor(const std::string& filename) {
        // Assume file stored successfully
        return "150 Ok to send data.\r\n226 Transfer complete.\r\n";
    }

    std::string getCurrentDirectory() {
        return currentDirectory;
    }

};

SimulatedFS fs;

void signalHandler(int signum) {
    std::cout << "Interrupt signal received. Exiting..." << std::endl;
    
    // Stop the io_service. This will cause the server's loop to exit
    io_service.stop();
}

// Function to get current timestamp as a string
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    char timeString[26];
    ctime_s(timeString, sizeof(timeString), &now_time_t);

    return std::string(timeString);
}

// Helper function to trim strings (removes leading and trailing whitespace)
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::string simulateListCommand() {
    std::stringstream listOutput;
    // Header for the data connection opened
    listOutput << "150 Opening ASCII mode data connection for file list.\r\n";

    // Fake directory content
    listOutput << "drwxr-xr-x 7 user group 4096   Mar 01 12:34 Data\n";
    listOutput << "drwxr-xr-x 7 user group 4096   Mar 01 12:34 Incoming\n";
    listOutput << "drwxr-xr-x 7 user group 4096   Mar 01 12:34 Outgoing\n";
    listOutput << "-rw-r--r-- 1 user group 204800 Apr 29 08:37 Client_PoCs.xlsx\n";
    listOutput << "-rw-r--r-- 1 user group 204800 Mar 01 12:34 Client_Billable_Hours.xlsx\n";
    listOutput << "-rw-r--r-- 1 user group 102400 May 03 10:17 log.txt\n";
    listOutput << "-rw-r--r-- 1 user group 204800 Feb 28 12:02 ReportTemplate.docx\n";

    // Footer for the data transfer completion
    listOutput << "226 Transfer complete.\r\n";
    return listOutput.str();
}


// Function to log connection details
void logConnectionDetails(const tcp::socket& socket, const std::string& dataReceived, const std::string& logFilePath, unsigned short port) {
    std::ofstream logFile(logFilePath, std::ios::app | std::ios::out);
    if (logFile.is_open()) {
        logFile << "Connected Port: " << port << std::endl;
        logFile << "Timestamp: " << getCurrentTimestamp();
        logFile << "Source IP: " << socket.remote_endpoint().address().to_string() << std::endl;
        logFile << "Source Port: " << socket.remote_endpoint().port() << std::endl;
        logFile << "Received Data: " << dataReceived << std::endl;
        logFile << "-----------------------------------\n";
    }
    else {
        std::cerr << "Unable to open log file at " << logFilePath << std::endl;
    }
    
}

// Handles filesystem-related commands
std::string handleFSCommands(const std::string& command, const std::string& argument, SimulatedFS& fs) {
    if (command == "CD") {
        return fs.cd(argument);
    }
    else if (command == "MKDIR") {
        return fs.mkdir(argument);
    }
    else if (command == "RETR") {
        return fs.retr(argument);
    }
    else if (command == "STOR") {
        return fs.stor(argument);
    }
    else if (command == "LIST") {
        return simulateListCommand();
    }
    else if (command == "PWD") {
        return "257 \"" + fs.getCurrentDirectory() + "\" is the current directory.\r\n";
    }
    else {
        return "502 Command not implemented.\r\n";
    }
}


// Parses received data and determines an appropriate response
std::string parseCommand(const std::string& data, bool& loggedIn, SimulatedFS& fs, bool& shouldClose) {
    // \s* matches any whitespace characters
    // \S+ matches any non-whitespace character and the + sign requires one more characters
    // (.*) matches any character except a newline and the * means zero or more characters
    // \s*(\S+)\s* captures the COMMAND and  (.*)\s* will capture the arguement
    std::regex commandRegex(R"(\s*(\S+)\s*(.*)\s*)"); // Handles commands more flexibly
    std::smatch matches;
    std::string response;

    if (std::regex_search(data, matches, commandRegex)) {
        std::string command = matches[1].str();
        std::string argument = matches[2].str();
        std::transform(command.begin(), command.end(), command.begin(), ::toupper); // Normalize to uppercase

        if (command == "USER") {
            loggedIn = false; // Reset login state
            response = "331 Password required for " + argument + ".\r\n";
        }
        else if (command == "PASS") {
            if (!loggedIn) {
                loggedIn = true;  // Assume login success
                response = "230 User logged in, proceed.\r\n";
            }
            else {
                response = "503 Login with USER first.\r\n";
            }
        }
        else if (loggedIn && command == "HELP") {
            response =  "214-The following commands are recognized.\r\nUSER\r\nPASS\r\nCD\r\nMKDIR\r\nRETR\r\nSTOR\r\nPWD\r\nQUIT\r\nLIST\r\nHELP\r\n214 Help OK.\r\n";
        }
        else if (command == "QUIT" || command == "EXIT") {
            response = "221 Goodbye.\r\n";
            shouldClose = true;  // Signal that the connection should be closed after sending the response
            
        }
        else if (!loggedIn) {
            response = "530 Please log in with USER and PASS.\r\n";
        }
        else {
            response = handleFSCommands(command, argument, fs);
        }
    }
    else {
        response = "500 Syntax error, command unrecognized.\r\n";
    }
    return response + "ftp>";  // Append ftp prompt to all responses to simulate continuous session
}

// Function to handle connection
void handle_connection(tcp::socket socket, const std::string& logFilePath, unsigned short port) {
    bool loggedIn = false;  // Track login state
    bool shouldClose = false; // Flag to check if the connection should be closed
    try {
        std::string welcomeMessage = "220 Welcome to SchubertNet's FTP Server\nftp>";
        boost::asio::write(socket, boost::asio::buffer(welcomeMessage));

        while (!shouldClose) {
            char dataBuffer[1024] = { 0 };
            boost::system::error_code error;
            std::size_t length = socket.read_some(boost::asio::buffer(dataBuffer), error);

            if (error == boost::asio::error::eof) {
                std::cout << "Client disconnected." << std::endl;
                break; // Connection closed cleanly by peer.
            }
            else if (error) {
                throw boost::system::system_error(error); 
            }

            std::string receivedData(dataBuffer, length);
            std::cout << "Received: " << receivedData << std::endl; // Debug display

            // Log connection details
            logConnectionDetails(socket, receivedData, logFilePath, port);

            // Respond based on the parsed command
            std::string response = parseCommand(receivedData, loggedIn, fs, shouldClose);
            boost::asio::write(socket, boost::asio::buffer(response));

            if (shouldClose) {
                socket.close(); // Close the socket properly
                std::cout << "Connection closed by client command." << std::endl;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in connection handler: " << e.what() << std::endl;
    }
}


void startServers(boost::asio::io_service& io_service, const std::vector<unsigned short>& ports, const std::string& logFilePath) {
    std::vector<std::unique_ptr<boost::asio::ip::tcp::acceptor>> acceptors;
    for (unsigned short port : ports) {
        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
        auto acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(io_service, endpoint);
        std::cout << "Server is running on port " << port << std::endl;

        acceptors.push_back(std::move(acceptor));
    }

    while (!io_service.stopped()) {
        for (auto& acceptor : acceptors) {
            if (acceptor->is_open()) {
                tcp::socket socket(io_service);
                acceptor->accept(socket);
                std::thread(handle_connection, std::move(socket), logFilePath, acceptor->local_endpoint().port()).detach();
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: FinalProject <port1> <port2> ... <\\log\\file\\path.txt>" << std::endl;
        return 1;
    }

    std::vector<unsigned short> ports;
    for (int i = 1; i < argc - 1; ++i) {  // Skip the last argument for the log file path
        int port = std::stoi(argv[i]);
        ports.push_back(port);
    }

    std::string logFilePath = argv[argc - 1];  // Last argument is the log file path

    // Register signal SIGINT and signal handler  
    std::signal(SIGINT, signalHandler);

    try {
        startServers(io_service, ports, logFilePath);
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Servers stopped." << std::endl;
    return 0;
}

