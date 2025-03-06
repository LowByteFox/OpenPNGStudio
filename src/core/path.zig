const std = @import("std");
const builtin = @import("builtin");
const assert = std.debug.assert;

const PathNode = struct {
    name: []u8,
    is_file: bool,
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

    pub fn append_file(self: *Self, path: []const u8) !void {
        if (self.builder.last) |last| {
            assert(last.data.is_file == false);
        }

        var node = try self.allocator.create(L.Node);
        node.data.is_file = true;
        node.data.name = try self.allocator.dupe(u8, path);

        self.builder.append(node);
    }

    pub fn append_dir(self: *Self, path: []const u8) !void {
        if (self.builder.last) |last| {
            assert(last.data.is_file == false);
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
        const buffer = self.allocator.alloc(u8, self.buf_size());
        const offset = blk: {
            comptime switch (builtin.target.os.tag) {
                .windows => {
                    @memcpy(buffer[0..2], "C:");
                    break :blk 2;
                },
                else => {
                    break :blk 0;
                }
            };
        };

        std.debug.print("{}\n", .{offset});

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
