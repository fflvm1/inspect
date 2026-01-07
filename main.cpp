#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include <cstring>
#include <vector>
#include <string>

std::string utilver = "1.2";    // inspect version
bool nerd = false;  // Nerd mode status
bool json = false;  // JSON format switch

// Info grabber
struct PathData {
    const char* path;   // Path to file/directory/whatever
    std::string fileType;   // Extracted file type (regular, directory, etc.)
    struct stat info;    // Info getter
    off_t file_size; // Extracted size
    bool r_user;    // Can user read file/directory/whatever?
    bool r_group;   // Can group read file/directory/whatever?
    bool r_other;   // Can other read file/directory/whatever?
    bool w_user;    // Can user write to file/directory/whatever?
    bool w_group;   // Can group write to file/directory/whatever?
    bool w_other;   // Can other write to file/directory/whatever?
    bool e_user;    // Can user execute file/directory/whatever?
    bool e_group;   // Can group execute file/directory/whatever?
    bool e_other;   // Can other execute file/directory/whatever?
    bool readable;  // Is it readable (at least one of the three should be able to read it)
    bool writable;  // Is it writeable (at least one of the three should be able to write to it)
    bool executable;    // Is it executable (at least one of the three should be able to execute it)
    bool isLast;    // Is this path is the last in the queue?
    uid_t UID;  // User ID
    struct passwd* pw;  // Password to grab username via UID
    gid_t GID;  // Group ID
    struct group* gr;   // Password to grab group name via GID
    std::string atime;  // Access time
    std::string mtime;  // Modify time
    std::string ctime;  // Status time
    
    // Printing info
    void printInfo() {
        std::cout << "Inspecting: " << path << std::endl;   // Show inspected path
        std::cout << "Type: " << fileType << std::endl; // Show type from path
        // Annotate size, to ensure that there is no confusion between directory and contents sizes (in case of a directory)
        std::cout << "Size" << ((fileType == "directory") && (!nerd) ? " (directory entry): " : ": ");
        
        // Show the size in bytes no matter what if using nerd mode
        if (nerd) {
            std::cout << file_size << "\n";
            // Show the sizes and convert them from bytes if not using Nerd Mode
        } else {
            if (file_size >= 1000000000000LL) {
                std::cout << file_size / 1000000000000.0 << " TB (" << file_size << " bytes)\n";
            }   else if (file_size >= 1000000000LL) {
                std::cout << file_size / 1000000000.0 << " GB (" << file_size << " bytes)\n";
            }   else if (file_size >= 1000000LL) {
                std::cout << file_size / 1000000.0 << " MB (" << file_size << " bytes)\n";
            }   else if (file_size >= 1000LL) {
                std::cout << file_size / 1000.0 << " KB (" << file_size << " bytes)\n";
            }   else {
                std::cout << file_size << " bytes\n";
            }
        }
        
        // List the permissions in rwxr-xr-x style
        std::cout << "Permissions: ";
        std::cout << (r_user ? "r" : "-");
        std::cout << (w_user ? "w" : "-");
        std::cout << (e_user ? "x" : "-");
        std::cout << (r_group ? "r" : "-");
        std::cout << (w_group ? "w" : "-");
        std::cout << (e_group ? "x" : "-");
        std::cout << (r_other ? "r" : "-");
        std::cout << (w_other ? "w" : "-");
        std::cout << (e_other ? "x" : "-");
        std::cout << "\n";
        
        // Only show info if user is not using Nerd Mode
        if (!nerd) {
            // Show if the object is readable and by who (hidden in Nerd Mode)
            std::cout << "Readable: " << (readable ? "yes" : "no");
            if (readable) {
                std::cout << " (";
                bool first = true;
                
                if (r_user)  { std::cout << (first ? "" : ", ") << "user";  first = false; }
                if (r_group) { std::cout << (first ? "" : ", ") << "group"; first = false; }
                if (r_other) { std::cout << (first ? "" : ", ") << "other"; }
                
                std::cout << ")";
            }
            
            std::cout << "\n";
            
            // Show if the object is writable and by who (hidden in nerd mode)
            std::cout << "Writeable: " << (writable ? "yes" : "no");
            if (writable) {
                std::cout << " (";
                bool first = true;
                
                if (w_user)  { std::cout << (first ? "" : ", ") << "user";  first = false; }
                if (w_group) { std::cout << (first ? "" : ", ") << "group"; first = false; }
                if (w_other) { std::cout << (first ? "" : ", ") << "other"; }
                
                std::cout << ")";
            }
            
            std::cout << "\n";
            
            // Show if the object is executable and by who (hidden in nerd mode)
            std::cout << "Executable: " << (executable ? "yes" : "no");
            if (executable) {
                std::cout << " (";
                bool first = true;
                
                if (e_user)  { std::cout << (first ? "" : ", ") << "user";  first = false; }
                if (e_group) { std::cout << (first ? "" : ", ") << "group"; first = false; }
                if (e_other) { std::cout << (first ? "" : ", ") << "other"; }
                
                std::cout << ")";
            }
            std::cout << "\n";
        }
        
        // Show the owner name if possible
        if (pw) {
            std::cout << "Owner: " << pw->pw_name
            << " (UID " << UID << ")\n";
        } else {
            std::cout << "Owner: <unknown>"
            << " (UID " << UID << ")\n";
        }
        
        // Show the group name if possible
        if (gr) {
            std::cout << "Group: " << gr->gr_name
            << " (GID " << GID << ")\n";
        } else {
            std::cout << "Group: <unknown>"
            << " (GID " << GID << ")\n";
        }
        
        // Get access time and show
        std::cout << (nerd ? "atime: " : "Accessed: ") << atime;
        
        // Get modified time and show
        std::cout << (nerd ? "mtime: " : "Modified: ") << mtime;
        
        // Get status change time and show
        std::cout << (nerd ? "ctime: " : "Status Change: ") << ctime;
    }
    
    // JSON-formatted printing
    void printJson() {
        std::cout << "    {\n";
        std::cout << "      \"path\": \"" << path << "\",\n";
        std::cout << "      \"type\": \"" << escapeJson(fileType) << "\",\n";
        std::cout << "      \"size\": " << file_size << ",\n";
        std::cout << "      \"uid\": " << UID << ",\n";
        std::cout << "      \"gid\": " << GID << "\n";
        std::cout << "    }";

        // Add a comma if there is another path in the queue (for proper JSON formatting)
        if (!isLast) {
            std::cout << ",";
        }

        std::cout << "\n";
    }
    
    // Changing some problematic characters in the path to ensure that the JSON path isn't broken
    std::string escapeJson(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;
            }
        }
        return out;
    }

    // Getting file type
    std::string getFileType() {
        // Is a regular file?
        if (S_ISREG(info.st_mode)) {
            return "file";
            // Is a directory?
        }   else if (S_ISDIR(info.st_mode)) {
            return "directory";
            // Is a symlink?
        }   else if (S_ISLNK(info.st_mode)) {
            return "symlink";
            // If it's some other type
        }   else {
            return "other";
        }
    }
    
    // Constructor
    PathData(const char* p, bool l) : path(p), isLast(l) {
        // Trying to gather info from path
        if (lstat(path, &info) != 0) {
            // Throwing an error if the path is inaccessible
            std::cerr << "inspect: cannot access " << path << ": "
            << std::strerror(errno) << "\n";
            // Throwing an error to ensure that the utility stops here
            throw std::runtime_error("access denied.");
           }
        
        // Gathering info from path
        fileType = getFileType();   // Type
        file_size = info.st_size; // Size
        r_user = info.st_mode & S_IRUSR;    // Readable by user?
        r_group = info.st_mode & S_IRGRP;   // Readable by group?
        r_other = info.st_mode & S_IROTH;   // Readable by other?
        w_user = info.st_mode & S_IWUSR;    // Writable by user?
        w_group = info.st_mode & S_IWGRP;   // Writable by group?
        w_other = info.st_mode & S_IWOTH;   // Writable by other?
        e_user = info.st_mode & S_IXUSR;    // Executable by user?
        e_group = info.st_mode & S_IXGRP;   // Executable by group?
        e_other = info.st_mode & S_IXOTH;   // Executable by other?
        readable = r_user || r_group || r_other;    // Can be read by anyone?
        writable = w_user || w_group || w_other;    // Can be written to by anyone?
        executable = e_user || e_group || e_other;  // Can be executed by anyone?
        UID = info.st_uid;  // User ID
        pw = getpwuid(UID); // Grabbing owner name via UID
        GID = info.st_gid;  // Group ID
        gr = getgrgid(GID); // Grabbing group name via GID
        // Access, modification and general status time
        atime = std::ctime(&info.st_atime);
        mtime = std::ctime(&info.st_mtime);
        ctime = std::ctime(&info.st_ctime);
        
        if (!json) {
            printInfo();    // Print regular style info
        }   else {
            printJson();    // Print info in JSON style
        }
    }
};

// Showing the inspect wordmark
void showBanner() {
    std::cout << "-- i n s p e c t. --\n";
}

int main(int argc, char * argv[]) {
    // Print usage guide if less then two arguments are given by the user and quit abruptly
    if (argc < 2) {
        showBanner();
        std::cerr << "Usage: inspect <path/s>\n";
        return 1;
    }
    
    // Define arguments
    struct option long_options[] = {
            {"ver", no_argument, nullptr, 'v'}, // Version argument
            {"help", no_argument, nullptr, 'h'},    // Help argument
            {"nerd", no_argument, nullptr, 'n'},    // Nerd argument
            {"json", no_argument, nullptr, 'j'},    // JSON argument
            {0,0,0,0}
        };

        int opt;    // Argument
        int option_index = 0;   // Argument index
        bool shownBanner = false;   // Whether the banner was shown or not

        // Argument code
        while ((opt = getopt_long(argc, argv, "vhnj", long_options, &option_index)) != -1) {
            switch (opt) {
                case 'v':   // Version argument
                    showBanner();
                    std::cout << "by Vova Flare\n";
                    std::cout << "version " << utilver << "\n";
                    std::cout << "a basic POSIX utility for inspecting file metadata.\n";
                    std::cout << "made in Xcode on a trusty iMac 13,1 from 2012.\n";
                    std::cout << "github: https://github.com/fflvm1/inspect\n";
                    return 0;
                case 'h':   // Help argument
                    showBanner();
                    std::cout << "Usage: inspect <path/s> [arguments]\n\n";
                    std::cout << "Options:\n";
                    std::cout << "  -v, --ver       Show version information\n";
                    std::cout << "  -h, --help      Show this help message\n";
                    std::cout << "  -n, --nerd      Hide redundant info for power users\n";
                    std::cout << "  --json          Print machine-readable JSON to standard output\n\n";
                    std::cout << "JSON output:\n";
                    std::cout << "  To save JSON to a file:\n";
                    std::cout << "   inspect <path/s> --json > output.json\n";

                    return 0;
                case 'n':   // Nerd argument
                    nerd = true;    // Enable nerd mode
                    break;
                case 'j':   // JSON argument
                    json = true;    // Remember to display text in the JSON format
                    break;
                default:    // Invalid argument
                    showBanner();
                    std::cerr << "inspect: unknown option\n";
                    return 1;
            }
            shownBanner = true; // Mark banner as shown so that it doesn't show again
        }
    
    // Show the banner if it was never shown before
    if (!shownBanner) {
        showBanner();
        shownBanner = true;
    }
    
    // Show the enabling message for Nerd Mode if not using JSON formatting
    if (nerd && !json) {
        showBanner();
        shownBanner = true;
        std::cout << "inspect: enabled 'nerd mode'.\n";
        std::cout << "------\n";
    }
    
    // Create array storing every path in the prompt
    std::vector<char*> paths;
    
    // Add all found paths to an array
    for (int i = optind; i < argc; i++) {
        paths.emplace_back(argv[i]);
    }
    
    // Add basic info about inspect to the JSON (if using JSON format)
    if (json) {
        std::cout << "{\n";
        std::cout << "  \"tool\": \"inspect\",\n";
        std::cout << "  \"tool_version\": \"" << utilver << "\",\n";
        std::cout << "  \"schema_version\": 1,\n";
        std::cout << "  \"data\": [\n";
    }
    
    // Run through each found path
    for (int i = 0; i < paths.size(); i++) {
        bool isLast = (i == paths.size() - 1);

        try {
            PathData pd(paths[i], isLast);  // Get data from path
        } catch (const std::exception&) {
            return 1;   // Show error code 1 if there was an error extracting data
        }

        // Some nice formatting to separate paths if more than 1 are inspected
        if (!json && !isLast) {
            std::cout << "------\n";
        }
    }
    
    // Add necessary brackets to end the JSON (if using JSON formatting)
    if (json) {
        std::cout << "  ]\n";
        std::cout << "}\n";
    }
    
    return 0;
}
