const std = @import("std");
const mem = std.mem;

pub fn main() anyerror!void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    const allocator = &arena.allocator;
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    var opt_goal: ?[]const u8 = null;
    var seed_str: ?[]const u8 = null;
    var positional: ?[]const u8 = null;

    var arg_i: usize = 1;
    while (arg_i < args.len) : (arg_i += 1) {
        const arg = args[arg_i];
        if (mem.eql(u8, arg, "-g") or mem.eql(u8, arg, "--goal")) {
            arg_i += 1;
            opt_goal = args[arg_i];
        } else if (mem.eql(u8, arg, "-s") or mem.eql(u8, arg, "--seed")) {
            arg_i += 1;
            seed_str = args[arg_i];
        } else if (positional) |p| {
            return errorUsage("unexpected parameter: {}\n", .{p});
        } else {
            positional = arg;
        }
    }

    const seed = if (seed_str) |s| blk: {
        break :blk try std.fmt.parseInt(usize, s, 10);
    } else blk: {
        var seed_bytes: [@sizeOf(usize)]u8 = undefined;
        try std.crypto.randomBytes(&seed_bytes);
        break :blk std.mem.readIntNative(usize, &seed_bytes);
    };

    const goal = opt_goal orelse {
        // just run the program that was passed in
        const program_filename = positional orelse return errorUsage("expected parameter\n", .{});

        // get program bytes
        const max_bytes = 32 * 1024;
        const program_bytes = try std.fs.cwd().readFileAlloc(allocator, program_filename, max_bytes);
        defer allocator.free(program_bytes);

        var bf = try BrainFuckInterpreter(readStdIn, writeStdOut).init(
            allocator,
            program_bytes,
            std.math.maxInt(usize),
        );
        try bf.start();

        return;
    };

    @panic("TODO: time to play with genetic algorithms!");
}

fn readStdIn() u8 {
    var buf: [1]u8 = undefined;
    std.io.getStdIn().readAll(&buf) catch unreachable;
    return buf[0];
}

fn writeStdOut(byte: u8) void {
    var buf: [1]u8 = .{byte};
    std.io.getStdOut().writeAll(&buf) catch unreachable;
}

fn errorUsage(comptime fmt: []const u8, args: var) anyerror!void {
    std.debug.warn(fmt, args);
    return error.InvalidCommandLineArgument;
}

fn BrainFuckInterpreter(comptime readByte: fn () u8, comptime writeByte: fn (byte: u8) void) type {
    return struct {
        tape: [tape_size]u8,
        tape_head: usize,
        pc: usize,
        cycle_count: usize,
        program: []const u8,
        max_cycles: usize,
        matching_bracket: std.AutoHashMap(usize, usize),

        const tape_size = 32 * 1024;

        const Self = @This();

        pub fn init(
            allocator: *mem.Allocator,
            program_bytes: []const u8,
            max_cycles: usize,
        ) !Self {
            var self: Self = .{
                .tape = [1]u8{0} ** tape_size,
                .tape_head = 0,
                .matching_bracket = std.AutoHashMap(usize, usize).init(allocator),
                .pc = 0,
                .cycle_count = 0,
                .program = program_bytes,
                .max_cycles = max_cycles,
            };
            // parse for matching brackets
            var stack = std.ArrayList(usize).init(allocator);
            var i: usize = 0;
            while (i < program_bytes.len) : (i += 1) {
                switch (program_bytes[i]) {
                    '[' => try stack.append(i),
                    ']' => if (stack.len != 0) {
                        const begin_index = stack.pop();
                        // -1 to compensate for the PC incrementing
                        try self.matching_bracket.putNoClobber(i, begin_index - 1);
                        try self.matching_bracket.putNoClobber(begin_index, i);
                    } else {
                        // just send control forward; ignore this close bracket
                        try self.matching_bracket.putNoClobber(i, i + 1);
                    },
                    else => continue,
                }
            }
            return self;
        }

        pub fn start(self: *Self) !void {
            while (self.pc < self.program.len) {
                if (self.cycle_count >= self.max_cycles) {
                    return error.CycleCountExceeded;
                }
                switch (self.program[self.pc]) {
                    '<' => if (self.tape_head != 0) {
                        self.tape_head -= 1;
                    },
                    '>' => if (self.tape_head < self.tape.len) {
                        self.tape_head += 1;
                    },
                    '+' => self.tape[self.tape_head] += 1,
                    '-' => self.tape[self.tape_head] -= 1,
                    '.' => writeByte(self.tape[self.tape_head]),
                    ',' => self.tape[self.tape_head] = readByte(),
                    '[' => if (self.tape[self.tape_head] == 0) {
                        self.pc = self.matching_bracket.getValue(self.pc).?;
                    },
                    ']' => self.pc = self.matching_bracket.getValue(self.pc).?,
                    else => {},
                }
                self.pc += 1;
                self.cycle_count += 1;
            }
        }
    };
}
