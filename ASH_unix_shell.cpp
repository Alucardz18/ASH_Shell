#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>   

using namespace std;

//  Error message 
const char error_message[] = "An error has occurred\n";
const int ERROR_MESSAGE_LEN = sizeof(error_message) - 1;
void printError() { write(STDERR_FILENO, error_message, ERROR_MESSAGE_LEN); }

// PATH directories
vector<string> pathDirs = { "/bin" };   

// display prompt
void printAsh() {
    char cwd[1024];
    const char* home_path_cstr = getenv("HOME");
    string home_dir = (home_path_cstr != nullptr) ? string(home_path_cstr) : "";

    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        string current_dir(cwd);
        if (!home_dir.empty() && current_dir == home_dir) cout << "ash > ";
        else  cout << "ash " << current_dir << " > ";
    }
    else {
        printError();
        cout << "ash > ";
    }
    cout.flush();
}

// preprocess input line 
static inline string preprocess(const string& s) {
    string out; out.reserve(s.size() + 4);
    for (char c : s) out += (c == '>') ? " > " : string(1, c);
    return out;
}
// split string by whitespace
vector<string> splitWS(const string& input) {
    stringstream ss(input);
    string tok; vector<string> tokens;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

// find executable in PATH directories
string findExecutable(const string& cmd) {
    for (const auto& dir : pathDirs) {
        string full = dir;
        if (!full.empty() && full.back() != '/') full += '/';
        full += cmd;
        if (access(full.c_str(), X_OK) == 0) return full;
    }
    return "";
}

// built-in command handler
bool handleBuiltin(const vector<string>& tokens) {
    if (tokens.empty()) return true;

    const string& cmd = tokens[0];
    if (cmd == "exit" || cmd == "EXIT") {
        if (tokens.size() > 1) { printError(); return true; }
        cout << "Nii Oye Kpakpo\nAggie Shell Project\n";
        exit(0);
    }
    else if (cmd == "cd") {
        if (tokens.size() != 2) { printError(); return true; }
        string target = tokens[1];

        if (target == "~") {
            const char* home = getenv("HOME");
            if (!home) { printError(); return true; }
            target = home;
        }
        else if (target.size() > 1 && target[0] == '$') {
            string var = target.substr(1);
            const char* val = getenv(var.c_str());
            if (!val) { printError(); return true; }
            target = val;
        }

        if (chdir(target.c_str()) != 0) { printError(); }
        return true;
    }
    else if (cmd == "path") {
        pathDirs.clear();
        for (size_t i = 1; i < tokens.size(); ++i) pathDirs.push_back(tokens[i]);
        return true;
    }
    return false; 
}

// execute external command
bool executeExternal(vector<string> tokens) {
    if (tokens.empty()) return true;

    bool hasRedirect = false; string outFile;
    int gtIndex = -1;
    for (int i = 0; i < (int)tokens.size(); ++i) {
        if (tokens[i] == ">") {
            if (gtIndex != -1) { printError(); return true; } 
            gtIndex = i;
        }
    }
	// Output Redirection Check
    if (gtIndex != -1) {
		// '>' must be followed by exactly one filename
        if (gtIndex == (int)tokens.size() - 1) { printError(); return true; } 
        outFile = tokens[gtIndex + 1];
		// there must be only one token after '>'
        if (gtIndex + 2 != (int)tokens.size()) { printError(); return true; } 
        hasRedirect = true;
        tokens.resize(gtIndex);
		// after resizing, there must be at least one token (the command)
        if (tokens.empty()) { printError(); return true; } 
    }
	// PATH must not be empty
    if (pathDirs.empty()) { printError(); return true; }

	// find executable
    const string& cmd = tokens[0];
    string execPath = findExecutable(cmd);
    if (execPath.empty()) { printError(); return true; }
    
    vector<char*> argv; argv.reserve(tokens.size() + 1);
    for (auto& s : tokens) argv.push_back(const_cast<char*>(s.c_str()));
    argv.push_back(nullptr);

	// External Command Execution. fork and exec
	// In child: handle redirection if needed, then execv
	// In parent: wait for child to finish
    pid_t pid = fork();
    if (pid < 0) { printError(); return true; }
    else if (pid == 0) {
        if (hasRedirect) {
            int fd = open(outFile.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
            if (fd < 0) { printError(); _exit(1); }
            if (dup2(fd, STDOUT_FILENO) < 0) { printError(); _exit(1); }
            close(fd);
        }
        execv(execPath.c_str(), argv.data());
        printError(); _exit(1);
    }
    else {
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) printError();
    }
    return true;
}

// execute a single line of input
inline void executeLine(const string& rawLine) {
    if (rawLine.empty()) return;
    string normalized = preprocess(rawLine);
    vector<string> tokens = splitWS(normalized);
    if (tokens.empty()) return;
	// handle built-in commands
    if (handleBuiltin(tokens)) return;
    executeExternal(tokens);
}


int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

	// argument check
	// Exits with an error if more than one command-line argument is provided
    if (argc > 2) {           
        printError();
        exit(1);
    }

    if (argc == 2) {
		// batch mode. Enters batch processing if a single argument (a filename) is provided
        ifstream fin(argv[1]);
		// Exits with an error if the specified batch file cannot be opened
        if (!fin.is_open()) {
            printError();
            exit(1);
        }
        string line;
        while (std::getline(fin, line)) {
            // Skips printing the prompt during file-based execution
            // Reads and executes each line from the batch file sequentially
            if (!line.empty()) executeLine(line);
        }
        // Exits successfully after reading all lines from the batch file
        exit(0);
    }

	// interactive mode. Enters the main loop for accepting user input from the console.
    string input;
    while (true) {
        printAsh();                 
        if (!getline(cin, input)) { 
            exit(0);
        }
        executeLine(input);
    }
}

