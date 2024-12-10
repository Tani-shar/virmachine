/* PROJECTNAME.h */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* unix only */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

typedef unsigned char int8;
// typedef unsigned short int int16_t;
typedef unsigned int int32;
typedef unsigned long long int int64;

#define $1 (int8 *)
#define $2 (int16_t)
#define $4 (int32)
#define $8 (int64)
#define $c (char *)
#define $i (int)

#define MEMORY_MAX (1 << 16)
int16_t memory[MEMORY_MAX];

int16_t sign_extend(int16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

int16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

// memory mapped registers
enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
};
// registers definition
enum
{
    // first 8 registers are general purpose register
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,   /* program counter : used for next instruction*/
    R_COND, /* contdition fflag tell us information about previos calculation*/
    R_COUNT
};
// instruction set

// an instruction is a a command which tells the Cpu to do some fundamental task

// Opcode: operation codes
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

int16_t reg[R_COUNT];

// condition flags

enum
{
    FL_POS = 1 << 0, /* P for positive number  */
    FL_ZRO = 1 << 1, /* Z for zero*/
    FL_NEG = 1 << 2, /* N for negative number*/
};

// trapcodes

enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

void update_flags(int16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }

    if (reg[r] >> 15)
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}
void mem_write(int16_t address, int16_t val) {
    if (address < 0 || address >= MEMORY_MAX) {
        fprintf(stderr, "ERROR: Memory write out of bounds: 0x%04X\n", address);
        return;
    }
    memory[address] = val;
}

int16_t mem_read(int16_t address) {
    if (address < 0 || address >= MEMORY_MAX) {
        fprintf(stderr, "ERROR: Memory read out of bounds: 0x%04X\n", address);
        return 0;
    }
    
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}
int16_t swap16(int16_t x)
{
    return (x << 8) | (x >> 8);
}

size_t read_image_file(FILE *file)
{
    int16_t origin;
    if (fread(&origin, sizeof(origin), 1, file) != 1)
    {
        fprintf(stderr, "ERROR: Could not read origin\n");
        return 0;
    }

    origin = swap16(origin);

    int16_t max_read = MEMORY_MAX - origin;
    int16_t *p = memory + origin;
    size_t read = fread(p, sizeof(int16_t), max_read, file);

    for (size_t i = 0; i < read; i++)
    {
        p[i] = swap16(p[i]);
    }

    return read;
}

int read_image(const char *image_path)
{
    FILE *file = fopen(image_path, "rb");
    if (!file)
    {
        fprintf(stderr, "ERROR: Could not open file %s\n", image_path);
        return 0;
    }

    size_t bytes_read = read_image_file(file);
    fclose(file);

    if (bytes_read == 0)
    {
        fprintf(stderr, "ERROR: Failed to read image file %s\n", image_path);
        return 0;
    }

    return 1;
}

/*
    Here is the procedure we need to write:

    1.Load one instruction from memory at the address of the PC register.
    2. Increment the PC register.
    3. Look at the opcode to determine which type of instruction it should perform.
    4. Perform the instruction using the parameters in the instruction.
    5. Go back to step 1.

*/
int main(int argc, const char *argv[])
{

    // @{Load arguments }

    if (argc < 2)
    {
        /* show usage string */
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }
    // @{setup}
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    reg[R_COND] = FL_ZRO;

    // set the pc to starting position here we define 0x3000 is default

    enum
    {
        PC_START = 0x3000
    };

    reg[R_PC] = PC_START;

    int running = 1;

    while (running)
    {

        // Fetching part

        int16_t instr = mem_read(reg[R_PC]++);
        // here we are right shifting by 12 to extract opcodes
        int16_t operation = instr >> 12;
        if (operation < 0 || operation >= R_COUNT)
        {
            fprintf(stderr, "ERROR: Invalid opcode: 0x%04X at PC: 0x%04X\n",
                    operation, reg[R_PC] - 1);
            running = 0;
            break;
        }

        switch (operation)
        {
        case OP_ADD:
            // for addition
            {
                int16_t r0 = (instr >> 9) & 0x7;

                int16_t r1 = (instr >> 6) & 0x7;

                int16_t imm_flag = (instr >> 5) & 0x1;

                if (imm_flag)
                {
                    int16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                }
                else
                {
                    int16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] + reg[r2];
                }

                update_flags(r0);
            }

            break;

        case OP_AND:
            //@{AND}
            {
                int16_t r0 = (instr >> 9) & 0x7;

                int16_t r1 = (instr >> 6) & 0x7;

                int16_t imm_flag = (instr >> 5) & 0x1;

                if (imm_flag)
                {
                    int16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] & imm5;
                }
                else
                {
                    int16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] & reg[r2];
                }

                update_flags(r0);
                break;
            }
        case OP_NOT:
        {
            int16_t r0 = (instr >> 9) & 0x7;
            int16_t r1 = (instr >> 6) & 0x7;

            reg[r0] = ~reg[r1];
            update_flags(r0);
        }
        break;
        case OP_BR:
        {
            int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            int16_t cond_flag = (instr >> 9) & 0x7;
            if (cond_flag & reg[R_COND])
            {
                reg[R_PC] += pc_offset;
            }

            break;
        }
        case OP_JMP:
        {
            int16_t r1 = (instr >> 6) & 0x7;
            reg[R_PC] = reg[r1];
        }
        break;
        case OP_JSR:
        {
            int16_t long_flag = (instr >> 11) & 1;
            reg[R_R7] = reg[R_PC];
            if (long_flag)
            {
                int16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                reg[R_PC] += long_pc_offset; /* JSR */
            }
            else
            {
                int16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1]; /* JSRR */
            }
        }
        break;
        case OP_LD:
        {
            int16_t pcoffset = sign_extend(instr & 0x1FF, 9);
            int16_t r0 = (instr >> 9) & 0x7;
            reg[r0] = mem_read(reg[R_PC] + pcoffset);
            update_flags(r0);
        }
        break;
        case OP_LDI:
            //@{LDI}
            {
                // desitination register(DR)
                int16_t r0 = (instr >> 9) & 0x7;
                // PC offset
                int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
            }
            break;
        case OP_LDR:
        {
            int16_t r0 = (instr >> 9) & 0x7;
            int16_t r1 = (instr >> 6) & 0x7;
            int16_t offset = sign_extend(instr & 0x3F, 6);
            reg[r0] = mem_read(reg[r1] + offset);
            update_flags(r0);
        }
        break;
        case OP_LEA:
        {
            int16_t r0 = (instr >> 9) & 0x7;
            int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            reg[r0] = reg[R_PC] + pc_offset;
            update_flags(r0);
        }
        break;
        case OP_ST:
        {
            int16_t r0 = (instr >> 9) & 0x7;
            int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            mem_write(reg[R_PC] + pc_offset, reg[r0]);
        }
        break;
        case OP_STI:
        {
            int16_t r0 = (instr >> 9) & 0x7;
            int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
        }
        break;
        case OP_STR:
        {
            int16_t r0 = (instr >> 9) & 0x7;
            int16_t r1 = (instr >> 6) & 0x7;
            int16_t offset = sign_extend(instr & 0x3F, 6);
            mem_write(reg[r1] + offset, reg[r0]);
        }
        break;
        case OP_TRAP:
            reg[R_R7] = reg[R_PC];

            switch (instr & 0xFF)
            {
            case TRAP_GETC:
            {
                reg[R_R0] = (int16_t)getchar();
                update_flags(R_R0);
            }
            break;
            case TRAP_OUT:
            {
                putc((char)reg[R_R0], stdout);
                fflush(stdout);
            }
            break;
            case TRAP_PUTS:
                // similar to printf
                {
                    int16_t *c = memory + reg[R_R0];
                    while (*c)
                    {
                        putc((char)*c, stdout);
                        ++c;
                    }
                    fflush(stdout);
                }
                break;
            case TRAP_IN:
            {
                printf("enter the character");
                char c = getchar();
                putc(c, stdout);
                fflush(stdout);
                reg[R_R0] = (int16_t)c;
                update_flags(R_R0);
            }
            break;
            case TRAP_PUTSP:
            {
                int16_t *c = memory + reg[R_R0];
                size_t safety_counter = 0;
                while (*c && safety_counter < MEMORY_MAX)
                {
                    char char1 = (*c) & 0xFF;
                    putc(char1, stdout);
                    char char2 = (*c) >> 8;
                    if (char2)
                        putc(char2, stdout);
                    ++c;
                    ++safety_counter;
                }
                fflush(stdout);
            }
            break;
            case TRAP_HALT:
            {
                puts("HALT");
                fflush(stdout);
                running = 0;
            }
            break;
            }
            break;
        case OP_RES:
        case OP_RTI:
        default:
            abort();
            break;
        }
    }
    restore_input_buffering();
    return 0;
}
