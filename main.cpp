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

std::string utilver = "1.3";    // inspect version
bool nerd = false;  // Nerd mode status
bool json = false;  // JSON format switch
bool skip = false;  // Skip problematic paths
bool line = false;  // Line mode

// Info grabber
struct PathData {
    const char* path;   // Path to file/directory/whatever
    std::string type;   // Extracted type (regular file, directory, symlink, other)
    struct stat info;    // Info getter
    off_t raw_size; // Extracted size
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
    bool writable;  // Is it writable (at least one of the three should be able to write to it)
    bool executable;    // Is it executable (at least one of the three should be able to execute it)
    bool isLast;    // Is this path is the last in the queue?
    bool accessError = false;   // Error retrieving info from path
    uid_t UID;  // User ID
    struct passwd* pw;  // Password to grab username via UID
    gid_t GID;  // Group ID
    struct group* gr;   // Password to grab group name via GID
    std::string atime;  // Access time
    std::string mtime;  // Modify time
    std::string ctime;  // Status time
    std::string owner;  // Owner name (if found)
    std::string group;  // Group name (if found)
    std::string perms;  // Permissions string
    int perm_mode;  // Permissions mode
    
    // Printing info
    void printInfo() {
        std::cout << "Inspecting: " << path << std::endl;   // Show inspected path
        
        std::cout << "Type: " << type << std::endl; // Show type of path
        
        // Annotate size, to ensure that there is no confusion between directory and contents sizes (in case of a directory)
        std::cout << "Size" << ((type == "directory") && (!nerd) ? " (directory entry): " : ": ");
        
        std::cout << getSizeString() << "\n";   // Show size
        
        // List the permissions in rwxr-xr-x style
        std::cout << "Permissions: " << perms << "\n";
        
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
            std::cout << "Writable: " << (writable ? "yes" : "no");
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
            std::cout << "Owner: " << owner
            << " (UID " << UID << ")\n";
        
        // Show the group name if possible
            std::cout << "Group: " << group
            << " (GID " << GID << ")\n";
        
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
        std::cout << "      \"type\": \"" << escapeJson(type) << "\",\n";
        std::cout << "      \"size\": " << getSizeString() << ",\n";
        std::cout << "      \"permissions\": \"" << perms << "\",\n";
        std::cout << "      \"mode\": " << perm_mode << ",\n";
        std::cout << "      \"uid\": " << UID << ",\n";
        std::cout << "      \"gid\": " << GID << ",\n";
        std::cout << "      \"atime\": " << info.st_atime << ",\n";
        std::cout << "      \"mtime\": " << info.st_mtime << ",\n";
        std::cout << "      \"ctime\": " << info.st_ctime << "\n";
        std::cout << "    }";
    }
    
    // Line mode info printing
    void printHorizontal() {
        std::cout << path << "  "
        << type << "  "
        << getSizeString() << "  "
        << perms << "   "
        << owner << " ("
        << UID << ")  "
        << group << " ("
        << GID << ")  "
        << formatTime(info.st_mtime, "%Y-%m-%d %H:%M") << "\n";
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
    std::string getType() {
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
    
    // Formatting size (e.g. 16 KB, 16K, etc)
    std::string getSizeString() {
        std::string SizeInString = std::to_string(raw_size);    // Covert raw size to a string
        
        // Return size in bytes if using nerd mode or JSON formatting without adding size format (e.g. MB or M)
        if (nerd || json)
            return SizeInString;
        
        // Convert size from bytes to... and add the size format in the end (e.g. MB or M)
        if (raw_size >= 1000000000000LL) {  // TB / T
            SizeInString = std::to_string(raw_size / 1000000000000.0) + (line ? "T" : " TB (" + std::to_string(raw_size) + " bytes)");
        }   else if (raw_size >= 1000000000LL) {    // GB / G
            SizeInString = std::to_string(raw_size / 1000000000.0) + (line ? "G" : " GB (" + std::to_string(raw_size) + " bytes)");
        }   else if (raw_size >= 1000000LL) {   // MB / M
            SizeInString = std::to_string(raw_size / 1000000.0) + (line ? "M" : " MB (" + std::to_string(raw_size) + " bytes)");
        }   else if (raw_size >= 1000LL) {  // KB / K
            SizeInString = std::to_string(raw_size / 1000.0) + (line ? "K" : " KB (" + std::to_string(raw_size) + " bytes)");
        }   else {  // bytes / B
            SizeInString = std::to_string(raw_size) + (line ? "B" : " bytes");
        }
        
        return SizeInString;    // Return complete string
    }

    // Formatting time based on provided format (POSIX, thread-safe)
    std::string formatTime(time_t t, const char* format) {
        std::tm tm{};
        localtime_r(&t, &tm);

        char buffer[64];
        if (std::strftime(buffer, sizeof(buffer), format, &tm)) {
            return std::string(buffer);
        }
        return {};
    }
    
    // Constructor
    PathData(const char* p, bool l) : path(p), isLast(l) {
        // Trying to gather info from path
        if (lstat(path, &info) != 0) {
            accessError = true; // Marking the path as not openable
            if (!skip) {    // Stopping the util from closing if using--continue-on-error argument
                // Throwing an error if the path is inaccessible
                std::cerr << "inspect: cannot access " << path << ": "
                << std::strerror(errno) << "\n";
                // Throwing an error to ensure that the utility stops here
                throw std::runtime_error("access denied.");
            }
        }
        
        // Gathering info from path
        type = getType();   // Type
        raw_size = info.st_size; // Size
        r_user = info.st_mode & S_IRUSR;    // Readable by user?
        r_group = info.st_mode & S_IRGRP;   // Readable by group?
        r_other = info.st_mode & S_IROTH;   // Readable by other?
        w_user = info.st_mode & S_IWUSR;    // Writable by user?
        w_group = info.st_mode & S_IWGRP;   // Writable by group?
        w_other = info.st_mode & S_IWOTH;   // Writable by other?
        e_user = info.st_mode & S_IXUSR;    // Executable by user?
        e_group = info.st_mode & S_IXGRP;   // Executable by group?
        e_other = info.st_mode & S_IXOTH;   // Executable by other?
        // Same permissions info in rwxr-xr-x style
        perms += (r_user ? "r" : "-");
        perms += (w_user ? "w" : "-");
        perms += (e_user ? "x" : "-");
        perms += (r_group ? "r" : "-");
        perms += (w_group ? "w" : "-");
        perms += (e_group ? "x" : "-");
        perms += (r_other ? "r" : "-");
        perms += (w_other ? "w" : "-");
        perms += (e_other ? "x" : "-");
        readable = r_user || r_group || r_other;    // Can be read by anyone?
        writable = w_user || w_group || w_other;    // Can be written to by anyone?
        executable = e_user || e_group || e_other;  // Can be executed by anyone?
        perm_mode = info.st_mode & 0777;    // Get permissions mode (JSON-only)
        UID = info.st_uid;  // User ID
        pw = getpwuid(UID); // Grabbing owner name via UID
        GID = info.st_gid;  // Group ID
        gr = getgrgid(GID); // Grabbing group name via GID
        // Find the owner name if possible via UID
        if (pw) {
            owner = pw->pw_name;
        } else {
            owner = "<unknown>";    // Fallback
        }
        // Find the group name if possible via GID
        if (gr) {
            group = gr->gr_name;
        } else {
            group = "<unknown>";    // Fallback
        }
        // Access, modification and general status time
        atime = std::ctime(&info.st_atime);
        mtime = std::ctime(&info.st_mtime);
        ctime = std::ctime(&info.st_ctime);
    }
};

// Showing the inspect wordmark
void showBanner() {
    std::cout << "-- i n s p e c t. --\n";
}

int main(int argc, char * argv[]) {
    // Print usage guide if less then two arguments are given by the user and quit abruptly
    if (argc < 2) {
        std::cerr << "Usage: inspect <path/s>\n";
        return 1;
    }
    
    // Define arguments
    struct option long_options[] = {
            {"ver", no_argument, nullptr, 'v'}, // Version argument
            {"help", no_argument, nullptr, 'h'},    // Help argument
            {"nerd", no_argument, nullptr, 'n'},    // Nerd argument
            {"json", no_argument, nullptr, 'j'},    // JSON argument
            {"continue-on-error", no_argument, nullptr, 'c'},   // Skip problematic paths
            {"line", no_argument, nullptr, 'l'},    // Line argument
            {0,0,0,0}
        };

        int opt;    // Argument
        int option_index = 0;   // Argument index

        // Argument code
        while ((opt = getopt_long(argc, argv, "vhnjcl", long_options, &option_index)) != -1) {
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
                    std::cout << "Usage: inspect <path/s> [arguments]\n\n";
                    std::cout << "Arguments:\n";
                    std::cout << "  -v, --ver       Show version information\n";
                    std::cout << "  -h, --help      Show this help message\n";
                    std::cout << "  -n, --nerd      Hide redundant info for power users\n";
                    std::cout << "  -j, --json      Print machine-readable JSON to standard output\n";
                    std::cout << "  -l, --line      Print info in horizontal, one line style\n\n";
                    std::cout << "JSON output:\n";
                    std::cout << "  To save JSON to a file:\n";
                    std::cout << "    inspect <path/s> --json > output.json\n";
                    std::cout << "Error handling:\n";
                    std::cout << "  -c, --continue-on-error";
                    std::cout << "      Continue processing remaining paths if one fails.\n                         ";
                    std::cout << "      By default, inspect exits immediately on the first error.\n";
                    return 0;
                case 'n':   // Nerd argument
                    nerd = true;    // Enable nerd mode
                    break;
                case 'j':   // JSON argument
                    json = true;    // Remember to display text in the JSON format
                    break;
                case 'c':   // Continue on error argument
                    skip = true;    // Enable skipping unopenable paths
                    break;
                case 'l':   // Line argument
                    line = true;    // Enable line mode
                    break;
                default:    // Invalid argument
                    return 1;
            }
        }
    
    // Create array storing every path in the prompt
    std::vector<char*> paths;
    
    // Add all found paths to an array
    for (int i = optind; i < argc; i++) {
        paths.emplace_back(argv[i]);
    }
    
    std::vector<PathData> datalist; // Array storing data for each path

    // Add data of each path to the database
    try {
        for (std::size_t i = 0; i < paths.size(); ++i) {
            bool isLast = (i + 1 == paths.size());  // Last path in the database?
            datalist.emplace_back(paths[i], isLast);
        }
    // Kill the utility if any path is inaccessible
    } catch (const std::exception&) {
            return 1;
    }
    
    // Add basic info about inspect to the JSON (if using JSON format)
    if (json) {
        std::cout << "{\n";
        std::cout << "  \"tool\": \"inspect\",\n";
        std::cout << "  \"tool_version\": \"" << utilver << "\",\n";
        std::cout << "  \"schema_version\": 2,\n";
        std::cout << "  \"data\": [\n";
    }

    std::vector<std::string> problematicPaths;  // Array storing problematic path
    
    // Print data of each path
    for (std::size_t i = 0; i < datalist.size(); i++) {
        // Adding path to the problematic array if any issues were found
        if (skip && datalist[i].accessError) {
            problematicPaths.emplace_back(datalist[i].path);
            
            // Showing skipping warning if a path is unopenable (--continue-on-error)
            if (!json && !line) {
                std::cerr << "inspect: skipping problematic path (" << datalist[i].path << ")\n";
                std::cout << (!datalist[i].isLast ? "------\n" : "");
            }
            continue;
        }
        
        if (json) {
            // JSON format (if --json argument is used)
            datalist[i].printJson();
        }   else if (line) {
            // Line format (if --line argument is used)
            datalist[i].printHorizontal();
        }   else {
            // Regular format
            datalist[i].printInfo();
        }

        // Visually separating data if multiple paths exist
        if ((!json && !line) && !datalist[i].isLast) {
            std::cout << "------\n";
        }   else if (json) { // Add a comma if there is another path in the queue (JSON formatting)
            if (i + 1 < datalist.size())
                // Verify if the file is the last and if it's openable (also used for --continue-on-error)
                std::cout << ((!datalist[i + 1].accessError || !datalist[i + 1].isLast) ? ",\n" : "\n");
            else
                std::cout << "\n";
        }
}
    
    // Add necessary brackets to end the JSON
    if (json) {
        std::cout << "  ]\n";
        std::cout << "}\n";
    }
    
    // Showing problematic paths (JSON and Line modes)
    if (json || line) {
        // If any problematic paths were found
        if (problematicPaths.size() > 0) {
            if (json)
                std::cerr << "\n";
            
            // Search for and display every problematic path at the end
            for (size_t i = 0; i < problematicPaths.size(); i++) {
                std::cerr << "inspect: skipped problematic path (" << problematicPaths[i] << ")\n";
            }
            return 1;   // Exit code 1
        }
    }   else {  // If bot using JSON mode or Line mode
            if (problematicPaths.size() > 0)
                return 1;   // Ensure that exiting code is 1
    }
    
    return 0;
}
