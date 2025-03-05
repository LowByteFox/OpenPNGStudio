const std = @import("std");
const rl = @import("raylib");

pub const mask = u64;

pub const State = enum(mask) {
    quiet = 1,
    talk = 1 << 1,
    pause = 1 << 2,
};

pub const Mod = enum(mask) {
    shift = 1 << 3,
    ctrl = 1 << 4,
    super = 1 << 5,
    meta = 1 << 6,
};

pub const KEY_START: mask = 7;

var current_mask: mask = 0;

pub fn update_mask() void {
    const key: mask = @intCast(rl.GetKeyPressed());
    if (key >= 'A' and key <= 'Z')
        current_mask |= @as(mask, 1) << @intCast((key - 'A' + KEY_START));

    if (rl.IsKeyPressed(rl.KEY_LEFT_SHIFT) or rl.IsKeyPressed(rl.KEY_RIGHT_SHIFT))
        current_mask |= @intFromEnum(Mod.shift);
    
    if (rl.IsKeyPressed(rl.KEY_LEFT_CONTROL) or rl.IsKeyPressed(rl.KEY_RIGHT_CONTROL))
        current_mask |= @intFromEnum(Mod.ctrl);

    if (rl.IsKeyPressed(rl.KEY_LEFT_SUPER) or rl.IsKeyPressed(rl.KEY_RIGHT_SUPER))
        current_mask |= @intFromEnum(Mod.super);

    if (rl.IsKeyPressed(rl.KEY_LEFT_ALT) or rl.IsKeyPressed(rl.KEY_RIGHT_ALT))
        current_mask |= @intFromEnum(Mod.meta);

    var new_mask: mask = 0;

    for (0..27) |i| {
        const bit: mask = @as(mask, 1) << @intCast((i + KEY_START));
        if (current_mask & bit != 0) {
            if (rl.IsKeyDown(@intCast(i + 'A')))
                new_mask |= bit;
        }
    }
    
    if (rl.IsKeyDown(rl.KEY_LEFT_SHIFT) or rl.IsKeyDown(rl.KEY_RIGHT_SHIFT))
        new_mask |= @intFromEnum(Mod.shift);
    
    if (rl.IsKeyDown(rl.KEY_LEFT_CONTROL) or rl.IsKeyDown(rl.KEY_RIGHT_CONTROL))
        new_mask |= @intFromEnum(Mod.ctrl);

    if (rl.IsKeyDown(rl.KEY_LEFT_SUPER) or rl.IsKeyDown(rl.KEY_RIGHT_SUPER))
        new_mask |= @intFromEnum(Mod.super);

    if (rl.IsKeyDown(rl.KEY_LEFT_ALT) or rl.IsKeyDown(rl.KEY_RIGHT_ALT))
        new_mask |= @intFromEnum(Mod.meta);

    current_mask &= new_mask;
}

pub fn get_mask() mask {
    return current_mask;
}

pub fn compare_mask(target: mask) bool {
    const states = [_]mask{ @intFromEnum(State.quiet),
        @intFromEnum(State.talk),
        @intFromEnum(State.pause),
    };

    var res = false;
    var has_mask = false;

    for (0..3) |i| {
        const e_mask = current_mask & states[i];
        const e_tar = target & states[i];

        if (e_tar == 0)
            continue;

        if (e_mask == e_tar) {
            res = true;
            break;
        }

        has_mask = true;
    }

    const mods = [_]mask{ @intFromEnum(Mod.shift),
        @intFromEnum(Mod.ctrl),
        @intFromEnum(Mod.super),
        @intFromEnum(Mod.meta),
    };

    var is_mod_set = false;

    for (0..4) |i| {
        if (target & mods[i] != 0) {
            is_mod_set = true;
            break;
        }
    }

    if (is_mod_set) {
        for (0..4) |i| {
            const e_mask = current_mask & mods[i];
            const e_tar = target & mods[i];

            if (e_mask != e_tar)
                return false;
        }

        if (has_mask and res == true) {
            res = true;
        } else if (!has_mask and res == false) {
            res = true;
        }
    }

    var has_key: i32 = -1;
    for (0..27) |i| {
        const bit: mask = @as(mask, 1) << @intCast((i + KEY_START));
        if (target & bit != 0) {
            has_key = @intCast(i);
            break;
        }
    }

    if (has_key != -1) {
        const bit: mask = @as(mask, 1) << @intCast((@as(mask, @intCast(has_key)) + KEY_START));

        if (current_mask & bit == 0)
            return false;

        if (has_mask and res == true) {
            res = true;
        } else if (!has_mask and res == false) {
            res = true;
        }
    }
    
    return true;
}
