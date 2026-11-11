#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Bind {
    bool enabled = false;
    int key = 0;
    int mode = 0;
    int slot = 1;
    std::string label;
    bool prev = false;
    bool state = false;
};

struct ValueBind {
    bool enabled = false;
    int key = 0;
    int mode = 0;
    int slot = 1;
    float value = 0.0f;
    std::string label;
    float saved = 0.0f;
    bool prev = false;
    uint64_t activatedAt = 0;
};

struct SliderBinds {
    ValueBind slots[3];
    int editSlot = 0;
};

struct ConfigBrowser {
    std::string dir = "C:\\Oblivion\\Configurations";
    char search[128] = {};
    std::vector<std::string> files;
    int selected = -1;
    double lastRefresh = 0.0;
    char newName[64] = {};
    float rowHover[256] = {};
    float btnHover[8] = {};
};

extern ConfigBrowser configBrowser;

// Shared helper for bind key names. Returns a pointer to a thread-local
// buffer, so it must be consumed immediately (do not store the pointer).
const char* GetKeyName(int vk);

struct AimSettings {
    bool enable = false;
    int mode = 0;
    int key = 0;
    float fov = 180.0f;
    int hitbox = 1;
    int hitbox_priority = 0;
    float smooth = 50.0f;
    bool visible_check = false;
    bool team_check = true;
    bool fov_visualize = false;
    float fov_visualize_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool humanize_enable = false;
    bool humanize_rand_smooth = false;
    float humanize_smooth_min = 30.0f;
    float humanize_smooth_max = 70.0f;
    bool humanize_jitter = false;
    float humanize_jitter_amount = 1.5f;
    bool humanize_overshoot = false;
    float humanize_overshoot_min = 5.0f;
    float humanize_overshoot_max = 15.0f;
    bool humanize_undershoot = false;
    float humanize_undershoot_min = 5.0f;
    float humanize_undershoot_max = 15.0f;
    bool humanize_curve = false;
    float humanize_curve_amount = 0.5f;
    bool humanize_reaction_enable = false;
    float humanize_reaction_min = 120.0f;
    float humanize_reaction_max = 280.0f;

    Bind bind_enable;
    Bind bind_visible_check;
    Bind bind_team_check;
    Bind bind_fov_visualize;
    Bind bind_humanize_enable;
    Bind bind_humanize_rand_smooth;
    Bind bind_humanize_jitter;
    Bind bind_humanize_overshoot;
    Bind bind_humanize_undershoot;
    Bind bind_humanize_curve;
    Bind bind_humanize_reaction_enable;

    SliderBinds bind_fov;
    SliderBinds bind_smooth;
    SliderBinds bind_hum_smooth_min;
    SliderBinds bind_hum_smooth_max;
    SliderBinds bind_hum_jitter_amount;
    SliderBinds bind_hum_overshoot_min;
    SliderBinds bind_hum_overshoot_max;
    SliderBinds bind_hum_undershoot_min;
    SliderBinds bind_hum_undershoot_max;
    SliderBinds bind_hum_curve_amount;
    SliderBinds bind_hum_reaction_min;
    SliderBinds bind_hum_reaction_max;
};

struct AimWeaponGroup {
    std::string name;
    std::vector<std::string> weapons;
    AimSettings settings;
};

struct Config {
    bool show_menu = false;

    AimSettings aim_shared;
    std::vector<AimWeaponGroup> aim_groups;
    int aim_current_edit = 0;

    AimSettings& CurrentAimEdit() {
        if (aim_current_edit < 0) aim_current_edit = 0;
        if (aim_current_edit > (int)aim_groups.size()) aim_current_edit = 0;
        if (aim_current_edit == 0) return aim_shared;
        return aim_groups[aim_current_edit - 1].settings;
    }
    const AimSettings& CurrentAimEdit() const {
        int idx = aim_current_edit;
        if (idx <= 0 || idx > (int)aim_groups.size()) return aim_shared;
        return aim_groups[idx - 1].settings;
    }

    bool AimbotNeedsBones() const {
        if (aim_shared.enable && aim_shared.hitbox != 0) return true;
        for (const auto& g : aim_groups) {
            if (g.settings.enable && g.settings.hitbox != 0) return true;
        }
        return false;
    }

    bool esp_enemy_enable = false;
    int esp_enemy_hotkey = 0;
    int esp_enemy_hotkey_mode = 0;
    bool esp_enemy_active = false;

    bool esp_enemy_box = false;
    float esp_enemy_box_color[4] = { 1.0f, 0.2f, 0.2f, 1.0f };
    float esp_enemy_box_fill_color[4] = { 1.0f, 0.2f, 0.2f, 0.15f };
    bool esp_enemy_box_style_corner = false;
    bool esp_enemy_box_style_rounded = false;
    bool esp_enemy_box_style_fill = false;
    bool esp_enemy_box_outline = false;
    float esp_enemy_box_outline_color[4] = { 0.0f, 0.0f, 0.0f, 0.85f };
    bool esp_enemy_skeleton = false;
    float esp_enemy_skeleton_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    uint32_t esp_enemy_skeleton_joints = 0x7F;
    bool esp_enemy_skeleton_draw_joints = false;
    bool esp_enemy_name = false;
    float esp_enemy_name_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_enemy_healthbar = false;
    float esp_enemy_healthbar_color[4] = { 0.3f, 1.0f, 0.3f, 1.0f };
    bool esp_enemy_distance = false;
    float esp_enemy_distance_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_enemy_headcircle = false;
    float esp_enemy_headcircle_color[4] = { 1.0f, 0.2f, 0.2f, 1.0f };
    bool esp_enemy_snapline = false;
    float esp_enemy_snapline_color[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
    bool esp_enemy_viewdirection = false;
    float esp_enemy_viewdirection_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_enemy_weapon = false;
    float esp_enemy_weapon_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool esp_enemy_flags = false;
    float esp_enemy_flags_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_enemy_flags_flashed = false;
    float esp_enemy_flags_flashed_color[4] = { 1.0f, 1.0f, 0.4f, 1.0f };
    bool esp_enemy_flags_scoped = false;
    float esp_enemy_flags_scoped_color[4] = { 0.4f, 0.8f, 1.0f, 1.0f };
    bool esp_enemy_flags_defusing = false;
    float esp_enemy_flags_defusing_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };
    bool esp_enemy_flags_planting = false;
    float esp_enemy_flags_planting_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    bool esp_enemy_flags_bomb = false;
    float esp_enemy_flags_bomb_color[4] = { 0.8f, 0.4f, 1.0f, 1.0f };
    bool esp_enemy_flags_armor = false;
    float esp_enemy_flags_armor_color[4] = { 0.4f, 0.6f, 1.0f, 1.0f };
    bool esp_enemy_flags_reloading = false;
    float esp_enemy_flags_reloading_color[4] = { 1.0f, 0.85f, 0.4f, 1.0f };
    bool esp_enemy_flags_money = false;
    float esp_enemy_flags_money_color[4] = { 0.4f, 1.0f, 0.4f, 1.0f };
    bool esp_enemy_flags_ping = false;
    float esp_enemy_flags_ping_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_enemy_flags_ammo = false;
    float esp_enemy_flags_ammo_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool esp_enemy_offscreen = false;
    float esp_enemy_offscreen_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    float esp_enemy_offscreen_distance = 0.35f;
    float esp_enemy_offscreen_size = 22.0f;

    bool esp_team_enable = false;
    int esp_team_hotkey = 0;
    int esp_team_hotkey_mode = 0;
    bool esp_team_active = false;

    bool esp_team_box = false;
    float esp_team_box_color[4] = { 0.2f, 1.0f, 0.2f, 1.0f };
    float esp_team_box_fill_color[4] = { 0.2f, 1.0f, 0.2f, 0.15f };
    bool esp_team_box_style_corner = false;
    bool esp_team_box_style_rounded = false;
    bool esp_team_box_style_fill = false;
    bool esp_team_box_outline = false;
    float esp_team_box_outline_color[4] = { 0.0f, 0.0f, 0.0f, 0.85f };
    bool esp_team_skeleton = false;
    float esp_team_skeleton_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    uint32_t esp_team_skeleton_joints = 0x7F;
    bool esp_team_skeleton_draw_joints = false;
    bool esp_team_name = false;
    float esp_team_name_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_team_healthbar = false;
    float esp_team_healthbar_color[4] = { 0.3f, 1.0f, 0.3f, 1.0f };
    bool esp_team_distance = false;
    float esp_team_distance_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_team_headcircle = false;
    float esp_team_headcircle_color[4] = { 0.2f, 1.0f, 0.2f, 1.0f };
    bool esp_team_snapline = false;
    float esp_team_snapline_color[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
    bool esp_team_viewdirection = false;
    float esp_team_viewdirection_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_team_weapon = false;
    float esp_team_weapon_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool esp_team_flags = false;
    float esp_team_flags_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_team_flags_flashed = false;
    float esp_team_flags_flashed_color[4] = { 1.0f, 1.0f, 0.4f, 1.0f };
    bool esp_team_flags_scoped = false;
    float esp_team_flags_scoped_color[4] = { 0.4f, 0.8f, 1.0f, 1.0f };
    bool esp_team_flags_defusing = false;
    float esp_team_flags_defusing_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };
    bool esp_team_flags_planting = false;
    float esp_team_flags_planting_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    bool esp_team_flags_bomb = false;
    float esp_team_flags_bomb_color[4] = { 0.8f, 0.4f, 1.0f, 1.0f };
    bool esp_team_flags_armor = false;
    float esp_team_flags_armor_color[4] = { 0.4f, 0.6f, 1.0f, 1.0f };
    bool esp_team_flags_reloading = false;
    float esp_team_flags_reloading_color[4] = { 1.0f, 0.85f, 0.4f, 1.0f };
    bool esp_team_flags_money = false;
    float esp_team_flags_money_color[4] = { 0.4f, 1.0f, 0.4f, 1.0f };
    bool esp_team_flags_ping = false;
    float esp_team_flags_ping_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool esp_team_flags_ammo = false;
    float esp_team_flags_ammo_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool esp_team_offscreen = false;
    float esp_team_offscreen_color[4] = { 0.3f, 1.0f, 0.3f, 1.0f };
    float esp_team_offscreen_distance = 0.35f;
    float esp_team_offscreen_size = 22.0f;

    bool vsync = false;
    bool allow_tearing = false;
    bool fullscreen_mode = false;

    bool safe_mode = true;
    bool safe_mode_no_warn = false;
    bool streamproof = false;

    static constexpr int kBindEspItemCount = 35;
    Bind bind_esp[2][kBindEspItemCount];
    SliderBinds bind_esp_offscreen_dist[2];
    SliderBinds bind_esp_offscreen_size[2];

    Bind bind_vsync;
    Bind bind_allow_tearing;
    Bind bind_fullscreen_mode;
    Bind bind_streamproof;
    Bind bind_safe_mode;

    void SetDefaults();
    void Save(const std::string& path);
    void Load(const std::string& path);
    void TickEspHotkeys();
    void TickBinds();
};

extern Config config;