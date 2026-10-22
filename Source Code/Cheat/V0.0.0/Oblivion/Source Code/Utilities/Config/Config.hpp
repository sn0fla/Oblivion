#pragma once
#include <string>
#include <vector>

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

struct Config {
    bool show_menu = true;

    bool  aim_enable = false;
    int   aim_mode = 0;
    int   aim_key = 0;
    float aim_fov = 180.0f;
    int   aim_hitbox = 1;
    float aim_smooth = 50.0f;

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
    bool  esp_enemy_flags_flashed = true;
    float esp_enemy_flags_flashed_color[4] = { 1.0f, 1.0f, 0.4f, 1.0f };
    bool  esp_enemy_flags_scoped = true;
    float esp_enemy_flags_scoped_color[4] = { 0.4f, 0.8f, 1.0f, 1.0f };
    bool  esp_enemy_flags_defusing = true;
    float esp_enemy_flags_defusing_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };
    bool  esp_enemy_flags_planting = true;
    float esp_enemy_flags_planting_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    bool  esp_enemy_flags_bomb = true;
    float esp_enemy_flags_bomb_color[4] = { 0.8f, 0.4f, 1.0f, 1.0f };
    bool  esp_enemy_flags_armor = true;
    float esp_enemy_flags_armor_color[4] = { 0.4f, 0.6f, 1.0f, 1.0f };
    bool  esp_enemy_flags_reloading = true;
    float esp_enemy_flags_reloading_color[4] = { 1.0f, 0.85f, 0.4f, 1.0f };
    bool  esp_enemy_flags_money = true;
    float esp_enemy_flags_money_color[4] = { 0.4f, 1.0f, 0.4f, 1.0f };
    bool  esp_enemy_flags_ping = true;
    float esp_enemy_flags_ping_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool  esp_enemy_flags_ammo = true;
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
    bool  esp_team_flags_flashed = true;
    float esp_team_flags_flashed_color[4] = { 1.0f, 1.0f, 0.4f, 1.0f };
    bool  esp_team_flags_scoped = true;
    float esp_team_flags_scoped_color[4] = { 0.4f, 0.8f, 1.0f, 1.0f };
    bool  esp_team_flags_defusing = true;
    float esp_team_flags_defusing_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };
    bool  esp_team_flags_planting = true;
    float esp_team_flags_planting_color[4] = { 1.0f, 0.3f, 0.3f, 1.0f };
    bool  esp_team_flags_bomb = true;
    float esp_team_flags_bomb_color[4] = { 0.8f, 0.4f, 1.0f, 1.0f };
    bool  esp_team_flags_armor = true;
    float esp_team_flags_armor_color[4] = { 0.4f, 0.6f, 1.0f, 1.0f };
    bool  esp_team_flags_reloading = true;
    float esp_team_flags_reloading_color[4] = { 1.0f, 0.85f, 0.4f, 1.0f };
    bool  esp_team_flags_money = true;
    float esp_team_flags_money_color[4] = { 0.4f, 1.0f, 0.4f, 1.0f };
    bool  esp_team_flags_ping = true;
    float esp_team_flags_ping_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool  esp_team_flags_ammo = true;
    float esp_team_flags_ammo_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool  esp_team_offscreen = false;
    float esp_team_offscreen_color[4] = { 0.3f, 1.0f, 0.3f, 1.0f };
    float esp_team_offscreen_distance = 0.35f;
    float esp_team_offscreen_size = 22.0f;

    bool vsync = true;
    bool allow_tearing = true;
    bool fullscreen_mode = false;

    bool safe_mode = true;
    bool safe_mode_no_warn = false;

    void SetDefaults();
    void Save(const std::string& path);
    void Load(const std::string& path);
};

extern Config config;