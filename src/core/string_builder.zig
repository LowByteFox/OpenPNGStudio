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

const INIT_SIZE = 32;

pub const StringBuilderError = error {
    BufferNotInitialized,
};

pub const StringBuilder = struct {
    const Self = @This();

    allocator: std.mem.Allocator,
    buffer: ?[]u8,
    index: usize,

    pub fn init(allocator: std.mem.Allocator) Self {
        return .{
            .allocator = allocator,
            .buffer = null,
            .index = 0,
        };
    }

    pub fn deinit(self: *Self) void {
        if (self.buffer) |buffer| {
            self.allocator.free(buffer);
        }

        self.index = 0;
    }

    pub fn shrinkToFit(self: *Self) !void {
        if (self.buffer) |buffer| {
            self.buffer = try self.allocator.realloc(buffer, self.index);
        }
    }

    pub fn getString(self: *Self) ![]u8 {
        if (self.buffer) |buffer| {
            self.index = 0;
            defer self.buffer = null;

            return buffer;
        } else {
            return StringBuilderError.BufferNotInitialized;
        }
    }

    pub fn appendChar(self: *Self, c: u8) !void {
        if (self.buffer) |buffer| {
            if (self.index == buffer.len) {
                try self.expandBuffer();
            }

            buffer[self.index] = c;
            self.index += 1;
        } else {
            try self.expandBuffer();
            try self.appendChar(c);
        }
    }

    pub fn appendString(self: *Self, str: []const u8) !void {
        if (self.buffer != null) {
            while (self.index + str.len >= self.buffer.?.len) {
                try self.expandBuffer();
            }

            var updated_buffer = self.buffer.?;
            @memcpy(updated_buffer[self.index..self.index + str.len], str);
            self.index += str.len;
        } else {
            try self.expandBuffer();
            try self.appendString(str);
        }
    }

    fn expandBuffer(self: *Self) !void {
        if (self.buffer) |buffer| {
            self.buffer = try self.allocator.realloc(buffer, buffer.len * 2);
        } else {
            self.buffer = try self.allocator.alloc(u8, INIT_SIZE);
        }
    }
};

test "append char" {
    var builder = StringBuilder.init(std.testing.allocator);
    defer builder.deinit();

    try builder.appendChar('C');
    try builder.appendChar('u');
    try builder.appendChar('t');
    try builder.appendChar('e');
    try builder.shrinkToFit();

    try std.testing.expectEqualSlices(u8, "Cute", builder.buffer.?);
}

test "append string" {
    var builder = StringBuilder.init(std.testing.allocator);
    defer builder.deinit();

    try builder.appendString("Hello, ");
    try builder.appendString("World!");
    try builder.shrinkToFit();

    try std.testing.expectEqualSlices(u8, "Hello, World!", builder.buffer.?);
}

test "append string big string" {
    var builder = StringBuilder.init(std.testing.allocator);
    defer builder.deinit();

    try builder.appendString("Lorem ipsum is a dummy or placeholder text commonly used in graphic design, publishing, and web development to fill empty spaces in a layout that does not yet have content. Lorem ipsum is typically a corrupted version of De finibus bonorum et malorum, a 1st-century BC text by the Roman statesman and philosopher Cicero, with words altered, added, and removed to make it nonsensical and improper Latin. The first two words themselves are a truncation of dolorem ipsum. More at WikipediaEa nulla eius quae sed in. Qui aperiam voluptatem molestiae ratione voluptate quidem dignissimos. Ut dolores omnis quis hic voluptatem a. Occaecati facere quod cupiditate. Dolore consequatur consequatur iure. Ducimus nostrum ut consequatur aliquid quos pariatur sequi quibusdam. Non a beatae deleniti.");
    try builder.appendString(" :3");
    try builder.shrinkToFit();

    try std.testing.expectEqualSlices(u8, "Lorem ipsum is a dummy or placeholder text commonly used in graphic design, publishing, and web development to fill empty spaces in a layout that does not yet have content. Lorem ipsum is typically a corrupted version of De finibus bonorum et malorum, a 1st-century BC text by the Roman statesman and philosopher Cicero, with words altered, added, and removed to make it nonsensical and improper Latin. The first two words themselves are a truncation of dolorem ipsum. More at WikipediaEa nulla eius quae sed in. Qui aperiam voluptatem molestiae ratione voluptate quidem dignissimos. Ut dolores omnis quis hic voluptatem a. Occaecati facere quod cupiditate. Dolore consequatur consequatur iure. Ducimus nostrum ut consequatur aliquid quos pariatur sequi quibusdam. Non a beatae deleniti. :3", builder.buffer.?);
}

test "get string" {
    var builder = StringBuilder.init(std.testing.allocator);
    defer builder.deinit();

    try builder.appendString("Hello, ");
    try builder.appendString("World!");
    try builder.shrinkToFit();

    const str = try builder.getString();
    defer std.testing.allocator.free(str);

    try std.testing.expectEqualSlices(u8, "Hello, World!", str);
}

test "get string uninit" {
    var builder = StringBuilder.init(std.testing.allocator);
    defer builder.deinit();

    try std.testing.expectError(StringBuilderError.BufferNotInitialized, builder.getString());
}
