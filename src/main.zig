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
// const path = @import("core/path.zig");
// const rl = @import("raylib");

pub fn main() !void {
    return;
    // const screenWidth = 800;
    // const screenHeight = 450;

    // rl.InitWindow(screenWidth, screenHeight, "raylib-zig [core] example - basic window");
    // defer rl.CloseWindow();

    // rl.SetTargetFPS(60);
    // _ = mask.compare_mask(0);

    // while (!rl.WindowShouldClose()) {
    //     mask.update_mask();

    //     std.debug.print("{b:064}\n", .{mask.get_mask()});

    //     rl.BeginDrawing();
    //     defer rl.EndDrawing();

    //     rl.ClearBackground(rl.WHITE);

    //     rl.DrawText("Uwu! You created your first window!", 190, 200, 20, rl.LIGHTGRAY);
    // }
}

comptime {
    _ = @import("core/mask.zig");
    _ = @import("core/path.zig");
    _ = @import("core/string_builder.zig");
}
