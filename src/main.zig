const std = @import("std");
const path = @import("core/path.zig");
// const mask = @import("core/mask.zig");
// const rl = @import("raylib");

pub fn main() !void {
    var dbg_alloc = std.heap.DebugAllocator(.{}).init;
    defer _ = dbg_alloc.detectLeaks();
    const allocator = dbg_alloc.allocator();

    var pth = path.Path.init(allocator);
    try pth.append_dir("home");
    try pth.append_dir("jany");
    try pth.append_file("Rose");

    std.debug.print("{any}\n", .{try pth.stringify()});

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
