# i n s p e c t.
Welcome to **inspect!** inspect is a **minimalist POSIX utility** written in **C++** with the goal of bringing **macOS' Quick Look** feature to **CLI environments** with some extra nerdy features.
## Features
By default, inspect shows the following info about provided path:
- **Type** (directory, regular file, symlink and others)
- **Size** (auto converted from bytes, if not running **nerd mode**, more on that later)
- **Permissions** (e.g. rwxr-xr-x)
- **Readable, Writable and Executable** (more human-readable version of permissions. Shows yes or no as well as who can perform such actions)
- **Owner and Group** (shows the names and IDs for the owner and group)
- **Accessed, Modified and Status Change** (in other words, atime, mtime and ctime dates)

**Example with the /etc symlink on macOS:**
```
Inspecting: /etc
Type: symlink
Size: 11 bytes
Permissions: rwxr-xr-x
Readable: yes (user, group, other)
Writeable: yes (user)
Executable: yes (user, group, other)
Owner: root (UID 0)
Group: wheel (GID 0)
Accessed: Sat Oct 25 06:22:19 2025
Modified: Sat Oct 25 06:22:19 2025
Status Change: Sat Oct 25 06:22:19 2025
```

**Nerd Mode** hides info like Readable, Writable and Executable, renames some into nerdier names as well as keeps sizes in bytes only.

**/etc example with Nerd Mode enabled:**
```
Inspecting: /etc
Type: symlink
Size: 11
Permissions: rwxr-xr-x
Owner: root (UID 0)
Group: wheel (GID 0)
atime: Sat Oct 25 06:22:19 2025
mtime: Sat Oct 25 06:22:19 2025
ctime: Sat Oct 25 06:22:19 2025
```

## Usage
To check info of a file, run ```./path/to/inspect /somefile``` in the terminal (replace with the actual path to inspect and file/directory/other you're trying to check the info for)

**Example with /etc:**

```./Users/flare/Downloads/inspect-macos-arm64 /etc```
### Multiple files (version 1.1 and above)
Since version 1.1, the user can include multiple paths and get info for all of them without the need of restarting the app.
**Example with the home directory and /etc:**

``./Users/flare/Downloads/inspect-macos-arm64 /Users/flare /etc``
### Arguments
If you want to check the app version, see all commands, or enable Nerd Mode, you can use arguments. 

**Example:**
```/path/to/inspect /somedir --nerd```

Full list of arguments:
| Argument | Short Argument | Description |
|----------|----------------|-------------|
| --help   | -h  | Shows help information |
| --ver  |  -v  | Shows utility version and some credits |
| --nerd |  -n | Enables Nerd Mode |

## Installation
### Compiling yourself
If you prefer compiling the code yourself, simply download the source code and compile ```main.cpp``` via your prefered compilator, like **clang** or **gcc**. Yep, it's the only thing you need to compile and run. The prefered usage platforms are **macOS, Linux and FreeBSD**. **I don't plan supporting anything else (that is UNIX-like, of course) anytime soon.**
### Downloading a pre-buit binary
If you're too lazy to compile the thing yourself, simply go to **Releases** of this GitHub page and grab the latest version of inspect for your prefered platform and follow the instructions.

**macOS Note: Install ```inspect-macos-x86_64.zip``` only on Intel Macs. Apple are planning to drop support for Rosetta 2, which is why for Apple Silicon users, i strongly recommend the native ```inspect-macos-arm64.zip```**.
# Thank you for trying inspect!
