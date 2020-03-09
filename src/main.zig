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
        break :blk try std.fmt.parseInt(u64, s, 10);
    } else blk: {
        var seed_bytes: [@sizeOf(u64)]u8 = undefined;
        try std.crypto.randomBytes(&seed_bytes);
        break :blk std.mem.readIntNative(u64, &seed_bytes);
    };

    var default_prng = std.rand.DefaultPrng.init(seed);
    const rng = &default_prng.random;

    const goal = opt_goal orelse {
        // just run the program that was passed in
        const program_filename = positional orelse return errorUsage("expected parameter\n", .{});

        // get program bytes
        const max_bytes = 32 * 1024;
        const program_bytes = try std.fs.cwd().readFileAlloc(allocator, program_filename, max_bytes);
        defer allocator.free(program_bytes);

        var bf = try BrainFuckInterpreter(void, readStdIn, writeStdOut).init(
            allocator,
            program_bytes,
            std.math.maxInt(usize),
        );
        bf.start({});

        return;
    };

    // goal is the goal stdout that we want to achieve.
    const max_stdout = 100 * 1024;
    const goal_out_bytes = try std.fs.cwd().readFileAlloc(allocator, goal, max_stdout);

    const babies_per_program = 10;
    const surviver_count = 5;
    const random_surviver_count = 5;
    const init_program_size = 999;
    const timeout_cycle_count = 800;
    const mutation_chance = 0.001;
    const generation_limit = std.math.maxInt(usize);

    const generation_size = babies_per_program * (surviver_count + random_surviver_count);

    // generate a set of random starting programs
    var program_set = std.ArrayList([]u8).init(allocator);
    {
        var i: usize = 0;
        while (i < generation_size) : (i += 1) {
            try program_set.append(try generateRandomProgram(allocator, rng, init_program_size));
            std.debug.warn("generated random bf program {}:\n{}\n", .{ i, program_set.span()[program_set.len - 1] });
        }
    }

    var best_score: f32 = 0.0;
    var best_src = std.ArrayList(u8).init(allocator);
    var best_output = std.ArrayList(u8).init(allocator);
    var best_cycle_count: usize = undefined;

    var bf_out = std.ArrayList(u8).init(allocator);

    const S = struct {
        fn inFn(list: *std.ArrayList(u8)) u8 {
            return 0;
        }
        fn outFn(list: *std.ArrayList(u8), byte: u8) void {
            list.append(byte) catch unreachable;
        }
    };

    const BF = BrainFuckInterpreter(*std.ArrayList(u8), S.inFn, S.outFn);
    var interp: BF = undefined;

    var generation_count: usize = 0;
    while (generation_count < generation_limit) {
        generation_count += 1;
        std.debug.warn("Generation {}\n", .{generation_count});
        var generation_score: f32 = 0;
        var generation_max_score: f32 = 0;
        var generation_program_size: usize = 0;

        // evaluate the set of programs and give a score to each
        const Score = struct {
            scalar: f32,
            src: []const u8,
        };
        var program_scores = std.ArrayList(Score).init(allocator);
        var i: usize = 0;
        while (i < program_set.len) : (i += 1) {
            generation_program_size += program_set.span()[i].len;
            std.debug.warn("evaluating program {}\n", .{i});
            interp = try BF.init(allocator, program_set.span()[i], timeout_cycle_count);
            defer interp.deinit();
            bf_out.shrink(0);
            interp.start(&bf_out);

            std.debug.warn("cycle count: {}\n", .{interp.cycle_count});

            const output = bf_out.toSliceConst();
            const cycle_usage = @intToFloat(f32, interp.cycle_count) / @intToFloat(f32, timeout_cycle_count);

            std.debug.warn("output:\n{}\n", .{output});

            const score = assignOutputScore(goal_out_bytes, output, cycle_usage, program_set.span()[i].len);
            generation_score += score;
            if (score > generation_max_score)
                generation_max_score = score;
            if (score > best_score) {
                best_score = score;

                best_output.shrink(0);
                try best_output.appendSlice(output);

                best_src.shrink(0);
                try best_src.appendSlice(program_set.span()[i]);

                best_cycle_count = interp.cycle_count;
            }

            std.debug.warn("output score: {}\n", .{score});
            try program_scores.append(.{ .scalar = score, .src = program_set.span()[i] });
        }

        // take a subset of the top scoring programs and breed them to get a new
        // set of programs to evaluate
        @panic("TODO take a subset of the top scoring programs and breed them to get a new...");
    }
}

fn generateRandomProgram(allocator: *mem.Allocator, rng: *std.rand.Random, program_size: usize) ![]u8 {
    const program = try allocator.alloc(u8, program_size);
    const possible_bytes = [_]u8{ ' ', '>', '<', '+', '-', '.', ',', '[', ']' };
    for (program) |*byte| {
        byte.* = possible_bytes[rng.uintLessThanBiased(u8, possible_bytes.len)];
    }
    return program;
}

fn readStdIn(context: void) u8 {
    var buf: [1]u8 = undefined;
    std.io.getStdIn().readAll(&buf) catch unreachable;
    return buf[0];
}

fn writeStdOut(context: void, byte: u8) void {
    var buf: [1]u8 = .{byte};
    std.io.getStdOut().writeAll(&buf) catch unreachable;
}

fn errorUsage(comptime fmt: []const u8, args: var) anyerror!void {
    std.debug.warn(fmt, args);
    return error.InvalidCommandLineArgument;
}

fn BrainFuckInterpreter(
    comptime Context: type,
    comptime readByte: fn (context: Context) u8,
    comptime writeByte: fn (context: Context, byte: u8) void,
) type {
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
            defer stack.deinit();
            for (program_bytes) |byte, i| {
                switch (byte) {
                    '[' => try stack.append(i),
                    ']' => if (stack.popOrNull()) |begin_index| {
                        try self.matching_bracket.putNoClobber(i, begin_index);
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

        pub fn deinit(self: *Self) void {
            self.matching_bracket.deinit();
            self.* = undefined;
        }

        pub fn start(self: *Self, context: Context) void {
            while (self.pc < self.program.len) {
                if (self.cycle_count >= self.max_cycles) return;
                switch (self.program[self.pc]) {
                    '<' => if (self.tape_head != 0) {
                        self.tape_head -= 1;
                    },
                    '>' => if (self.tape_head < self.tape.len) {
                        self.tape_head += 1;
                    },
                    '+' => self.tape[self.tape_head] +%= 1,
                    '-' => self.tape[self.tape_head] -%= 1,
                    '.' => writeByte(context, self.tape[self.tape_head]),
                    ',' => self.tape[self.tape_head] = readByte(context),
                    '[' => if (self.tape[self.tape_head] == 0) {
                        if (self.matching_bracket.getValue(self.pc)) |i| {
                            self.pc = i;
                        }
                    },
                    ']' => {
                        self.pc = self.matching_bracket.getValue(self.pc).?;
                        self.cycle_count += 1;
                        continue;
                    },
                    else => {},
                }
                self.pc += 1;
                self.cycle_count += 1;
            }
        }
    };
}

test "interpreter" {
    const src = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";

    const Context = struct {
        input: []const u8,
        output: std.ArrayList(u8),
        next: usize,

        fn testStdIn(self: *@This()) u8 {
            const index = self.next;
            self.next += 1;
            if (index < self.input.len) {
                return self.input[index];
            } else {
                return 0;
            }
        }

        fn testStdOut(self: *@This(), byte: u8) void {
            self.output.append(byte) catch unreachable;
        }
    };
    var context: Context = .{
        .input = "",
        .next = 0,
        .output = std.ArrayList(u8).init(std.testing.allocator),
    };
    defer context.output.deinit();

    var bf = try BrainFuckInterpreter(*Context, Context.testStdIn, Context.testStdOut).init(
        std.testing.allocator,
        src,
        std.math.maxInt(usize),
    );
    defer bf.deinit();

    bf.start(&context);

    std.testing.expect(mem.eql(u8, context.output.span(), "Hello World!\n"));
}

fn assignOutputScore(goal: []const u8, actual: []const u8, cycle_usage: f32, program_size: usize) f32 {
    // if actual is blank, score is 0
    // if goal == actual score is perfect 1
    var score: f32 = 0.0;
    var char_count = @intToFloat(f32, goal.len);
    var i: usize = 0;
    while (i < goal.len and i < actual.len) : (i += 1) {
        const ascii_diff = @intToFloat(f32, goal[i]) - @intToFloat(f32, actual[i]);
        const delta = (1.0 - ascii_diff / 255.0) * (1.0 / char_count);
        //stdout_ << "character " << i << " off by " << ascii_diff << ", adding " << delta << "to score";
        score += delta;
    }

    // penalty for actual being too long
    const how_much_longer = @intToFloat(f32, actual.len) - @intToFloat(f32, goal.len);
    if (how_much_longer > 0) {
        const max_penalty = 0.10;
        const penalty = (how_much_longer / 100.0) * max_penalty;
        score -= penalty;
    }

    // penalty for using more cycles
    const max_cycle_penalty = 0.05;
    const cycle_penalty = cycle_usage * cycle_usage * max_cycle_penalty;
    score -= cycle_penalty;

    // slight penalty for larger code
    const max_size_penalty = 0.025;
    const size_penalty = @intToFloat(f32, program_size) / 1000.0 * max_size_penalty;
    score -= size_penalty;

    if (score < 0.0) {
        score = 0.0;
    } else if (score > 1.0) {
        score = 1.0;
    }
    return score;
}
