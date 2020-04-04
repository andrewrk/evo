const std = @import("std");
const mem = std.mem;

const Prg = struct {
    buffer: [max_bytes]u8,
    len: usize,

    const max_bytes = 32 * 1024;

    fn span(self: *Prg) []u8 {
        return self.buffer[0..self.len];
    }

    fn append(self: *Prg, byte: u8) void {
        self.buffer[self.len] = byte;
        self.len += 1;
    }
};

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
        const program_bytes = try std.fs.cwd().readFileAlloc(allocator, program_filename, Prg.max_bytes);
        defer allocator.free(program_bytes);

        var bf: BrainFuckInterpreter(void, readStdIn, writeStdOut) = undefined;
        bf.reset(program_bytes, std.math.maxInt(usize));
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
    const program_set_a = try allocator.alloc(Prg, generation_size);
    const program_set_b = try allocator.alloc(Prg, generation_size);
    {
        var i: usize = 0;
        while (i < generation_size) : (i += 1) {
            program_set_a[i].len = init_program_size;
            generateRandomProgram(program_set_a[i].span(), rng);
            std.debug.warn("generated random bf program {}:\n{}\n", .{ i, program_set_a[i].span() });
        }
    }
    var program_set = &program_set_a;
    var other_program_set = &program_set_b;

    var best_score: f32 = 0.0;
    var best_src: Prg = undefined;
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

    const Score = struct {
        scalar: f32,
        src: []const u8,

        const Score = @This();

        fn greaterThanOrEql(a: Score, b: Score) bool {
            return a.scalar >= b.scalar;
        }
    };
    var program_scores = std.ArrayList(Score).init(allocator);

    var generation_count: usize = 0;
    while (generation_count < generation_limit) {
        generation_count += 1;
        //std.debug.warn("Generation {}\n", .{generation_count});
        var generation_score: f32 = 0;
        var generation_max_score: f32 = 0;
        var generation_program_size: usize = 0;

        program_scores.shrink(0);

        // evaluate the set of programs and give a score to each
        for (program_set.*) |*this_prg, i| {
            generation_program_size += this_prg.len;
            //std.debug.warn("evaluating program {}\n", .{i});
            interp.reset(this_prg.span(), timeout_cycle_count);
            bf_out.shrink(0);
            interp.start(&bf_out);

            //std.debug.warn("cycle count: {}\n", .{interp.cycle_count});

            const output = bf_out.toSliceConst();
            const cycle_usage = @intToFloat(f32, interp.cycle_count) / @intToFloat(f32, timeout_cycle_count);

            //std.debug.warn("output:\n{}\n", .{output});

            const score = assignOutputScore(goal_out_bytes, output, cycle_usage, this_prg.len);
            generation_score += score;
            if (score > generation_max_score)
                generation_max_score = score;
            if (score > best_score) {
                best_score = score;

                best_output.shrink(0);
                try best_output.appendSlice(output);

                best_src = this_prg.*;
                best_cycle_count = interp.cycle_count;

                std.debug.warn("new best. cycle_count={} score={}\noutput:\n{}\nsrc:\n{}\n", .{
                    best_cycle_count,
                    score,
                    best_output.span(),
                    best_src.span(),
                });
            }

            //std.debug.warn("output score: {}\n", .{score});
            try program_scores.append(.{ .scalar = score, .src = this_prg.span() });
        }

        // take a subset of the top scoring programs and breed them to get a new
        // set of programs to evaluate
        std.sort.sort(Score, program_scores.span(), Score.greaterThanOrEql);
        //std.debug.warn("program scores:\n", .{});
        //for (program_scores.span()) |score, i| {
        //    std.debug.warn("{} = {}\n", .{i, score.scalar});
        //}

        var survivor_index: usize = 0;
        var next_generation_index: usize = 0;
        while (survivor_index < surviver_count) : (survivor_index += 1) {
            const src = program_scores.span()[survivor_index].src;
            makeBabies(rng, src, other_program_set.*, &next_generation_index, babies_per_program, mutation_chance);
        }

        // take some random programs and breed them to get a new set of programs to evaluate
        var i: usize = 0;
        while (i < random_surviver_count) : (i += 1) {
            const rand_src = program_set.*[rng.uintLessThanBiased(usize, program_set.len)].span();
            makeBabies(rng, rand_src, other_program_set.*, &next_generation_index, babies_per_program, mutation_chance);
        }

        std.debug.warn("Best output so far: '{}'\n", .{best_output.span()});
        std.debug.warn("(S) Generation={} avg_score={} max_score={} avg_prg_size={}\n", .{
            generation_count,
            generation_score / generation_size,
            generation_max_score,
            generation_program_size / generation_size,
        });

        {
            const tmp = program_set;
            program_set = other_program_set;
            other_program_set = tmp;
        }
    }
}

fn makeBabies(
    rng: *std.rand.Random,
    src: []const u8,
    program_set: []Prg,
    next_generation_index: *usize,
    babies_per_program: usize,
    mutation_chance: f32,
) void {
    var baby: usize = 0;
    while (baby < babies_per_program) : (baby += 1) {
        // how is babby formed?
        const baby_prg = &program_set[next_generation_index.*];
        baby_prg.len = 0;
        for (src) |byte, byte_index| {
            // mutate?
            const rand_float = rng.float(f32);
            if (rand_float < mutation_chance) {
                // mutate!
                switch (rng.uintLessThanBiased(u8, 3)) {
                    0 => {
                        // change a byte randomly
                        baby_prg.append(randBfByte(rng));
                    },
                    1 => {
                        // insert a random byte
                        baby_prg.append(randBfByte(rng));
                        baby_prg.append(byte);
                    },
                    2 => {
                        // don't copy this byte
                    },
                    else => unreachable,
                }
            } else {
                // copy gene (byte) to program. no errors
                baby_prg.append(byte);
            }
        }

        //std.debug.warn("Generated new code for program {}:\n{}\n", .{ next_generation_index.*, baby_prg.span() });

        next_generation_index.* += 1;
    }
}

fn randBfByte(rng: *std.rand.Random) u8 {
    const possible_bytes = [_]u8{ ' ', '>', '<', '+', '-', '.', ',', '[', ']' };
    return possible_bytes[rng.uintLessThanBiased(u8, possible_bytes.len)];
}

fn generateRandomProgram(program: []u8, rng: *std.rand.Random) void {
    for (program) |*byte| {
        byte.* = randBfByte(rng);
    }
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

fn EvoVirtualMachine(
    comptime Context: type,
    comptime readByte: fn (context: Context) u8,
    comptime writeByte: fn (context: Context, byte: u8) void,
) type {
    return struct {
        tape: [tape_size]i32,
        pc: usize,
        cycle_count: usize,
        max_cycles: usize,

        const tape_size = 32 * 1024;
        const Self = @This();

        const OpCode = enum {
            NoOp = 0,
            Out = 1,
        };

        pub fn reset(self: *Self, program_code: []const i32, max_cycles: usize) void {
            self.* = .{
                .tape = undefined,
                .pc = 0,
                .cycle_count = 0,
                .max_cycles = max_cycles,
            };
            mem.copy(i32, &self.tape, program_code);
            mem.set(i32, self.tape[program_code.len..], 0);
        }

        pub fn start(self: *Self, context: Context) void {
            while (self.pc < self.tape.len and self.cycle_count < self.max_cycles) {
                const unsigned = @bitCast(u32, self.tape[self.pc]);
                const op_code_oob = @truncate(u8, unsigned);
                const op_code = op_code_oob % @typeInfo(OpCode).Enum.fields.len;
                switch (@intToEnum(OpCode, op_code)) {
                    .NoOp => {},
                    .Out => {
                        // TODO
                    },
                }

                self.pc += 1;
                self.cycle_count += 1;
            }
        }
    };
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
        matching_bracket: [Prg.max_bytes]usize,

        const tape_size = 32 * 1024;

        const Self = @This();

        pub fn reset(self: *Self, program_bytes: []const u8, max_cycles: usize) void {
            self.* = .{
                .tape = [1]u8{0} ** tape_size,
                .tape_head = 0,
                .matching_bracket = undefined,
                .pc = 0,
                .cycle_count = 0,
                .program = program_bytes,
                .max_cycles = max_cycles,
            };

            // parse for matching brackets
            for (program_bytes) |_, i| {
                // default: no control flow modification
                self.matching_bracket[i] = i;
            }
            var stack: [Prg.max_bytes]usize = undefined;
            var stack_index: usize = 0;
            for (program_bytes) |byte, i| {
                switch (byte) {
                    '[' => {
                        stack[stack_index] = i;
                        stack_index += 1;
                    },
                    ']' => if (stack_index != 0) {
                        stack_index -= 1;
                        const begin_index = stack[stack_index];
                        self.matching_bracket[i] = begin_index;
                        self.matching_bracket[begin_index] = i;
                    },
                    else => continue,
                }
            }
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
                        self.pc = self.matching_bracket[self.pc];
                    },
                    ']' => if (self.matching_bracket[self.pc] != self.pc) {
                        self.pc = self.matching_bracket[self.pc];
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
    {
        const src = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
        try testBF(src, "", "Hello World!\n");
    }
    {
        // bad bracket
        const src = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.++]]+.----]--.--------.>+.>.]";
        try testBF(src, "", "Hello World!\n");
    }
    {
        // extra begin bracket
        const src = "++[++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
        try testBF(src, "", "Hello World!\n");
    }
}

fn testBF(src: []const u8, input: []const u8, expected_output: []const u8) !void {
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
        .input = input,
        .next = 0,
        .output = std.ArrayList(u8).init(std.testing.allocator),
    };
    defer context.output.deinit();

    var bf: BrainFuckInterpreter(*Context, Context.testStdIn, Context.testStdOut) = undefined;
    bf.reset(src, std.math.maxInt(usize));

    bf.start(&context);

    std.testing.expect(mem.eql(u8, context.output.span(), expected_output));
}

fn assignOutputScore(goal: []const u8, actual: []const u8, cycle_usage: f32, program_size: usize) f32 {
    // if actual is blank, score is 0
    // if goal == actual score is perfect 1
    var score: f32 = 0.0;
    var char_count = @intToFloat(f32, goal.len);
    var i: usize = 0;
    while (i < goal.len and i < actual.len) : (i += 1) {
        const goal_float_char = @intToFloat(f32, @bitCast(i8, goal[i]));
        const actual_float_char = @intToFloat(f32, @bitCast(i8, actual[i]));
        const ascii_diff = std.math.fabs(goal_float_char - actual_float_char);
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

test "assign output score" {
    const goal = "Hello, World!\n";
    const actual = "\x0034!2OOabcdggiii";
    const cycle_usage = 0.167500004;
    const program_size = 376;
    const result = assignOutputScore(goal, actual, cycle_usage, program_size);
}
