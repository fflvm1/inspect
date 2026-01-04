#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>

int main(int argc, char * argv[]) {
    std::cout << "-- i n s p e c t. --\n";
    
    // Print usage guide if less then two arguments are given by the user and quit abruptly
    if (argc < 2) {
        std::cerr << "Usage: inspect <path>\n";
        return 1;
    }
    
    // Define arguments
    struct option long_options[] = {
            {"ver", no_argument, nullptr, 'v'}, // Version argument
            {"help", no_argument, nullptr, 'h'},    // Help argument
            {"nerd", no_argument, nullptr, 'n'},    // Nerd argument
            {0,0,0,0}
        };

        int opt;    // Argument
        int option_index = 0;   // Argument index
        bool nerd = false;  // Nerd mode status

        // Argument code
        while ((opt = getopt_long(argc, argv, "vhn", long_options, &option_index)) != -1) {
            switch (opt) {
                case 'v':   // Version argument
                    std::cout << "by Vova Flare\n";
                    std::cout << "version 1.0\n";
                    std::cout << "a basic POSIX utility for inspecting file metadata.\n";
                    std::cout << "made in Xcode on a trusty iMac 13,1 from 2012.\n";
                    std::cout << "github: https://github.com/fflvm1/inspect\n";
                    return 0;
                case 'h':   // Help argument
                    std::cout << "Usage: inspect <file> [options]\n";
                    std::cout << "Version: inspect -v or --ver\n";
                    std::cout << "Help: inspect -h or --h\n";
                    std::cout << "Hiding unecessary options: inspect <file> -n or --nerd\n";
                    return 0;
                case 'n':   // Nerd argument
                    nerd = true;    // Enable nerd mode
                    std::cout << "inspect: enabled 'nerd mode'.\n";
                    std::cout << "------\n";
                    break;
                default:    // Invalid argument
                    std::cerr << "inspect: unknown option\n";
                    return 1;
            }
        }
    
    // Get path from the running string
    const char* path = argv[optind];
       struct stat info;    // Get info
        // Check whether the path is valid, and if not, quit abruptly
       if (lstat(path, &info) != 0) {
           std::cerr << "inspect: cannot access " << path << ": "
                     << std::strerror(errno) << "\n";
           return 1;
       }
    
    // Show the path of the inspected file/directory/whatever (I'll call this an object)
    std::cout << "Inspecting: " << path << std::endl;
    
    // Size string (needs to be modified in case of a directory)
    std::string sizeString = "Size: ";
    
    // Check if the object is a regular file and print type
    if (S_ISREG(info.st_mode)) {
        std::cout << "Type: file\n";
        // Check if the object is a directory and print type
    }   else if (S_ISDIR(info.st_mode)) {
        std::cout << "Type: directory\n";
        // Annotate the size as one of the directory itself, not files inside (if not using nerd mode)
        sizeString = (nerd ? sizeString : "Size (directory entry): ");
        // Check if the object is a symlink and print type
    }   else if (S_ISLNK(info.st_mode)) {
            std::cout << "Type: symlink\n";
        // Print type as other if it doesn't match any of those provided before
    }   else {
            std::cout << "Type: other\n";
    }
    
    off_t file_size = info.st_size; // Get size of the object
    
    // Show the size in bytes no matter what if using nerd mode
    if (nerd) {
        std::cout << sizeString << file_size << "\n";
        // Show the sizes and convert them from bytes if not using nerd mode
    } else {
        if (file_size >= 1000000000000LL) {
            std::cout << sizeString << file_size / 1000000000000.0 << " TB (" << file_size << " bytes)\n";
        }   else if (file_size >= 1000000000LL) {
            std::cout << sizeString << file_size / 1000000000.0 << " GB (" << file_size << " bytes)\n";
        }   else if (file_size >= 1000000LL) {
            std::cout << sizeString << file_size / 1000000.0 << " MB (" << file_size << " bytes)\n";
        }   else if (file_size >= 1000LL) {
            std::cout << sizeString << file_size / 1000.0 << " KB (" << file_size << " bytes)\n";
        }   else {
            std::cout << sizeString << file_size << " bytes\n";
        }
    }
    
    // Get permissions (reading, writing, executing) from user, groups and others
    bool r_user = info.st_mode & S_IRUSR;
    bool r_group = info.st_mode & S_IRGRP;
    bool r_other = info.st_mode & S_IROTH;
    bool w_user = info.st_mode & S_IWUSR;
    bool w_group = info.st_mode & S_IWGRP;
    bool w_other = info.st_mode & S_IWOTH;
    bool e_user = info.st_mode & S_IXUSR;
    bool e_group = info.st_mode & S_IXGRP;
    bool e_other = info.st_mode & S_IXOTH;
    
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
    
    // Conclude whether the files can be read, written or executed if anyone passes the criteria
    bool readable = r_user || r_group || r_other;
    bool writable = w_user || w_group || w_other;
    bool executable = e_user || e_group || e_other;
    
    // Only show info if user is not running with nerd mode
    if (!nerd) {
        // Show if the object is readable and by who (hidden in nerd mode)
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
        
    // Get user ID
    uid_t UID = info.st_uid;
        
    // Get password to find the name of the owner based on the UID
    struct passwd* pw = getpwuid(UID);
    
    // Show the name if possible
    if (pw) {
        std::cout << "Owner: " << pw->pw_name
        << " (UID " << UID << ")\n";
    } else {
        std::cout << "Owner: <unknown>"
        << " (UID " << UID << ")\n";
    }
    
    // Get the group ID
    gid_t GID = info.st_gid;
    
    // Get the name of the group if possible
    struct group* gr = getgrgid(GID);

    // Show the group name if possible
    if (gr) {
        std::cout << "Group: " << gr->gr_name
                  << " (GID " << GID << ")\n";
    } else {
        std::cout << "Group: <unknown>"
                  << " (GID " << GID << ")\n";
    }
    
    // Get access time and show
    char* atime = std::ctime(&info.st_atime);
    std::cout << (nerd ? "atime: " : "Accessed: ") << atime;
    
    // Get modified time and show
    char* mtime = std::ctime(&info.st_mtime);
    std::cout << (nerd ? "mtime: " : "Modified: ") << mtime;
    
    // Get status change time and show
    char* ctime = std::ctime(&info.st_ctime);
    std::cout << (nerd ? "ctime: " : "Status Change: ") << ctime;
    
    return 0;
}

