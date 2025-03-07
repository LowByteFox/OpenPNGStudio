// This file is part of OpenPNGStudio. 
// Copyright (C) 2024-2025 LowByteFox
//   
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//   
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//   
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

const std = @import("std");
const builtin = @import("builtin");
const builder = @import("string_builder.zig");

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
        var s_builder = builder.StringBuilder.init(self.allocator);

        switch (builtin.target.os.tag) {
            .windows => try s_builder.appendString("C:"),
            else => try s_builder.appendChar('/'),
        }

        var iter = self.builder.first;

        while (iter) |i| : (iter = i.next) {
            try s_builder.appendString(i.data.name);

            if (!i.data.is_file) {
                try s_builder.appendChar(std.fs.path.sep);
            }
        }

        try s_builder.shrinkToFit();

        return s_builder.getString();
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
