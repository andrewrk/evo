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

### Main Op Codes

 * 0x00 - no-op
 * 0x01 - jump
   - JumpOperandLeft(ValueSource)
   - JumpOperandRight(ValueSource)
   - JumpComparison(Comparison)
   - JumpLabel(CodeLabel)
 * 0x02 - output one byte
   - OutputByte(ValueSource)
 * 0x03 - input byte
   - InputByte(ValueDest)
 * 0x04 - perform a calculation
   - CalcOperandLeft(ValueSource)
   - CalcOperandRight(ValueSource)
   - CalcOperation(Operation)
   - CalcDest(ValueDest)
 * 0x05 - call
   - CallLabel(CodeLabel)
   - CallCountdown(ValueSource) - how many instructions until the call happens
 * 0x06 - return
 * 0x07 - declare label
   - remaining 3 bytes are an integer representing the label value
 * 0x08 - copy
   - CopySrc(ValueSource)
   - CopyDest(ValueDest)

### Parameter Setting Op Codes

 * 0x09 - ValueSource
 * 0x0a - RegisterSource
 * 0x0b - OverflowBehavior
 * 0x0c - ValueDest
 * 0x0d - CodeLabel
 * 0x0e - Comparison
 * 0x0f - Operation
