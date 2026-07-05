#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
using namespace std;
mutex console_mtx;
// =======================================
// ESTADOS DO SISTEMA / SYSTEMSTATES
// =======================================
struct systemStates {
  atomic<bool> pointersReady = false;
  atomic<bool> tablesReady= false;
  atomic<bool> BootloaderReady = false;
  atomic<bool> hardwareCheck = false;
};
systemStates state;
void ExitStates() {
  state.pointersReady.store(false);
  state.tablesReady.store(false);
  state.BootloaderReady.store(false);
  state.hardwareCheck.store(false);
};
// ===================
// RINGS
// ===================
enum class Rings {
  USER,
  KERNEL
};
Rings ringNormal = Rings::USER;
Rings ringKernel = Rings::KERNEL;
//  ===============
//  FILES
//  ===============
struct File {
  string local;
  string name;
  string type;
  string content;
};
File BootFile = {
  "C:sysPartitions//BOOTSYS//MiniOS32//sys",
  "Bootloader",
  "[BOOTsys]",
  "org 0x7C00\n"
  "start:\n"
  "mov si, msg\n"
  "print_loop:\n"
  "lodsb\n"
  "or al, al\n"
  "jz hang\n"
  "mov ah, 0x0E\n"
  "int 0x10\n"
  "jmp print_loop\n"
  "hang:\n"
  "jmp hang\n"
  "msg db 'Loading MiniOS Kernel...', 13, 10, 0"
};
File kernelFile = {
  "C:sys//MiniOS32//sys32Path",
  "kernel.elf",
  "KERNEL",
  "[Code.in-C++]"
};
// =============
// UEFI
// =============
void uefi() {
  lock_guard<mutex> lock(console_mtx);
  cout << "[UEFI]: Running POST (Power-On Self Test)..." << endl;
  state.hardwareCheck = true;
  cout << "[UEFI]: Reading boot file: " << BootFile.local << endl;
  cout << "[UEFI]: Executing: " << BootFile.name << endl;
  this_thread::sleep_for(chrono::milliseconds(700));
  cout << "[UEFI]: Bootloader -> INITIALIZED" << endl;
  state.BootloaderReady = true;
}
// =================
// BOOT / KERNEL
// =================
void Bootloader() {
  lock_guard<mutex> lock(console_mtx);
  if (!state.BootloaderReady) {
    cout << "[ERROR]: Bootloader not initialized." << endl;
    return;
  }
  cout << "[BOOTLOADER]: Loading system tables..." << endl;
  state.tablesReady = true;
  cout << "[BOOTLOADER]: Setting up memory pointers..." << endl;
  state.pointersReady = true;
  cout << "[BOOTLOADER]: Reading kernel file: " << kernelFile.local << endl;
  cout << "[BOOTLOADER]: Executing: " << kernelFile.name << endl;
  this_thread::sleep_for(chrono::milliseconds(700));
  cout << "====================================================" << endl;
  cout << "[SUCCESS] -> KERNEL: INITIALIZED <- [SUCCESS]" << endl;
  cout << "====================================================" << endl;
  state.BootloaderReady = false;
}
struct Microkernel {
  string name = "MiniOS";
  string version = "0.1.1";
  string type = "Microkernel";
  atomic<bool> initialized = false;
  void init() {
    initialized = true;
  }
  void shutdown() {
    initialized = false;
  }
};
Microkernel kernel;
// =================
// THREADS
// =================
atomic<bool> shellRunning = true;
atomic<bool> systemRunning = true;
atomic<int> uptimeSeconds = 0;
void uptime(atomic<bool>& systemRunning) {
  while (systemRunning) {
    this_thread::sleep_for(chrono::seconds(1));
    uptimeSeconds.fetch_add(1);
  }
}
// =================
// SHELL
// =================
void Shell(atomic<bool>& systemRunning) {
  string command;
  while (systemRunning) {
    {
      lock_guard<mutex> lock(console_mtx);
      cout << "[KERNEL]: Shell initialized." << endl;
      cout << "[KERNEL]: Type 'help' to list commands." << endl;
    }
    bool localShell = true;
    while (localShell) {
      {
        lock_guard<mutex> lock(console_mtx);
        cout << "hostuser@MiniOS> ";
        cout.flush();
      }
      if (!(cin >> command)) {
        systemRunning = false;
        break;
      }
      if (command == "help") {
        lock_guard<mutex> lock(console_mtx);
        cout << "============== COMMANDS ==============" << endl;
        cout << "help - show this command list." << endl;
        cout << "info - Display system informations." << endl;
        cout << "exit - Shutdown OS." << endl;
        cout << "kill - Shutdown shell." << endl;
        cout << "clock - registry informations of system time." << endl;
        cout << "ls - List system files." << endl;
        cout << "cat - Display file content." << endl;
        cout << "clear - clear the terminal screen." << endl;
        cout << "reboot - Restart the OS." << endl;
        cout << "ring - Check current ring privileges." << endl;
        cout << "hardware - Display initial boot device status." << endl;
        cout << "============== END ==============" << endl;
    } else if (command == "info") {
        lock_guard<mutex> lock(console_mtx);
        cout << "========== SYSTEM INFORMATIONS ==========" << endl;
        cout << "[NAME]: " << kernel.name << endl;
        cout << "[VERSION]: " << kernel.version << endl;
        cout << "[ARCHITECTURE]: " << kernel.type << endl;
        cout << "========== END OF COMMANDS ==========" << endl;
    } else if (command == "exit") {
        lock_guard<mutex> lock(console_mtx);
        cout << "[KERNEL]: Shutting OS..." << endl;
        this_thread::sleep_for(chrono::milliseconds(500));
        systemRunning = false;
        localShell = false;
        ExitStates();
        cout << "[KERNEL]: Goodbye!" << endl;
    } else if (command == "kill") {
        lock_guard<mutex> lock(console_mtx);
        cout << "[KERNEL]: Shell terminated by kernel." << endl;
        localShell = false;
    } else if (command == "clock") {
        int time = uptimeSeconds.load();
        lock_guard<mutex> lock(console_mtx);
        cout << "[CLOCK]: System time: "
        << time
        << " second(s)." << endl;
    } else if (command == "ls") {
        lock_guard<mutex> lock(console_mtx);
        cout << "--- FILE SYSTEM ---" << endl;
        cout << "[FILE]: " << BootFile.name << " \t[TYPE]: " << BootFile.type << endl;
        cout << "-------------------" << endl;
    } else if (command == "cat") {
        lock_guard<mutex> lock(console_mtx);
        cout << "--- READING FILE: " << BootFile.name << " ---" << endl;
        cout << BootFile.content << endl;
        cout << "---------------------------------" << endl;
        this_thread::sleep_for(chrono::milliseconds(500));
    } else if (command == "clear") {
        lock_guard<mutex> lock(console_mtx);
        for (int i = 0; i < 40; i++) {
          cout << "\n";
        }
    } else if (command == "reboot") {
        {
          lock_guard<mutex> lock(console_mtx);
          cout << "[KERNEL]: Rebooting OS..." << endl;
          this_thread::sleep_for(chrono::milliseconds(500));
        }
        uptimeSeconds.store(0);
        ExitStates();
        uefi();
        Bootloader();
        {
          lock_guard<mutex> lock(console_mtx);
          cout << "[KERNEL]: Reboot -> Successful!" << endl;
        }
    } else if (command == "ring") {
        lock_guard<mutex> lock(console_mtx);
        cout << "========== PRIVILEGE SECURITY ============================" << endl;
        if (ringNormal == Rings::USER) {
          cout << "[RING]: USER MODE (Ring 3)" << endl;
          cout << "[STATUS]: Real Environment - Standard Access." << endl;
        } else {
          cout << "[RING]: KERNEL MODE (Ring 0)" << endl;
          cout << "[STATUS]: Protected Environment - Unlimited Access." << endl;
        }
        cout << "==========================================================" << endl;
    } else if (command == "hardware") {
        lock_guard<mutex> lock(console_mtx);
        cout << "========== BOOT HARDWARE REPORT ==========" << endl;
        cout << "[POST]: " << (state.hardwareCheck ? "PASSED" : "FAILED") << endl;
        cout << "[SYS TABLES]: " << (state.tablesReady ? "ALLOCATED" : "NOT FOUND") << endl;
        cout << "[POINTERS]: " << (state.pointersReady ? "MAPPED" : "UNMAPPED") << endl;
        cout << "==========================================" << endl;
      } else {
        lock_guard<mutex> lock(console_mtx);
        cout << "[KERNEL]: Unknown command. Type 'help' for assistance." << endl;
      }
    }
    if (systemRunning) {
      lock_guard<mutex> lock(console_mtx);
      cout << "[KERNEL]: Restarting shell..." << endl;
      this_thread::sleep_for(chrono::milliseconds(500));
    }
  }
}
int main() {
  uefi();
  Bootloader();
  kernel.init();
  thread Clock(uptime, ref(systemRunning));
  thread shellThread(Shell, ref(systemRunning));
  shellThread.join();
  systemRunning = false;
  Clock.join();
  return 0;
}