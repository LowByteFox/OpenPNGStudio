const std = @import("std");
const builtin = @import("builtin");
const assert = std.debug.assert;

const PathNode = struct {
    name: []u8,
    is_file: bool,
};

pub const PathError = error {
    ParentNotADir,
};

pub const Path = struct {
    const L = std.DoublyLinkedList(PathNode);
    const Self = @This();
    allocator: std.mem.Allocator,
    builder: L,

    pub fn init(allocator: std.mem.Allocator) Self {
        return .{
            .allocator = allocator,
            .builder = L{},
        };
    }

    pub fn deinit(self: *Self) void {
        var first = self.builder.popFirst();
        while (first) |i| : (first = self.builder.popFirst()) {
            self.allocator.free(i.data.name);
            self.allocator.destroy(i);
        }
    }

    pub fn append_file(self: *Self, path: []const u8) !void {
        if (self.builder.last) |last| {
            if (last.data.is_file != false) {
                return PathError.ParentNotADir;
            }
        }

        var node = try self.allocator.create(L.Node);
        node.data.is_file = true;
        node.data.name = try self.allocator.dupe(u8, path);

        self.builder.append(node);
    }

    pub fn append_dir(self: *Self, path: []const u8) !void {
        if (self.builder.last) |last| {
            if (last.data.is_file != false) {
                return PathError.ParentNotADir;
            }
        }

        var node = try self.allocator.create(L.Node);
        node.data.is_file = false;
        node.data.name = try self.allocator.dupe(u8, path);

        self.builder.append(node);
    }

    pub fn pop(self: *Self) void {
        if (self.builder.last) |last| {
            self.allocator.destroy(last);
        }
    }

    pub fn stringify(self: *Self) ![]u8 {
        const buffer = try self.allocator.alloc(u8, self.buf_size());
        const offset = blk: {
            switch (builtin.target.os.tag) {
                .windows => {
                    @memcpy(buffer[0..2], "C:");
                    break :blk 2;
                },
                else => {
                    buffer[0] = '/';
                    break :blk 1;
                }
            }
        };

        var index: usize = offset;

        var iter = self.builder.first;

        while (iter) |i| : (iter = i.next) {
            @memcpy(buffer[index..index + i.data.name.len], i.data.name);
            index += i.data.name.len;

            if (!i.data.is_file) {
                buffer[index] = '/';
                index += 1;
            }
        }

        return buffer;
    }

    fn buf_size(self: *const Self) usize {
        var size: usize = comptime switch (builtin.target.os.tag) {
            .windows => 3,
            .linux, .macos, .freebsd, .openbsd, .netbsd, .dragonfly, .solaris => 1,
            else => @compileError("Unsupported platform " ++ @tagName(builtin.target.os.tag)),
        };

        var iter = self.builder.first;

        while (iter) |i| : (iter = i.next) {
            size += i.data.name.len;

            if (!i.data.is_file) {
                size += 1;
            }
        }

        return size;
    }
};

test "basic path" {
    var path = Path.init(std.testing.allocator);
    defer path.deinit();

    try path.append_dir("usr");
    try path.append_dir("bin");

    try std.testing.expectEqual(2, path.builder.len);
}

test "attempt append file" {
    var path = Path.init(std.testing.allocator);
    defer path.deinit();

    try path.append_dir("rose");
    try path.append_file("is"); 

    try std.testing.expectError(PathError.ParentNotADir, path.append_file("cute"));
}

test "path length" {
    var path = Path.init(std.testing.allocator);
    defer path.deinit();

    try path.append_dir("usr");
    try path.append_dir("bin");

    try std.testing.expectEqual(9 + comptime switch (builtin.target.os.tag) {
        .windows => 2,
        else => 0,
    }, path.buf_size());
}

test "path string" {
    var path = Path.init(std.testing.allocator);
    defer path.deinit();

    try path.append_dir("home");
    try path.append_dir("rose");
    try path.append_file("cute :3");

    const buf = try path.stringify();
    defer std.testing.allocator.free(buf);

    try std.testing.expectEqualSlices(u8, "/home/rose/cute :3", buf);
}
