# Evo

A programming language made for being the DNA of genetic algorithms.

Raw results from the decade-old project: https://s3.amazonaws.com/superjoe/temp/fix_quirks/out.html

I never did a blog writeup for this, so I'm rebooting it in Zig.

## Status

 * [x] Experiment with using Brainfuck code as the DNA of a genetic algorithm,
       as a control group.
 * [x] Invent Evo language minimal viable specification.
 * [ ] Implement an Evo interpreter.
 * [ ] Test to find out how effective it is.

## Building and Running

```
zig build
./zig-cache/bin/evo
```

## Evo Virtual Machine Specification

The Evo Virtual Machine is based on a finite tape of a fixed number of cells.
Each cell is a signed twos complement 32-bit integer.

An Evo program is an array of instructions. Each instruction occupies one tape
cell. An instruction interprets a tape cell as a little-endian array of 4
unsigned bytes. The first byte is the opcode. When an opcode is out of range,
it is wrapped around using modulus to produce a value in range.

When an Evo Virtual Machine begins, the program is copied into the tape at
offset 0. The program counter is set to 0.

The tape can be any size. When an Evo program attempts to access a tape cell
outside the range, the behavior is determined by a global variable, which can
be set at runtime.

Instead of parameters being data following the instructions, all instruction
parameters are global variables and each have their own instruction to set.

After executing an instruction, the program counter is incremented. If the
program counter increments beyond the tape, the program ends.

### Core Instructions

 * 0x00 - no-op
 * 0x01 - output one byte
   - OutputByte(ValueSource)
 * 0x02 - input byte
   - InputByte(ValueDest)
 * 0x03 - jump
   - JumpOperandLeft(ValueSource)
   - JumpOperandRight(ValueSource)
   - JumpComparison(Comparison)
   - JumpLabel(CodeLabel)
 * 0x04 - perform a calculation
   - CalcOperandLeft(ValueSource)
   - CalcOperandRight(ValueSource)
   - CalcOperation(Operation)
   - CalcDest(ValueDest)
 * 0x05 - call
   - CallLabel(CodeLabel)
   - CallCountdown(ValueSource) - how many instructions until the call happens
 * 0x06 - return
 * 0x07 - declare CodeLabel
   - remaining 3 bytes are an integer representing the label value
 * 0x08 - copy
   - CopySrc(ValueSource)
   - CopyDest(ValueDest)

### Parameter Setting Instructions

 * 0x09 - Comparison - default left operand is nonzero
 * 0x0d - CodeLabel
 * 0x0f - Operation

### Parameter Values

#### ValueSource

 * 0 - None. This operation is a noop.
 * 1 - Tape cell 0 interpreted as an Integer.
 * 2 - Tape cell 1 interpreted as an Integer.
 * 3 - Tape cell 2 interpreted as an Integer.
 * 4 - Tape cell 3 interpreted as an Integer.
 * 5 - The value 0.
 * 6 - The value 1.
 * 7 - The value 2.
 * 8 - The value 3.
 * 9 - The value -1.
 * A - The value -2.
 * B - The value -3.
 * C - The value -4.
 * D - Tape cell 0 interpreted as a CodeLabel.
 * E - Tape cell 1 interpreted as a CodeLabel.
 * F - Tape cell 2 interpreted as a CodeLabel.
 * G - Tape cell 3 interpreted as a CodeLabel.

#### ValueDest

 * 0x00 - None. This operation is a noop.
 * 0x01 - Tape cell 0.
 * 0x02 - Tape cell 1.
 * 0x03 - Tape cell 2.
 * 0x04 - Tape cell 3.
 * 0x05 - Tape cell 0 interpreted as a CodeLabel.
 * 0x06 - Tape cell 1 interpreted as a CodeLabel.
 * 0x07 - Tape cell 2 interpreted as a CodeLabel.
 * 0x08 - Tape cell 3 interpreted as a CodeLabel.
 * 0x09 - PC + 0
 * 0x0A - PC + 1
 * 0x0B - PC + 2
 * 0x0C - PC + 3
 * 0x0D - PC + 4
 * 0x0E - PC + (tape cell 0 interpreted as an Integer).
 * 0x0F - PC + (tape cell 1 interpreted as an Integer).
 * 0xA0 - PC + (tape cell 2 interpreted as an Integer).
 * 0xA1 - PC + (tape cell 3 interpreted as an Integer).

#### Comparison

 * 0 - Never
 * 1 - Always
 * 2 - Left operand is zero
 * 3 - Left operand greater than right operand
 * 4 - Left operand less than right operand
 * 5 - Left operand equal right operand
 * 6 - Left operand greator or equal to right operand
 * 7 - Left operand less or equal to right operand
 * 8 - Right operand is non-zero
 * 9 - Right operand is zero
 * A - Left operand is non-zero
