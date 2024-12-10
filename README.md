# **LC-3 Emulator**

This pr\[oject is an emulator for the LC-3 architecture, implemented in C. The emulator simulates the functionality of the LC-3 machine, a hypothetical 16-bit microprocessor commonly used for educational purposes. It allows you to load machine code from a binary file and execute LC-3 instructions, enabling you to explore and experiment with assembly programming for the LC-3.

Made with the help of [Write your Own Virtual Machine](https://www.jmeiners.com/lc3-vm/#:hello-world-assembly)

## **Features**

- **Basic Instruction Set Support**: The emulator supports a range of instructions from the LC-3 ISA, including ADD, AND, BR (branch), LD (load), ST (store), JSR, TRAP, and more.
- **Memory and Registers**: The LC-3 machine model includes 8 general-purpose registers (R0 through R7), a condition flag register (COND), and a program counter (PC). Memory is emulated with a size of 65,536 (2^16) words.
- **Keyboard I/O**: The emulator supports basic keyboard input and output, including reading characters and printing strings via the TRAP instructions.
- **Memory Mapped I/O**: Implements memory-mapped I/O for keyboard and display operations.

## **Instructions Supported**

### **Arithmetic and Logic Operations**

- ADD: Add two registers or a register and an immediate value.
- AND: Bitwise AND between two registers or a register and an immediate value.
- NOT: Bitwise NOT of a register.

### **Load and Store Operations**

- LD: Load a value from memory into a register.
- ST: Store a value from a register into memory.
- LDR: Load a value from a memory address computed by adding an offset to a register.
- STR: Store a value from a register into a memory address computed by adding an offset to a register.
- LDI: Load a value indirectly (using the value in memory as the address).

### **Control Flow Operations**

- BR: Conditional branch based on the condition flags.
- JMP: Jump to a target address stored in a register.
- JSR: Jump to a subroutine (either via a register or an offset).
- LEA: Load the effective address into a register.

### **Trap Operations**

- TRAP: Call a system trap (e.g., read input, print output, halt program).

## **Setup Instructions**

**Clone the repository**:  
<br/>git clone <https://github.com/Tani-shar/lc3-emulator.git>

**Navigate to the project directory**:  
<br/>cd lc3-emulator

**Compile the project**:  
<br/>Ensure you have a C compiler (such as gcc) installed. Run the following command to compile the source code:  
<br/>gcc -o lc3_emulator lc3_emulator.c

**Run the Emulator**:  
<br/>You can run the emulator with the following command:  
<br/>./lc3_emulator &lt;path_to_image_file&gt;

1. Replace &lt;path_to_image_file&gt; with the path to your LC-3 binary image file (a .bin file containing machine code).  

## **How to Use the Emulator**

1. **Load Machine Code**: Use the read_image function to load the LC-3 machine code into memory from a binary file.
2. **Running the Program**: The emulator will start executing from memory location 0x3000, which is the default starting address.
3. **User Input and Output**: The program allows basic input/output via the keyboard and terminal. Use TRAP instructions for input/output operations (e.g., TRAP 0x20 for reading a character, TRAP 0x21 for printing a character).

### **Example Command**

To run a program with a binary image:

./lc3_emulator program_image.bin

### **Exit the Emulator**

To exit the emulator, press Ctrl+C (which triggers a SIGINT signal) to terminate the program gracefully.

## **Project Structure**

- **lc3_emulator.c**: The main C source file that contains the implementation of the emulator.
- **memory.h**: Header file defining the memory and registers.
- **Makefile**: Build file for compiling the emulator.
- **README.md**: This file, providing an overview of the project.

## **Contributions**

Contributions are welcome! Please fork the repository, create a new branch, and submit a pull request with your changes.
