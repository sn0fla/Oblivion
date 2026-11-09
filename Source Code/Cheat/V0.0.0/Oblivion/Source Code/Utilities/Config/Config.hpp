#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct ConfigBrowser {
    std::string dir = "C:\\Oblivion\\Configurations";
    char search[128] = {};
    std::vector<std::string> files;
    int    selected = -1;
    double lastRefresh = 0.0;
    char   newName[64] = {};
    float  rowHover[256] = {};
    float  btnHover[8] = {};
};

extern ConfigBrowser configBrowser;

struct AimSettings {
    bool  enable = false;
    int   mode = 0;
    int   key = 0;
    float fov = 180.0f;
    int   hitbox = 1;
    int   hitbox_priority = 0;
    float smooth = 50.0f;
    bool  visible_check = false;
    bool  fov_visualize = false;
    float fov_visualize_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool  humanize_enable = false;
    bool  humanize_rand_smooth = false;
    float humanize_smooth_min = 30.0f;
    float humanize_smooth_max = 70.0f;
    bool  humanize_jitter = false;
    float humanize_jitter_amount = 1.5f;
    bool  humanize_overshoot = false;
    float humanize_overshoot_min = 5.0f;
    float humanize_overshoot_max = 15.0f;
    bool  humanize_undershoot = false;
    float humanize_undershoot_min = 5.0f;
    float humanize_undershoot_max = 15.0f;
    bool  humanize_curve = false;
    float humanize_curve_amount = 0.5f;
    bool  humanize_reaction_enable = false;
    float humanize_reaction_min = 120.0f;
    float humanize_reaction_max = 280.0f;
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
    bool  esp_enemy_weapon = false;
    float esp_enemy_weapon_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool  esp_enemy_flags = false;
    float esp_enemy_flags_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool  esp_enemy_flags_flashed = false;
    float esp_enemy_flags_flashed_color[4] = { 1.0f, 1.0f, 0.4f, 1.0f };
    bool  esp_enemy_flags_scoped = false;
    float esp_enemy_flags_scoped_color[4] = { 0.4f, 0.8f, 1.0f, 1.0f };
    bool  esp_enemy_flags_defusing = false;
    float esp_enemy_flags_defusing_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };
    bool  esp_enemy_flags_planting = false;
    float esp_enemy_flags_planting_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    bool  esp_enemy_flags_bomb = false;
    float esp_enemy_flags_bomb_color[4] = { 0.8f, 0.4f, 1.0f, 1.0f };
    bool  esp_enemy_flags_armor = false;
    float esp_enemy_flags_armor_color[4] = { 0.4f, 0.6f, 1.0f, 1.0f };
    bool  esp_enemy_flags_reloading = false;
    float esp_enemy_flags_reloading_color[4] = { 1.0f, 0.85f, 0.4f, 1.0f };
    bool  esp_enemy_flags_money = false;
    float esp_enemy_flags_money_color[4] = { 0.4f, 1.0f, 0.4f, 1.0f };
    bool  esp_enemy_flags_ping = false;
    float esp_enemy_flags_ping_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool  esp_enemy_flags_ammo = false;
    float esp_enemy_flags_ammo_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool  esp_enemy_offscreen = false;
    float esp_enemy_offscreen_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    float esp_enemy_offscreen_distance = 0.35f;
    float esp_enemy_offscreen_size = 22.0f;

    bool esp_team_enable = false;
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
    bool  esp_team_weapon = false;
    float esp_team_weapon_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool  esp_team_flags = false;
    float esp_team_flags_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool  esp_team_flags_flashed = false;
    float esp_team_flags_flashed_color[4] = { 1.0f, 1.0f, 0.4f, 1.0f };
    bool  esp_team_flags_scoped = false;
    float esp_team_flags_scoped_color[4] = { 0.4f, 0.8f, 1.0f, 1.0f };
    bool  esp_team_flags_defusing = false;
    float esp_team_flags_defusing_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };
    bool  esp_team_flags_planting = false;
    float esp_team_flags_planting_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    bool  esp_team_flags_bomb = false;
    float esp_team_flags_bomb_color[4] = { 0.8f, 0.4f, 1.0f, 1.0f };
    bool  esp_team_flags_armor = false;
    float esp_team_flags_armor_color[4] = { 0.4f, 0.6f, 1.0f, 1.0f };
    bool  esp_team_flags_reloading = false;
    float esp_team_flags_reloading_color[4] = { 1.0f, 0.85f, 0.4f, 1.0f };
    bool  esp_team_flags_money = false;
    float esp_team_flags_money_color[4] = { 0.4f, 1.0f, 0.4f, 1.0f };
    bool  esp_team_flags_ping = false;
    float esp_team_flags_ping_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool  esp_team_flags_ammo = false;
    float esp_team_flags_ammo_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool  esp_team_offscreen = false;
    float esp_team_offscreen_color[4] = { 0.3f, 1.0f, 0.3f, 1.0f };
    float esp_team_offscreen_distance = 0.35f;
    float esp_team_offscreen_size = 22.0f;

    bool vsync = false;
    bool allow_tearing = false;
    bool fullscreen_mode = false;

    bool safe_mode = true;
    bool safe_mode_no_warn = false;
    bool streamproof = false;

    void SetDefaults();
    void Save(const std::string& path);
    void Load(const std::string& path);
};

extern Config config;