#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "byte_array.h" 
#include "nav_elements.h"
#include "etc_elements.h"
#include "fontBytes.h"
#include "auth.h"
#include "memory.h"
#include "imguicpp.h"
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <cmath> 
#include <vector>
#include <cstdlib>
#include <ctime>
#include <dwmapi.h> 
#include <gdiplus.h>
#include <sstream>
#include <algorithm>
#include <comdef.h>
#include <Wbemidl.h>
#include <wininet.h>
#include <TlHelp32.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

// --- Global Variables Fixes ---
bool fock = false;
int tabs = 0;
bool hide_login = false;
bool pasCheck = false;

const char* target_title = "LIKHON CORPORATION";
std::string sheet_id = "1oG6obMc9pYmpz62FbhuifgtlEtGevtaW9eQuK9qiON4";
std::string app_ver = "LIKHON";

// Glass Theme Modes
enum GlassThemeMode {
    GLASS_THEME_OBSIDIAN = 0, // Apple VisionOS Deep Obsidian Liquid Glass
    GLASS_THEME_CRYSTAL = 1   // Fluent Crystal Frost Translucent Glass
};
static int g_glass_theme = GLASS_THEME_OBSIDIAN;
static float g_glass_opacity_scale = 0.45f; // Global glass opacity scale (lower = more sheer transparency)
static bool b_prism_caustics = true;
static int status_prism_caustics = 0;
static float g_glass_reflection_intensity = 1.0f;

static ImVec4 global_accent = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 1.0f); // Default White
#define ACCENT_U32(alpha) IM_COL32((int)(global_accent.x * 255), (int)(global_accent.y * 255), (int)(global_accent.z * 255), (alpha))

struct GifFrame { ID3D11ShaderResourceView* tex; float delay; };
std::vector<GifFrame> bg_frames; int current_frame = 0; float frame_time = 0.0f;
static char wallpaper_path[512] = ""; static char config_path[MAX_PATH] = "";
static int wallpaper_pending_action = 0; static char pending_path[512] = "";

ImFont* main_font = nullptr; ImFont* bold_font = nullptr; ImFont* title_font = nullptr; ImFont* sub_font = nullptr; ImFont* tab_icons = nullptr; ImFont* bold = nullptr; ImFont* combo_arrow = nullptr;

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

HWND g_hwnd = NULL; RECT rc = { 0 };

namespace window { ImVec2 size = { 650, 450 }; }
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
enum heads { rage, visuals, brutal, settings };

// Under-Glass Animated Particle System
struct LiquidParticle {
    ImVec2 pos;
    ImVec2 velocity;
    float radius;
    float pulse_phase;
    float speed;
};
static std::vector<LiquidParticle> particles;
static bool particles_init = false;

// --- DWM Acrylic Blur Behind + Rounded Corners (Windows 10/11) ---
typedef enum _ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5,
    ACCENT_INVALID_STATE = 6
} ACCENT_STATE;

typedef struct _ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor; // ABGR format
    DWORD AnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA {
    DWORD Attrib; // 19 = WCA_ACCENT_POLICY
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

void EnableBlurBehind(HWND hwnd) {
    // 1. Windows 11 22H2+ native Acrylic backdrop attribute (DWMWA_SYSTEMBACKDROP_TYPE = 38)
    // 1 = Auto, 2 = None, 3 = Acrylic, 4 = Mica, 5 = Tabbed
    DWORD backdropType = 3; // Acrylic
    DwmSetWindowAttribute(hwnd, 38, &backdropType, sizeof(backdropType));

    // 2. DwmEnableBlurBehindWindow (universal baseline blur)
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(hwnd, &bb);
    if (bb.hRgnBlur) DeleteObject(bb.hRgnBlur);

    // 3. SetWindowCompositionAttribute for Acrylic (Win10 1803+ / Win11)
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        auto SetWindowCompositionAttribute = (pfnSetWindowCompositionAttribute)GetProcAddress(hUser32, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute) {
            // Ultra-sheer liquid acrylic tint: ~10% alpha for crystalline transparency
            DWORD tint_alpha = (DWORD)(0x1c * g_glass_opacity_scale);
            if (tint_alpha < 0x04) tint_alpha = 0x04;
            DWORD tint_color = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                ? ((tint_alpha << 24) | 0x0018120e)
                : ((tint_alpha << 24) | 0x00ffffff);
            ACCENT_POLICY accent = { ACCENT_ENABLE_ACRYLICBLURBEHIND, 2, tint_color, 0 };
            WINDOWCOMPOSITIONATTRIBDATA data = { 19, &accent, sizeof(accent) };
            if (!SetWindowCompositionAttribute(hwnd, &data)) {
                accent.AccentState = ACCENT_ENABLE_BLURBEHIND;
                accent.GradientColor = 0;
                SetWindowCompositionAttribute(hwnd, &data);
            }
        }
    }
}

void ApplyRoundedCorners(HWND hwnd, int w, int h, int radius = 22) {
    // Win32 rounded region clip — works on ALL Windows versions
    HRGN hRgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius, radius);
    SetWindowRgn(hwnd, hRgn, TRUE);

    // Win11: Native hardware-accelerated rounded corners via DWM
    DWORD preference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

// Frame-rate independent exponential smoothing lerp
inline float ExpLerp(float current, float target, float speed, float delta) {
    return ImLerp(current, target, 1.0f - expf(-speed * delta));
}

inline ImU32 LerpColor(ImU32 a, ImU32 b, float t) {
    ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
    ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
    float alpha_mod = ImGui::GetStyle().Alpha;
    return ImGui::ColorConvertFloat4ToU32(ImVec4(
        ca.x + (cb.x - ca.x) * t,
        ca.y + (cb.y - ca.y) * t,
        ca.z + (cb.z - ca.z) * t,
        (ca.w + (cb.w - ca.w) * t) * alpha_mod
    ));
}

// Crisp text with drop shadow for legibility over translucent surfaces
inline void DrawGlassText(ImFont* font, float size, ImVec2 pos, ImU32 text_col, const char* text) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddText(font, size, pos + ImVec2(1.0f, 1.0f), IM_COL32(0, 0, 0, 155), text);
    draw->AddText(font, size, pos, text_col, text);
}

// =========================================================================
// LIQUID GLASS MATERIAL & SHADING (ImDrawList)
// =========================================================================
// Multi-layer physical liquid/curved glass shader:
// a) Diffuse ambient drop shadow (3-4 soft outer blurred passes with low alpha black)
// b) Under-glass neon backlight glow (radiating from beneath the glass)
// c) Translucent glass plate fill (alpha between 30 and 80, never fully opaque)
// d) Dual Glass Borders: 1px outer refraction line and 1px inner inset glow
// e) Specular Top Edge (Meniscus highlight): 1px bright white reflection line catching the curved bevel
// f) Prismatic chromatic dispersion on interactive hover
void DrawLiquidGlass(
    ImDrawList* draw,
    ImVec2 min, ImVec2 max,
    float rounding,
    ImU32 fill_col,
    ImU32 border_col,
    float shadow_alpha = 1.0f,
    float meniscus_intensity = 1.0f,
    ImU32 neon_glow_col = 0,
    float neon_glow_alpha = 0.0f,
    bool enable_prism = false
) {
    float global_alpha = ImGui::GetStyle().Alpha;

    // Scale glass plate fill alpha dynamically for crystal-sheer transparency
    ImVec4 fc = ImGui::ColorConvertU32ToFloat4(fill_col);
    fc.w *= g_glass_opacity_scale;
    ImU32 scaled_fill = ImGui::ColorConvertFloat4ToU32(fc);

    // Layer 1: Diffuse Ambient Drop Shadow (subtle soft outer passes, never muddy)
    if (shadow_alpha > 0.01f) {
        for (float p = 1.0f; p <= 3.0f; p += 1.0f) {
            float exp_x = p * 0.8f;
            float exp_y = p * 1.3f;
            int s_alpha = (int)((8.0f - p * 2.0f) * shadow_alpha * global_alpha * g_glass_opacity_scale);
            if (s_alpha > 0) {
                draw->AddRectFilled(
                    ImVec2(min.x - exp_x, min.y + exp_y * 0.4f),
                    ImVec2(max.x + exp_x, max.y + exp_y * 1.2f),
                    IM_COL32(0, 0, 0, s_alpha),
                    rounding + p * 1.0f
                );
            }
        }
    }

    // Layer 2: Under-Glass Neon Ambient Backlight (radiating from beneath the glass)
    if (neon_glow_alpha > 0.01f && (neon_glow_col & 0x00FFFFFF) != 0) {
        ImVec4 gc = ImGui::ColorConvertU32ToFloat4(neon_glow_col);
        for (float g = 1.0f; g <= 3.0f; g += 1.0f) {
            float g_exp = g * 2.8f;
            int g_alpha = (int)((36.0f - g * 10.0f) * neon_glow_alpha * global_alpha * g_glass_opacity_scale);
            ImU32 bloom_col = ImGui::ColorConvertFloat4ToU32(ImVec4(gc.x, gc.y, gc.z, (float)g_alpha / 255.0f));
            draw->AddRectFilled(
                min - ImVec2(g_exp, g_exp),
                max + ImVec2(g_exp, g_exp),
                bloom_col,
                rounding + g_exp * 0.75f
            );
        }
    }

    // Layer 3: Translucent Glass Plate Fill (ultra sheer, crystal clear)
    draw->AddRectFilled(min, max, scaled_fill, rounding);

    // Micro-fresnel internal gradient (liquid thickness depth - subtle sheen)
    ImVec2 half_max = ImVec2(max.x, min.y + (max.y - min.y) * 0.45f);
    draw->AddRectFilled(min, half_max, IM_COL32(255, 255, 255, (int)(5 * global_alpha * g_glass_opacity_scale)), rounding, ImDrawFlags_RoundCornersTop);

    // Layer 4: Dual Glass Borders - 1px Inner Inset Glow
    if (rounding > 2.0f) {
        draw->AddRect(
            min + ImVec2(1.0f, 1.0f),
            max - ImVec2(1.0f, 1.0f),
            IM_COL32(255, 255, 255, (int)(18 * global_alpha)),
            rounding - 1.0f,
            0,
            1.0f
        );
    }

    // Layer 5: Dual Glass Borders - 1px Outer Refraction Line
    draw->AddRect(min, max, border_col, rounding, 0, 1.0f);

    // Layer 6: Specular Top Edge (Meniscus Highlight)
    // Overhead light striking curved bevel
    if (meniscus_intensity > 0.01f && (max.x - min.x) > rounding * 1.8f) {
        float bevel_inset = rounding * 0.7f;
        ImVec2 p_start = ImVec2(min.x + bevel_inset, min.y);
        ImVec2 p_end = ImVec2(max.x - bevel_inset, min.y);

        if (enable_prism && b_prism_caustics) {
            // Chromatic prism dispersion at the specular highlight
            draw->AddLine(p_start - ImVec2(1.0f, 0.0f), p_end - ImVec2(1.0f, 0.0f), IM_COL32(90, 210, 255, (int)(80 * meniscus_intensity * global_alpha)), 1.0f);
            draw->AddLine(p_start + ImVec2(1.0f, 0.0f), p_end + ImVec2(1.0f, 0.0f), IM_COL32(255, 120, 220, (int)(70 * meniscus_intensity * global_alpha)), 1.0f);
        }

        int m_alpha = (int)(155 * meniscus_intensity * global_alpha);
        if (m_alpha > 255) m_alpha = 255;
        draw->AddLine(p_start, p_end, IM_COL32(255, 255, 255, m_alpha), 1.0f);

        // Subtle bottom secondary bounce reflection
        draw->AddLine(
            ImVec2(min.x + bevel_inset * 1.4f, max.y),
            ImVec2(max.x - bevel_inset * 1.4f, max.y),
            IM_COL32(255, 255, 255, (int)(32 * meniscus_intensity * global_alpha)),
            1.0f
        );
    }
}

// Under-Glass Animated Particle System
void RenderLiquidParticles(ImDrawList* draw, ImVec2 pos, ImVec2 size, float delta, bool anim, bool glow) {
    if (!anim) return;
    float time = (float)ImGui::GetTime();
    ImVec2 mouse_pos = ImGui::GetMousePos();
    float connect_dist = 78.0f;
    float mouse_dist = 92.0f;

    if (!particles_init || particles.empty()) {
        particles.clear();
        for (int i = 0; i < 70; i++) {
            LiquidParticle p;
            p.pos = ImVec2((float)(std::rand() % (int)size.x), (float)(std::rand() % (int)size.y));
            float angle = ((float)(std::rand() % 360)) * (3.14159f / 180.0f);
            p.speed = 12.0f + (float)(std::rand() % 24);
            p.velocity = ImVec2(cosf(angle) * p.speed, sinf(angle) * p.speed);
            p.radius = 1.6f + ((float)(std::rand() % 16) / 10.0f);
            p.pulse_phase = ((float)(std::rand() % 100) / 100.0f) * 6.28f;
            particles.push_back(p);
        }
        particles_init = true;
    }

    for (size_t i = 0; i < particles.size(); i++) {
        particles[i].pos.x += particles[i].velocity.x * delta;
        particles[i].pos.y += particles[i].velocity.y * delta;

        if (particles[i].pos.x < 4.0f) { particles[i].pos.x = 4.0f; particles[i].velocity.x *= -1.0f; }
        if (particles[i].pos.x > size.x - 4.0f) { particles[i].pos.x = size.x - 4.0f; particles[i].velocity.x *= -1.0f; }
        if (particles[i].pos.y < 4.0f) { particles[i].pos.y = 4.0f; particles[i].velocity.y *= -1.0f; }
        if (particles[i].pos.y > size.y - 4.0f) { particles[i].pos.y = size.y - 4.0f; particles[i].velocity.y *= -1.0f; }

        ImVec2 p_pos = pos + particles[i].pos;
        float pulse = sinf(time * 2.4f + particles[i].pulse_phase) * 0.35f + 0.65f;
        float r = particles[i].radius * pulse;

        // Soft outer optical halo
        draw->AddCircleFilled(p_pos, r * 3.4f, ACCENT_U32((int)(34 * pulse)));
        // Mid aura
        draw->AddCircleFilled(p_pos, r * 2.0f, ACCENT_U32((int)(90 * pulse)));
        // Core photon
        draw->AddCircleFilled(p_pos, r, IM_COL32(255, 255, 255, (int)(225 * pulse)));

        if (glow) {
            for (size_t j = i + 1; j < particles.size(); j++) {
                ImVec2 p_pos2 = pos + particles[j].pos;
                float dx = p_pos.x - p_pos2.x;
                float dy = p_pos.y - p_pos2.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < connect_dist) {
                    float factor = 1.0f - (dist / connect_dist);
                    int line_alpha = (int)(110.0f * factor * factor);
                    draw->AddLine(p_pos, p_pos2, ACCENT_U32(line_alpha), 1.0f);
                }
            }
            float m_dx = p_pos.x - mouse_pos.x;
            float m_dy = p_pos.y - mouse_pos.y;
            float m_dist = sqrtf(m_dx * m_dx + m_dy * m_dy);
            if (m_dist < mouse_dist) {
                float factor = 1.0f - (m_dist / mouse_dist);
                int m_alpha = (int)(165.0f * factor);
                draw->AddLine(p_pos, mouse_pos, ACCENT_U32(m_alpha), 1.2f);
            }
        }
    }
}

// Notifications in Liquid Glass Capsule Style
void RenderNotifications(ImVec2 window_pos, float delta) {
    float offset_y = 12.0f;
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    for (auto it = notifications.begin(); it != notifications.end(); ) {
        it->timer -= delta;
        if (it->timer <= 0.0f) { it = notifications.erase(it); continue; }

        float alpha_anim = 1.0f; float slide_x = 0.0f;
        if (it->timer > 2.7f) {
            float t = (3.0f - it->timer) / 0.3f;
            slide_x = -(1.0f - t) * 18.0f;
            alpha_anim = t;
        }
        else if (it->timer < 0.3f) {
            alpha_anim = it->timer / 0.3f;
        }

        ImGui::PushFont(sub_font);
        ImVec2 text_size = ImGui::CalcTextSize(it->message.c_str());
        ImGui::PopFont();

        float box_w = text_size.x + 44.0f;
        float box_h = 34.0f;
        ImVec2 box_min = ImVec2(window_pos.x + 14.0f + slide_x, window_pos.y + offset_y);
        ImVec2 box_max = ImVec2(box_min.x + box_w, box_min.y + box_h);

        ImU32 dot_theme, glow_inner, glow_outer;
        if (it->type == Notify_Success) {
            dot_theme = IM_COL32(65, 225, 140, (int)(255 * alpha_anim));
            glow_inner = IM_COL32(65, 225, 140, (int)(130 * alpha_anim));
            glow_outer = IM_COL32(65, 225, 140, (int)(50 * alpha_anim));
        }
        else if (it->type == Notify_Warning) {
            dot_theme = IM_COL32(245, 195, 55, (int)(255 * alpha_anim));
            glow_inner = IM_COL32(245, 195, 55, (int)(130 * alpha_anim));
            glow_outer = IM_COL32(245, 195, 55, (int)(50 * alpha_anim));
        }
        else {
            dot_theme = IM_COL32(255, 75, 85, (int)(255 * alpha_anim));
            glow_inner = IM_COL32(255, 75, 85, (int)(130 * alpha_anim));
            glow_outer = IM_COL32(255, 75, 85, (int)(50 * alpha_anim));
        }

        float radius = box_h * 0.5f;
        ImU32 bg_col = (g_glass_theme == GLASS_THEME_OBSIDIAN)
            ? IM_COL32(18, 24, 34, (int)(160 * alpha_anim))
            : IM_COL32(255, 255, 255, (int)(75 * alpha_anim));
        ImU32 border_col = IM_COL32(255, 255, 255, (int)(75 * alpha_anim));

        DrawLiquidGlass(draw, box_min, box_max, radius, bg_col, border_col, alpha_anim, 1.2f, dot_theme, 0.4f * alpha_anim, true);

        ImVec2 circle_pos = ImVec2(box_min.x + 16.0f, box_min.y + box_h * 0.5f);
        draw->AddCircleFilled(circle_pos, 8.5f, glow_outer);
        draw->AddCircleFilled(circle_pos, 6.0f, glow_inner);
        draw->AddCircleFilled(circle_pos, 4.0f, dot_theme);
        draw->AddCircleFilled(circle_pos - ImVec2(1.0f, 1.0f), 1.2f, IM_COL32(255, 255, 255, (int)(240 * alpha_anim)));

        ImU32 text_col = IM_COL32(245, 248, 255, (int)(255 * alpha_anim));
        draw->AddText(sub_font, 14.0f, ImVec2(box_min.x + 30.0f, box_min.y + (box_h - text_size.y) * 0.5f) + ImVec2(1.0f, 1.0f), IM_COL32(0, 0, 0, (int)(160 * alpha_anim)), it->message.c_str());
        draw->AddText(sub_font, 14.0f, ImVec2(box_min.x + 30.0f, box_min.y + (box_h - text_size.y) * 0.5f), text_col, it->message.c_str());

        offset_y += box_h + 8.0f;
        ++it;
    }
}

// =========================================================================
// LIQUID GLASS COMPONENT REDESIGN
// =========================================================================

bool CustomSelectable(const char* label, bool active) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 38.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID(label);

    ImGui::ItemSize(total_size);
    ImVec2 btn_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 btn_size = ImVec2(total_size.x - shadow_margin * 2.0f, 38.0f);
    ImRect bb(btn_pos, btn_pos + btn_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    // Frame-rate independent lerp animations
    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, hovered ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float active_anim = window->StateStorage.GetFloat(id + 2, 0.0f);
    active_anim = ExpLerp(active_anim, active ? 1.0f : 0.0f, 16.0f, delta);
    window->StateStorage.SetFloat(id + 2, active_anim);

    // Dynamic expand on hover (+1.5px)
    float expand = 1.5f * hover_anim;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 10.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(32 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(16 * global_alpha));
    ImU32 hover_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(28, 38, 56, (int)(65 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(38 * global_alpha));
    ImU32 active_bg = ACCENT_U32((int)(45 * global_alpha));

    ImU32 bg_col = LerpColor(base_bg, hover_bg, hover_anim);
    bg_col = LerpColor(bg_col, active_bg, active_anim);

    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(45 * global_alpha)),
        ACCENT_U32((int)(215 * global_alpha)),
        active_anim
    );
    if (hover_anim > 0.01f && active_anim < 0.5f) {
        border_col = LerpColor(border_col, IM_COL32(255, 255, 255, (int)(85 * global_alpha)), hover_anim);
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 neon_glow = ACCENT_U32(255);
    float neon_alpha = active_anim * 0.85f + hover_anim * 0.25f;

    DrawLiquidGlass(
        draw, draw_min, draw_max, rounding,
        bg_col, border_col,
        0.7f, 1.1f + 0.3f * hover_anim,
        neon_glow, neon_alpha,
        hovered
    );

    ImGui::PushFont(bold_font);
    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImGui::PopFont();

    ImU32 text_col = LerpColor(
        IM_COL32(195, 208, 230, (int)(230 * global_alpha)),
        IM_COL32(255, 255, 255, (int)(255 * global_alpha)),
        ImMax(hover_anim, active_anim)
    );

    ImVec2 text_pos = bb.Min + ImVec2((btn_size.x - text_size.x) * 0.5f, (btn_size.y - text_size.y) * 0.5f);
    DrawGlassText(bold_font, 16.0f, text_pos, text_col, label);

    return pressed;
}

bool CustomToggleButton(const char* label, const char* sub_label, bool* v, int* status = nullptr) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 52.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID(label);

    ImGui::ItemSize(total_size);
    ImVec2 btn_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 btn_size = ImVec2(total_size.x - shadow_margin * 2.0f, 52.0f);
    ImRect bb(btn_pos, btn_pos + btn_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) {
        *v = !*v;
        if (*v) {
            if (status != nullptr) *status = 1;
            AddNotification(std::string(label) + " Activated", Notify_Warning);
        }
        else {
            if (status != nullptr) *status = 0;
            AddNotification(std::string(label) + " Disabled", Notify_Warning);
        }
    }

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, hovered ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float active_anim = window->StateStorage.GetFloat(id + 2, 0.0f);
    active_anim = ExpLerp(active_anim, *v ? 1.0f : 0.0f, 16.0f, delta);
    window->StateStorage.SetFloat(id + 2, active_anim);

    float expand = 1.5f * hover_anim;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 11.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(30 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(15 * global_alpha));
    ImU32 hover_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(28, 38, 56, (int)(62 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(38 * global_alpha));
    ImU32 active_bg = ACCENT_U32((int)(40 * global_alpha));

    ImU32 bg_col = LerpColor(base_bg, hover_bg, hover_anim);
    bg_col = LerpColor(bg_col, active_bg, active_anim);

    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(48 * global_alpha)),
        ACCENT_U32((int)(200 * global_alpha)),
        active_anim
    );
    if (hover_anim > 0.01f && active_anim < 0.5f) {
        border_col = LerpColor(border_col, IM_COL32(255, 255, 255, (int)(85 * global_alpha)), hover_anim);
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 neon_glow = ACCENT_U32(255);
    float neon_alpha = active_anim * 0.75f + hover_anim * 0.2f;

    DrawLiquidGlass(
        draw, draw_min, draw_max, rounding,
        bg_col, border_col,
        0.75f, 1.1f + 0.3f * hover_anim,
        neon_glow, neon_alpha,
        hovered
    );

    // High contrast crisp text with drop shadow
    ImU32 title_col = IM_COL32(245, 248, 255, (int)(255 * global_alpha));
    ImU32 sub_col = IM_COL32(160, 175, 205, (int)(220 * global_alpha));
    DrawGlassText(bold_font, 17.0f, btn_pos + ImVec2(14, 6), title_col, label);
    DrawGlassText(sub_font, 13.0f, btn_pos + ImVec2(14, 29), sub_col, sub_label);

    // Glowing Liquid Jewel / Indicator Orb
    int current_status = (status != nullptr) ? *status : 0;
    ImU32 dot_theme, glow_inner, glow_outer;
    if (current_status == 1) { // Warning / In Progress
        dot_theme = IM_COL32(245, 195, 55, 255);
        glow_inner = IM_COL32(245, 195, 55, (int)(135 * active_anim * global_alpha));
        glow_outer = IM_COL32(245, 195, 55, (int)(55 * active_anim * global_alpha));
    }
    else if (current_status == 2) { // Error
        dot_theme = IM_COL32(255, 75, 85, 255);
        glow_inner = IM_COL32(255, 75, 85, (int)(135 * active_anim * global_alpha));
        glow_outer = IM_COL32(255, 75, 85, (int)(55 * active_anim * global_alpha));
    }
    else { // Active (Neon Accent)
        dot_theme = ACCENT_U32(255);
        glow_inner = ACCENT_U32((int)(135 * active_anim * global_alpha));
        glow_outer = ACCENT_U32((int)(55 * active_anim * global_alpha));
    }

    ImVec2 circle_pos = ImVec2(bb.Max.x - 20.0f, bb.Min.y + 26.0f);
    ImU32 dot_core_col = LerpColor(IM_COL32(255, 255, 255, 45), dot_theme, active_anim);

    if (active_anim > 0.01f) {
        draw->AddCircleFilled(circle_pos, 8.5f * active_anim, glow_outer);
        draw->AddCircleFilled(circle_pos, 6.0f * active_anim, glow_inner);
    }
    draw->AddCircleFilled(circle_pos, 4.5f, dot_core_col);
    if (active_anim > 0.01f) {
        draw->AddCircleFilled(circle_pos - ImVec2(1.0f, 1.0f), 1.3f, IM_COL32(255, 255, 255, (int)(240 * active_anim * global_alpha)));
    }

    return pressed;
}

bool CustomButton(const char* label) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 40.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID(label);

    ImGui::ItemSize(total_size);
    ImVec2 btn_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 btn_size = ImVec2(total_size.x - shadow_margin * 2.0f, 40.0f);
    ImRect bb(btn_pos, btn_pos + btn_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, hovered ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float held_anim = window->StateStorage.GetFloat(id + 2, 0.0f);
    held_anim = ExpLerp(held_anim, held ? 1.0f : 0.0f, 18.0f, delta);
    window->StateStorage.SetFloat(id + 2, held_anim);

    float expand = 1.5f * hover_anim - held_anim * 1.0f;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 10.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(32 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(16 * global_alpha));
    ImU32 hover_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(28, 40, 60, (int)(68 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(40 * global_alpha));

    ImU32 bg_col = LerpColor(base_bg, hover_bg, hover_anim);
    if (held) bg_col = ACCENT_U32((int)(60 * global_alpha));

    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(55 * global_alpha)),
        ACCENT_U32((int)(200 * global_alpha)),
        hover_anim * 0.65f
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawLiquidGlass(
        draw, draw_min, draw_max, rounding,
        bg_col, border_col,
        0.8f, 1.2f + 0.3f * hover_anim,
        ACCENT_U32(255), hover_anim * 0.45f,
        hovered
    );

    ImGui::PushFont(bold_font);
    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImGui::PopFont();

    ImVec2 text_pos = bb.Min + ImVec2((btn_size.x - text_size.x) * 0.5f, (btn_size.y - text_size.y) * 0.5f);
    ImU32 text_col = LerpColor(
        IM_COL32(220, 230, 245, (int)(245 * global_alpha)),
        IM_COL32(255, 255, 255, (int)(255 * global_alpha)),
        hover_anim
    );
    DrawGlassText(bold_font, 17.0f, text_pos, text_col, label);

    return pressed;
}

// Inset Frosted Glass Channel Sliders with Glowing Liquid Grabbers
bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f") {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 56.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID(label);

    ImGui::ItemSize(total_size);
    ImVec2 box_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 box_size = ImVec2(total_size.x - shadow_margin * 2.0f, 56.0f);
    ImRect bb(box_pos, box_pos + box_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (held) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float track_width = box_size.x - 28.0f;
        float fill_ratio = ImClamp((mouse_x - (bb.Min.x + 14.0f)) / track_width, 0.0f, 1.0f);
        *v = v_min + fill_ratio * (v_max - v_min);
    }

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, (hovered || held) ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float expand = 1.0f * hover_anim;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 11.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(30 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(15 * global_alpha));
    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(45 * global_alpha)),
        IM_COL32(255, 255, 255, (int)(75 * global_alpha)),
        hover_anim
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawLiquidGlass(draw, draw_min, draw_max, rounding, base_bg, border_col, 0.7f, 1.1f, 0, 0.0f, false);

    // Title label & Value display
    DrawGlassText(bold_font, 16.0f, bb.Min + ImVec2(14, 8), IM_COL32(245, 248, 255, (int)(255 * global_alpha)), label);

    char val_buf[64];
    sprintf_s(val_buf, format, *v);
    ImGui::PushFont(bold_font);
    ImVec2 val_size = ImGui::CalcTextSize(val_buf);
    ImGui::PopFont();

    // Value badge pill
    ImVec2 badge_min = ImVec2(bb.Max.x - 14.0f - val_size.x - 12.0f, bb.Min.y + 7.0f);
    ImVec2 badge_max = ImVec2(bb.Max.x - 14.0f, bb.Min.y + 25.0f);
    draw->AddRectFilled(badge_min, badge_max, IM_COL32(255, 255, 255, (int)(20 * global_alpha)), 6.0f);
    draw->AddRect(badge_min, badge_max, IM_COL32(255, 255, 255, (int)(35 * global_alpha)), 6.0f);
    draw->AddText(bold_font, 14.0f, badge_min + ImVec2(6.0f, 1.0f), IM_COL32(215, 230, 250, (int)(240 * global_alpha)), val_buf);

    // Inset Frosted Glass Channel
    ImVec2 track_min = ImVec2(bb.Min.x + 14, bb.Min.y + 38);
    ImVec2 track_max = ImVec2(bb.Max.x - 14, bb.Min.y + 43);
    float track_round = 2.5f;

    // Inset channel fill
    draw->AddRectFilled(track_min, track_max, IM_COL32(10, 14, 22, (int)(75 * global_alpha * g_glass_opacity_scale)), track_round);
    draw->AddRect(track_min, track_max, IM_COL32(255, 255, 255, (int)(25 * global_alpha)), track_round);
    // Inset top shadow line
    draw->AddLine(track_min + ImVec2(1, 1), ImVec2(track_max.x - 1, track_min.y + 1), IM_COL32(0, 0, 0, (int)(50 * global_alpha * g_glass_opacity_scale)));

    // Liquid Fill Progression
    float fill_ratio = (v_max > v_min) ? (*v - v_min) / (v_max - v_min) : 0.0f;
    fill_ratio = ImClamp(fill_ratio, 0.0f, 1.0f);
    ImVec2 fill_max = ImVec2(track_min.x + fill_ratio * (track_max.x - track_min.x), track_max.y);

    if (fill_ratio > 0.005f) {
        ImU32 fill_col = LerpColor(ACCENT_U32(185), ACCENT_U32(255), hover_anim);
        draw->AddRectFilled(track_min, fill_max, fill_col, track_round);
        // Specular top highlight along liquid fill
        draw->AddLine(track_min + ImVec2(1, 0), fill_max - ImVec2(1, 4), IM_COL32(255, 255, 255, (int)(160 * global_alpha)));
    }

    // Glowing Liquid Grabber Orb
    float grabber_rad = 6.5f + (hover_anim * 2.0f);
    ImVec2 grabber_pos = ImVec2(fill_max.x, track_min.y + 2.5f);

    // Outer neon bloom halo
    draw->AddCircleFilled(grabber_pos, grabber_rad * 2.0f, ACCENT_U32((int)(55 * global_alpha)));
    // Mid liquid orb
    draw->AddCircleFilled(grabber_pos, grabber_rad, ACCENT_U32((int)(240 * global_alpha)));
    // Specular highlight spot (3D glassy bead look)
    draw->AddCircleFilled(grabber_pos - ImVec2(1.8f, 1.8f), grabber_rad * 0.35f, IM_COL32(255, 255, 255, (int)(250 * global_alpha)));
    draw->AddCircle(grabber_pos, grabber_rad, IM_COL32(255, 255, 255, (int)(180 * global_alpha)), 0, 1.0f);

    return pressed || held;
}

bool CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d") {
    float fv = (float)*v;
    bool res = CustomSliderFloat(label, &fv, (float)v_min, (float)v_max, format);
    *v = (int)(fv + 0.5f);
    if (*v < v_min) *v = v_min;
    if (*v > v_max) *v = v_max;
    return res;
}

// Inset Frosted Glass Input Channels
bool CustomInputText(const char* label, char* buf, size_t buf_size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 60.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID(label);

    ImVec2 box_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 box_size = ImVec2(total_size.x - shadow_margin * 2.0f, 60.0f);
    ImRect bb(box_pos, box_pos + box_size);

    ImGui::ItemSize(total_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    bool active = ImGui::IsItemActive();

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, (hovered || active) ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float expand = 1.0f * hover_anim;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 11.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(30 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(15 * global_alpha));
    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(45 * global_alpha)),
        ACCENT_U32((int)(190 * global_alpha)),
        hover_anim
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawLiquidGlass(draw, draw_min, draw_max, rounding, base_bg, border_col, 0.7f, 1.1f, 0, 0.0f, false);

    DrawGlassText(bold_font, 16.0f, bb.Min + ImVec2(14, 8), IM_COL32(245, 248, 255, (int)(255 * global_alpha)), label);

    // Inset frosted glass inner input box
    ImVec2 inner_min = bb.Min + ImVec2(12, 30);
    ImVec2 inner_max = bb.Max - ImVec2(12, 6);
    draw->AddRectFilled(inner_min, inner_max, IM_COL32(10, 14, 22, (int)(75 * global_alpha * g_glass_opacity_scale)), 6.0f);
    draw->AddRect(inner_min, inner_max, IM_COL32(255, 255, 255, (int)(25 * global_alpha)), 6.0f);

    ImVec2 backup_pos = window->DC.CursorPos;
    ImGui::SetCursorScreenPos(inner_min + ImVec2(8, 3));
    ImGui::PushItemWidth(inner_max.x - inner_min.x - 16.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(245, 248, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ACCENT_U32(120));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushFont(bold_font);
    ImGui::PushID(label);
    ImGui::InputText("##input", buf, buf_size);
    ImGui::PopID();
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(6);
    ImGui::PopItemWidth();
    window->DC.CursorPos = backup_pos;

    return false;
}

bool CustomKeybind(const char* label, const char* sub_label, int* key) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 52.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID(label);

    ImVec2 box_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 box_size = ImVec2(total_size.x - shadow_margin * 2.0f, 52.0f);
    ImRect bb(box_pos, box_pos + box_size);

    ImGui::ItemSize(total_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    static ImGuiID active_keybind = 0;
    static bool wait_for_release = false;
    if (pressed) {
        if (active_keybind == id) active_keybind = 0;
        else { active_keybind = id; wait_for_release = true; }
    }
    bool is_active = (active_keybind == id);
    if (is_active) {
        if (wait_for_release) {
            bool any_down = false;
            for (int i = 1; i < 256; i++) {
                if (GetAsyncKeyState(i) & 0x8000) { any_down = true; break; }
            }
            if (!any_down) wait_for_release = false;
        }
        else {
            for (int i = 1; i < 256; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    *key = (i == VK_ESCAPE) ? 0 : i;
                    active_keybind = 0;
                    break;
                }
            }
        }
    }

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, (hovered || is_active) ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float expand = 1.0f * hover_anim;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 11.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(30 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(15 * global_alpha));
    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(45 * global_alpha)),
        ACCENT_U32((int)(200 * global_alpha)),
        is_active ? 1.0f : (hover_anim * 0.5f)
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawLiquidGlass(draw, draw_min, draw_max, rounding, base_bg, border_col, 0.7f, 1.1f, ACCENT_U32(255), is_active ? 0.6f : 0.0f, false);

    DrawGlassText(bold_font, 17.0f, bb.Min + ImVec2(14, 6), IM_COL32(245, 248, 255, (int)(255 * global_alpha)), label);
    DrawGlassText(sub_font, 13.0f, bb.Min + ImVec2(14, 29), IM_COL32(160, 175, 205, (int)(220 * global_alpha)), sub_label);

    const char* key_name = "None";
    char key_buf[64] = { 0 };
    if (is_active) key_name = ". . .";
    else if (*key != 0) {
        int k = *key;
        if (k == VK_LBUTTON) key_name = "M1";
        else if (k == VK_RBUTTON) key_name = "M2";
        else if (k == VK_MBUTTON) key_name = "M3";
        else if (k == VK_XBUTTON1) key_name = "M4";
        else if (k == VK_XBUTTON2) key_name = "M5";
        else if (k == VK_SHIFT || k == VK_LSHIFT || k == VK_RSHIFT) key_name = "Shift";
        else if (k == VK_CONTROL || k == VK_LCONTROL || k == VK_RCONTROL) key_name = "Ctrl";
        else if (k == VK_MENU || k == VK_LMENU || k == VK_RMENU) key_name = "Alt";
        else if (k == VK_SPACE) key_name = "Space";
        else {
            UINT scanCode = MapVirtualKeyA(k, MAPVK_VK_TO_VSC);
            if (GetKeyNameTextA(scanCode << 16, key_buf, sizeof(key_buf)) != 0) key_name = key_buf;
            else { sprintf_s(key_buf, "Key %d", k); key_name = key_buf; }
        }
    }

    ImGui::PushFont(bold_font);
    ImVec2 val_size = ImGui::CalcTextSize(key_name);
    ImGui::PopFont();

    float badge_pad_x = 12.0f;
    float badge_pad_y = 6.0f;
    float badge_w = val_size.x + badge_pad_x * 2.0f;
    float badge_h = val_size.y + badge_pad_y * 2.0f;
    if (badge_w < 44.0f) badge_w = 44.0f;

    ImVec2 badge_min = ImVec2(bb.Max.x - 14.0f - badge_w, bb.Min.y + (box_size.y - badge_h) * 0.5f);
    ImVec2 badge_max = badge_min + ImVec2(badge_w, badge_h);

    float pulse = is_active ? (sinf((float)ImGui::GetTime() * 8.0f) * 0.5f + 0.5f) : 0.0f;
    ImU32 badge_bg = is_active
        ? ACCENT_U32((int)(120 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(25 * global_alpha));
    ImU32 badge_border = is_active
        ? ACCENT_U32((int)(240 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(50 * global_alpha));

    if (is_active) {
        draw->AddRect(badge_min - ImVec2(pulse * 3.0f, pulse * 3.0f), badge_max + ImVec2(pulse * 3.0f, pulse * 3.0f), ACCENT_U32((int)(140 * (1.0f - pulse) * global_alpha)), 8.0f, 0, 1.5f);
    }
    draw->AddRectFilled(badge_min, badge_max, badge_bg, 7.0f);
    draw->AddRect(badge_min, badge_max, badge_border, 7.0f, 0, 1.0f);

    ImVec2 text_badge_pos = ImVec2(badge_min.x + (badge_w - val_size.x) * 0.5f, badge_min.y + (badge_h - val_size.y) * 0.5f);
    DrawGlassText(bold_font, 15.0f, text_badge_pos, IM_COL32(245, 248, 255, (int)(255 * global_alpha)), key_name);

    return pressed;
}

bool CustomColorEdit(const char* label, const char* sub_label, ImVec4* col) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGui::PushID(label);

    float shadow_margin = 3.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 total_size = ImVec2(ImGui::GetContentRegionAvail().x, 52.0f + shadow_margin * 1.5f);
    ImGuiID id = window->GetID("##color_box");

    ImGui::ItemSize(total_size);
    ImVec2 box_pos = pos + ImVec2(shadow_margin, shadow_margin * 0.5f);
    ImVec2 box_size = ImVec2(total_size.x - shadow_margin * 2.0f, 52.0f);
    ImRect bb(box_pos, box_pos + box_size);

    if (!ImGui::ItemAdd(bb, id)) { ImGui::PopID(); return false; }
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) ImGui::OpenPopup("##color_picker");

    float delta = ImGui::GetIO().DeltaTime;
    float global_alpha = ImGui::GetStyle().Alpha;

    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, hovered ? 1.0f : 0.0f, 14.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    float expand = 1.0f * hover_anim;
    ImVec2 draw_min = bb.Min - ImVec2(expand, expand);
    ImVec2 draw_max = bb.Max + ImVec2(expand, expand);
    float rounding = 11.0f + (expand * 0.5f);

    ImU32 base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
        ? IM_COL32(18, 24, 36, (int)(30 * global_alpha))
        : IM_COL32(255, 255, 255, (int)(15 * global_alpha));
    ImU32 border_col = LerpColor(
        IM_COL32(255, 255, 255, (int)(45 * global_alpha)),
        ACCENT_U32((int)(190 * global_alpha)),
        hover_anim
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawLiquidGlass(draw, draw_min, draw_max, rounding, base_bg, border_col, 0.7f, 1.1f, 0, 0.0f, false);

    DrawGlassText(bold_font, 17.0f, bb.Min + ImVec2(14, 6), IM_COL32(245, 248, 255, (int)(255 * global_alpha)), label);
    DrawGlassText(sub_font, 13.0f, bb.Min + ImVec2(14, 29), IM_COL32(160, 175, 205, (int)(220 * global_alpha)), sub_label);

    // Floating glass color swatch preview
    ImU32 prev_col = ImGui::ColorConvertFloat4ToU32(*col);
    ImVec2 p_min = ImVec2(bb.Max.x - 44.0f, bb.Min.y + 12.0f);
    ImVec2 p_max = ImVec2(bb.Max.x - 14.0f, bb.Min.y + 40.0f);
    draw->AddRectFilled(p_min, p_max, prev_col, 7.0f);
    draw->AddRect(p_min, p_max, IM_COL32(255, 255, 255, (int)(180 * global_alpha)), 7.0f, 0, 1.0f);
    draw->AddLine(p_min + ImVec2(3.0f, 0.0f), ImVec2(p_max.x - 3.0f, p_min.y), IM_COL32(255, 255, 255, 220), 1.0f);

    bool value_changed = false;
    ImGui::SetNextWindowPos(ImVec2(bb.Max.x + 5, bb.Min.y - 100), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("##color_picker")) {
        if (ImGui::ColorPicker4("##picker", (float*)col, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview))
            value_changed = true;
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return value_changed;
}

// Modern Circular Liquid Glass Close Button
bool CustomCloseButton() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImVec2 btn_size(24.0f, 24.0f);
    ImVec2 pos = window->DC.CursorPos;
    ImGuiID id = window->GetID("CloseBtn");
    ImRect bb(pos, pos + btn_size);

    ImGui::ItemSize(btn_size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    float delta = ImGui::GetIO().DeltaTime;
    float hover_anim = window->StateStorage.GetFloat(id + 1, 0.0f);
    hover_anim = ExpLerp(hover_anim, hovered ? 1.0f : 0.0f, 15.0f, delta);
    window->StateStorage.SetFloat(id + 1, hover_anim);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 center = pos + ImVec2(btn_size.x * 0.5f, btn_size.y * 0.5f);

    float radius = 9.5f + (hover_anim * 1.5f);
    ImU32 normal_bg = IM_COL32(255, 255, 255, 30);
    ImU32 hover_bg = IM_COL32(255, 65, 75, 220);
    ImU32 bg_col = LerpColor(normal_bg, hover_bg, hover_anim);

    if (hover_anim > 0.01f) {
        draw->AddCircleFilled(center, radius * 1.8f, IM_COL32(255, 60, 70, (int)(75 * hover_anim)));
    }
    draw->AddCircleFilled(center, radius, bg_col);
    draw->AddCircle(center, radius, IM_COL32(255, 255, 255, (int)(60 + hover_anim * 120)), 0, 1.0f);
    draw->AddCircleFilled(center - ImVec2(2.0f, 2.0f), 1.5f, IM_COL32(255, 255, 255, 180));

    float cross_size = 3.2f;
    ImU32 cross_col = IM_COL32(255, 255, 255, 255);
    draw->AddLine(center - ImVec2(cross_size, cross_size), center + ImVec2(cross_size, cross_size), cross_col, 1.4f);
    draw->AddLine(center + ImVec2(cross_size, -cross_size), center - ImVec2(cross_size, -cross_size), cross_col, 1.4f);

    return pressed;
}

void move_window() {
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("Move_detector", window::size);
    ImGui::SetItemAllowOverlap();
    static POINT drag_start_pos;
    static RECT window_start_rect;
    static bool is_dragging = false;
    if (ImGui::IsItemActive()) {
        if (!is_dragging) {
            GetCursorPos(&drag_start_pos);
            GetWindowRect(g_hwnd, &window_start_rect);
            is_dragging = true;
        }
        else {
            POINT current_pos;
            GetCursorPos(&current_pos);
            int dx = current_pos.x - drag_start_pos.x;
            int dy = current_pos.y - drag_start_pos.y;
            MoveWindow(g_hwnd, window_start_rect.left + dx, window_start_rect.top + dy, (int)window::size.x, (int)window::size.y, TRUE);
        }
    }
    else is_dragging = false;
}

bool LoadWallpaperFromFile(const char* path) {
    if (!g_pd3dDevice) return false;
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (wchars_num <= 1) return false;
    std::wstring wpath(wchars_num, 0);
    MultiByteToWideChar(CP_UTF8, 0, path, -1, &wpath[0], wchars_num);
    Gdiplus::Image* img = Gdiplus::Image::FromFile(wpath.c_str());
    if (!img || img->GetLastStatus() != Gdiplus::Ok) { if (img) delete img; return false; }
    for (auto& frame : bg_frames) { if (frame.tex) frame.tex->Release(); }
    bg_frames.clear(); current_frame = 0; frame_time = 0.0f;

    GUID dimID;
    img->GetFrameDimensionsList(&dimID, 1);
    UINT frameCount = img->GetFrameCount(&dimID);
    UINT propSize = img->GetPropertyItemSize(PropertyTagFrameDelay);
    Gdiplus::PropertyItem* pItem = propSize > 0 ? (Gdiplus::PropertyItem*)malloc(propSize) : NULL;
    if (pItem) img->GetPropertyItem(PropertyTagFrameDelay, propSize, pItem);
    long* delays = pItem ? (long*)pItem->value : nullptr;

    for (UINT i = 0; i < frameCount; i++) {
        img->SelectActiveFrame(&dimID, i);
        Gdiplus::Bitmap* bmp = (Gdiplus::Bitmap*)img;
        Gdiplus::BitmapData bmpData;
        Gdiplus::Rect rect(0, 0, bmp->GetWidth(), bmp->GetHeight());
        bmp->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
        int w = bmp->GetWidth(); int h = bmp->GetHeight();
        unsigned char* pixels = (unsigned char*)bmpData.Scan0;
        int stride = abs(bmpData.Stride);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                pixels[y * stride + x * 4 + 3] = 255;
            }
        }

        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = bmp->GetWidth();
        desc.Height = bmp->GetHeight();
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData;
        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = bmpData.Scan0;
        initData.SysMemPitch = bmpData.Stride;

        ID3D11Texture2D* pTexture = NULL;
        HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, &pTexture);
        ID3D11ShaderResourceView* pSRV = NULL;
        if (SUCCEEDED(hr) && pTexture) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
            pTexture->Release();
        }

        if (SUCCEEDED(hr) && pSRV) {
            GifFrame frame;
            frame.tex = pSRV;
            frame.delay = (delays && i < propSize / sizeof(long)) ? (delays[i] * 0.01f) : 0.1f;
            if (frame.delay < 0.01f) frame.delay = 0.1f;
            bg_frames.push_back(frame);
        }
        bmp->UnlockBits(&bmpData);
    }
    if (pItem) free(pItem);
    delete img;
    return !bg_frames.empty();
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

    int app_state = 0; // Start with intro & HWID authentication check

    std::thread gg(KeyBindsFunction); gg.detach();
    StartFilterThread();
    std::thread(loopsOfaim).detach();

    std::srand((unsigned int)std::time(nullptr));
    strcpy_s(config_path, MAX_PATH, "C:\\Windows\\config.ini");

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Cristal LiquidGlass", NULL };
    ::RegisterClassExW(&wc);

    // True alpha compositing window with DWM Acrylic & blur behind
    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED,
        wc.lpszClassName,
        L"Cristal Liquid Glass",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) / 2) - ((int)window::size.x / 2),
        (GetSystemMetrics(SM_CYSCREEN) / 2) - ((int)window::size.y / 2),
        (int)window::size.x,
        (int)window::size.y,
        0, 0, wc.hInstance, 0
    );
    C c; c.E(); c.T();
    // Initialize layered window composition
    ::SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

    // DWM extended frame into entire client area for true glass composition
    MARGINS margins = { -1, -1, -1, -1 };
    ::DwmExtendFrameIntoClientArea(g_hwnd, &margins);

    // Hardware-accelerated rounded corners (22px radius for polished curved glass edges)
    ApplyRoundedCorners(g_hwnd, (int)window::size.x, (int)window::size.y, 22);

    // Enable Windows 10/11 Fluent Acrylic / Blur Behind
    EnableBlurBehind(g_hwnd);

    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    GetPrivateProfileStringA("Settings", "Wallpaper", "", wallpaper_path, sizeof(wallpaper_path), config_path);
    if (wallpaper_path[0] != '\0') LoadWallpaperFromFile(wallpaper_path);

    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.IniFilename = NULL;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Dear ImGui Style Tweaks for Liquid Glass Design
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 14.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 12.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);

    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 3;
    font_config.RasterizerMultiply = 1.15f;
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0400, 0x052F, 0x2DE0, 0x2DFF, 0xA640, 0xA69F, 0xE000, 0xE226, 0 };

    main_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, &font_config, ranges);
    bold_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 18.0f, &font_config, ranges);
    title_font = io.Fonts->AddFontFromMemoryTTF((void*)fontBytes, sizeof(fontBytes), 26.0f, &font_config, ranges);
    sub_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f, &font_config, ranges);
    font_config.PixelSnapH = false;
    tab_icons = io.Fonts->AddFontFromMemoryTTF(clarityfont, sizeof(clarityfont), 18.0f, &font_config, ranges);

    if (!main_font) main_font = io.Fonts->AddFontDefault();
    if (!bold_font) bold_font = io.Fonts->AddFontDefault();
    if (!title_font) title_font = io.Fonts->AddFontDefault();
    if (!sub_font) sub_font = io.Fonts->AddFontDefault();
    if (!tab_icons) tab_icons = bold_font;
    bold = bold_font;

    // Clear color with 0.0 alpha for true alpha compositing
    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    static char dest_path[MAX_PATH] = "C:\\Program Files\\BlueStacks_msi5";
    static bool b_particle_glow = true;
    static int status_particle_glow = 0;
    static bool b_particle_anim = true;
    static int status_particle_anim = 0;

    static heads tab = rage;
    static float tab_alpha = 1.0f;
    static float intro_timer = 0.0f;
    static int char_idx = 0;
    static std::string intro_text = "";
    static float anim_progress = 0.0f;

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Pending wallpaper operations
        if (wallpaper_pending_action == 1) {
            wallpaper_pending_action = 0;
            LoadWallpaperFromFile(pending_path);
        }
        else if (wallpaper_pending_action == 2) {
            wallpaper_pending_action = 0;
            for (auto& frame : bg_frames) { if (frame.tex) frame.tex->Release(); }
            bg_frames.clear();
            wallpaper_path[0] = '\0';
            WritePrivateProfileStringA("Settings", "Wallpaper", "", config_path);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (memMode == 0) { fastMem = false; }
        else if (memMode == 1) { fastMem = true; }

        ImGui::PushFont(main_font);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(window::size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent base

        ImGui::Begin("BigMouthArmy_Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

        auto pos = ImGui::GetWindowPos();
        auto size = ImGui::GetWindowSize();
        {
            auto draw = ImGui::GetWindowDrawList();
            float delta = ImGui::GetIO().DeltaTime;

            // -------------------------------------------------------------
            // 1. UNDER-GLASS PARTICLES & GLOW LINES (Refract through glass)
            // -------------------------------------------------------------
            RenderLiquidParticles(draw, pos, size, delta, b_particle_anim, b_particle_glow);

            // -------------------------------------------------------------
            // 2. BASE CANVAS FROSTED LIQUID GLASS LAYER
            // -------------------------------------------------------------
            ImU32 base_fill = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                ? IM_COL32(12, 16, 26, 50)    // Ultra-sheer translucent obsidian glass
                : IM_COL32(255, 255, 255, 20); // Ultra-sheer translucent crystal frost
            ImU32 base_border = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                ? IM_COL32(255, 255, 255, 45)
                : IM_COL32(255, 255, 255, 75);

            DrawLiquidGlass(draw, pos, pos + size, 16.0f, base_fill, base_border, 1.0f, 1.2f, 0, 0.0f, true);

            // Translucent Wallpaper (Frosted Glass Overlay)
            if (!bg_frames.empty()) {
                frame_time += delta;
                if (frame_time >= bg_frames[current_frame].delay) {
                    frame_time -= bg_frames[current_frame].delay;
                    current_frame = (current_frame + 1) % bg_frames.size();
                }
                // Subtle blurred passes under the glass
                for (float bx = -2.0f; bx <= 2.0f; bx += 2.0f) {
                    for (float by = -2.0f; by <= 2.0f; by += 2.0f) {
                        draw->AddImageRounded((void*)bg_frames[current_frame].tex, pos + ImVec2(bx, by), pos + size + ImVec2(bx, by), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 8), 16.0f);
                    }
                }
                draw->AddImageRounded((void*)bg_frames[current_frame].tex, pos, pos + size, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32), 16.0f);
                draw->AddRectFilled(pos, pos + size, IM_COL32(10, 14, 22, 50), 16.0f);
            }

            // Window drag listener
            move_window();

            // Circular Liquid Glass Close Button (Top-Right)
            ImGui::SetCursorPos(ImVec2(size.x - 34.0f, 12.0f));
            if (CustomCloseButton()) { done = true; }

            // =============================================================
            // STATE 0: INTRO TYPEWRITER & INITIALIZATION
            // =============================================================
            if (app_state == 0)
            {
                intro_timer += delta;
                if (intro_timer > 0.08f && char_idx < (int)strlen(target_title)) {
                    intro_text += target_title[char_idx];
                    char_idx++;
                    intro_timer = 0.0f;
                }
                std::string display_text = intro_text;
                if (char_idx < (int)strlen(target_title) || intro_timer < 1.0f) {
                    display_text += (fmod(ImGui::GetTime(), 0.8) < 0.4 ? "_" : "");
                }

                // Centered Floating Liquid Glass Welcome Card
                float card_w = 420.0f; float card_h = 220.0f;
                ImVec2 card_min = ImVec2(pos.x + (size.x - card_w) * 0.5f, pos.y + (size.y - card_h) * 0.5f);
                ImVec2 card_max = card_min + ImVec2(card_w, card_h);
                ImU32 card_fill = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                    ? IM_COL32(18, 24, 36, 60)
                    : IM_COL32(255, 255, 255, 25);

                DrawLiquidGlass(draw, card_min, card_max, 16.0f, card_fill, IM_COL32(255, 255, 255, 65), 1.0f, 1.4f, ACCENT_U32(255), 0.35f, true);

                ImGui::PushFont(title_font);
                ImVec2 ts = ImGui::CalcTextSize(display_text.c_str());
                ImGui::PopFont();

                float text_x = card_min.x + (card_w - ts.x) * 0.5f;
                float text_y = card_min.y + 45.0f;

                // Electronic text bloom
                for (int r = 1; r <= 4; r++) {
                    float a_factor = 1.0f - ((float)r / 5.0f);
                    draw->AddText(title_font, 22.0f, ImVec2(text_x - r, text_y), IM_COL32(255, 255, 255, (int)(120 * a_factor)), display_text.c_str());
                    draw->AddText(title_font, 22.0f, ImVec2(text_x + r, text_y), IM_COL32(255, 255, 255, (int)(120 * a_factor)), display_text.c_str());
                }
                draw->AddText(title_font, 22.0f, ImVec2(text_x, text_y), IM_COL32(255, 255, 255, 255), display_text.c_str());

                // Status Subtitle
                const char* sub_msg = "VISIONOS LIQUID GLASS ENGINE INITIALIZING";
                ImGui::PushFont(sub_font);
                ImVec2 sub_ts = ImGui::CalcTextSize(sub_msg);
                ImGui::PopFont();
                DrawGlassText(sub_font, 13.0f, ImVec2(card_min.x + (card_w - sub_ts.x) * 0.5f, card_min.y + 105.0f), IM_COL32(175, 195, 225, 230), sub_msg);

                // Luminous loading bar
                ImVec2 bar_min = ImVec2(card_min.x + 40.0f, card_min.y + 135.0f);
                ImVec2 bar_max = ImVec2(card_max.x - 40.0f, card_min.y + 140.0f);
                draw->AddRectFilled(bar_min, bar_max, IM_COL32(10, 14, 22, 180), 2.5f);
                float load_prog = ImClamp((float)char_idx / (float)strlen(target_title), 0.05f, 1.0f);
                ImVec2 load_max = ImVec2(bar_min.x + load_prog * (bar_max.x - bar_min.x), bar_max.y);
                draw->AddRectFilled(bar_min, load_max, ACCENT_U32(230), 2.5f);
                draw->AddLine(bar_min, ImVec2(load_max.x, bar_min.y), IM_COL32(255, 255, 255, 200));

                // Loading status indicator
                DrawGlassText(sub_font, 13.0f, ImVec2(card_min.x + (card_w - 120.0f) * 0.5f, card_min.y + 168.0f), IM_COL32(175, 195, 225, 180), "Authenticating...");

                if (char_idx == (int)strlen(target_title) && intro_timer > 1.2f) {
                    static bool auth_started = false;
                    if (!auth_started) {
                        auth_started = true;
                        std::thread fast([&app_state]()
                            {
                                // Check app version first
                                std::string remote_ver = gk_auth::checkUpdate(sheet_id);
                                if (!remote_ver.empty() && remote_ver != app_ver) {
                                    app_state = 1;
                                    fock = false;
                                    AddNotification("Update Required! v" + remote_ver + " available.", Notify_Error);
                                    return;
                                }
                                if (gk_auth::hwidLogin(sheet_id)) {
                                    fock = true;
                                    tabs = 0;
                                    hide_login = true;
                                    app_state = 2;
                                    AddNotification("Login Successful!", Notify_Success);
                                }
                                else {
                                    app_state = 1;
                                    fock = false;
                                    AddNotification("Please Enter License Key", Notify_Warning);
                                }
                            });
                        fast.detach();
                    }
                }
            }
            // =============================================================
            // STATE 1: FLOATING GLASS LICENSE LOGIN
            // =============================================================
            else if (app_state == 1)
            {
                static char license_key[128] = "";
                static bool verifying = false;

                float card_w = 380.0f; float card_h = 270.0f;
                ImVec2 card_min = ImVec2(pos.x + (size.x - card_w) * 0.5f, pos.y + (size.y - card_h) * 0.5f);
                ImVec2 card_max = card_min + ImVec2(card_w, card_h);
                ImU32 card_fill = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                    ? IM_COL32(18, 24, 36, 60)
                    : IM_COL32(255, 255, 255, 25);

                DrawLiquidGlass(draw, card_min, card_max, 16.0f, card_fill, IM_COL32(255, 255, 255, 65), 1.0f, 1.3f, ACCENT_U32(255), 0.3f, true);

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
                ImGui::SetCursorScreenPos(card_min + ImVec2(20, 16));
                ImGui::BeginChild("LoginChild", ImVec2(card_w - 40.0f, card_h - 32.0f), false, ImGuiWindowFlags_NoScrollbar);

                ImGui::PushFont(title_font);
                const char* login_title = " RELOGIN BITCH ";
                ImVec2 title_sz = ImGui::CalcTextSize(login_title);
                ImGui::SetCursorPosX((card_w - 40.0f - title_sz.x) * 0.5f);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", login_title);
                ImGui::PopFont();

                ImGui::Dummy(ImVec2(0, 4.0f));
                CustomInputText("License Key", license_key, sizeof(license_key));
                ImGui::Dummy(ImVec2(0, 4.0f));

                if (verifying) {
                    CustomButton("Verifying Key...");
                }
                else {
                    if (CustomButton("Verify Key")) {
                        if (strlen(license_key) > 0) {
                            verifying = true;
                            std::thread([key = std::string(license_key), &app_state]() {
                                // Check app version first
                                std::string remote_ver = gk_auth::checkUpdate(sheet_id);
                                if (!remote_ver.empty() && remote_ver != app_ver) {
                                    AddNotification("Update Required! v" + remote_ver + " available.", Notify_Error);
                                    verifying = false;
                                    return;
                                }
                                if (gk_auth::licenseLogin(sheet_id, key)) {
                                    fock = true;
                                    tabs = 0;
                                    hide_login = true;
                                    app_state = 2;
                                    AddNotification("ACCESS GRANTED & DEVICE LOCKED.", Notify_Success);
                                }
                                else {
                                    AddNotification("VALIDATION FAILURE.", Notify_Error);
                                }
                                verifying = false;
                                }).detach();
                        }
                        else {
                            AddNotification("Please enter a valid key!", Notify_Error);
                        }
                }
                }

                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }
            // =============================================================
            // STATE 2: MODERN LIQUID GLASS DASHBOARD
            // =============================================================
            else if (app_state == 2)
            {
                anim_progress += delta * 1.5f;
                if (anim_progress > 1.0f) anim_progress = 1.0f;

                // -------------------------------------------------------------
                // 3A. FLOATING NAVIGATION DOCK (Detached Frosted Pill Dock)
                // -------------------------------------------------------------
                ImVec2 dock_pos = ImVec2(pos.x + 12.0f, pos.y + 14.0f);
                ImVec2 dock_size = ImVec2(52.0f, size.y - 28.0f);
                float dock_rounding = 26.0f; // Perfect capsule pill shape

                ImU32 dock_fill = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                    ? IM_COL32(16, 22, 34, 45)
                    : IM_COL32(255, 255, 255, 20);
                ImU32 dock_border = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                    ? IM_COL32(255, 255, 255, 55)
                    : IM_COL32(255, 255, 255, 85);

                DrawLiquidGlass(draw, dock_pos, dock_pos + dock_size, dock_rounding, dock_fill, dock_border, 0.85f, 1.25f, 0, 0.0f, true);

                const char* tab_icons_letters[] = { "B", "D", "C", "E" };
                const char* tab_names[] = { "Aimbot", "Visuals", "Brutal", "Settings" };
                int num_tabs = 4;
                float tab_btn_size = 38.0f;
                float tab_spacing = 48.0f;
                float total_tabs_h = ((num_tabs - 1) * tab_spacing) + tab_btn_size;
                float dock_tabs_start_y = dock_pos.y + (dock_size.y - total_tabs_h) * 0.5f;

                for (int i = 0; i < num_tabs; i++) {
                    ImVec2 btn_min = ImVec2(dock_pos.x + (dock_size.x - tab_btn_size) * 0.5f, dock_tabs_start_y + (i * tab_spacing));
                    ImVec2 btn_max = btn_min + ImVec2(tab_btn_size, tab_btn_size);

                    ImGui::SetCursorScreenPos(btn_min);
                    ImGuiID tab_id = ImGui::GetID((std::string("tab_btn_") + std::to_string(i)).c_str());
                    bool clicked = ImGui::InvisibleButton((std::string("btn_") + std::to_string(i)).c_str(), ImVec2(tab_btn_size, tab_btn_size));
                    if (clicked && tab != (heads)i) {
                        tab = (heads)i;
                        tab_alpha = 0.0f;
                    }

                    bool is_hover = ImGui::IsItemHovered();
                    bool is_act = (tab == (heads)i);

                    float t_hover = ImGui::GetCurrentWindow()->StateStorage.GetFloat(tab_id + 1, 0.0f);
                    t_hover = ExpLerp(t_hover, is_hover ? 1.0f : 0.0f, 14.0f, delta);
                    ImGui::GetCurrentWindow()->StateStorage.SetFloat(tab_id + 1, t_hover);

                    float t_act = ImGui::GetCurrentWindow()->StateStorage.GetFloat(tab_id + 2, 0.0f);
                    t_act = ExpLerp(t_act, is_act ? 1.0f : 0.0f, 16.0f, delta);
                    ImGui::GetCurrentWindow()->StateStorage.SetFloat(tab_id + 2, t_act);

                    float expand = 1.5f * t_hover;
                    ImVec2 t_draw_min = btn_min - ImVec2(expand, expand);
                    ImVec2 t_draw_max = btn_max + ImVec2(expand, expand);
                    float t_rounding = 12.0f + (expand * 0.5f);

                    ImU32 t_base_bg = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                        ? IM_COL32(255, 255, 255, (int)(15 + t_hover * 35))
                        : IM_COL32(255, 255, 255, (int)(30 + t_hover * 45));
                    ImU32 t_border = LerpColor(
                        IM_COL32(255, 255, 255, 40),
                        ACCENT_U32(215),
                        t_act
                    );

                    DrawLiquidGlass(
                        draw, t_draw_min, t_draw_max, t_rounding,
                        t_base_bg, t_border,
                        0.6f, 1.1f + 0.3f * t_hover,
                        ACCENT_U32(255), t_act * 0.85f,
                        is_hover
                    );

                    // Luminous active vertical indicator pill on dock's left edge
                    if (t_act > 0.01f) {
                        float pill_h = 18.0f * t_act;
                        ImVec2 ind_min = ImVec2(dock_pos.x + 3.0f, btn_min.y + (tab_btn_size - pill_h) * 0.5f);
                        ImVec2 ind_max = ImVec2(dock_pos.x + 6.5f, ind_min.y + pill_h);
                        draw->AddRectFilled(ind_min - ImVec2(1.5f, 1.5f), ind_max + ImVec2(1.5f, 1.5f), ACCENT_U32((int)(60 * t_act)), 2.0f);
                        draw->AddRectFilled(ind_min, ind_max, ACCENT_U32((int)(255 * t_act)), 2.0f);
                    }

                    // Icon with smooth illumination
                    ImGui::PushFont(tab_icons);
                    ImVec2 icon_sz = ImGui::CalcTextSize(tab_icons_letters[i]);
                    ImGui::PopFont();

                    ImU32 icon_col = LerpColor(
                        IM_COL32(180, 195, 220, 200),
                        IM_COL32(255, 255, 255, 255),
                        ImMax(t_hover, t_act)
                    );

                    ImVec2 icon_pos = btn_min + ImVec2((tab_btn_size - icon_sz.x) * 0.5f, (tab_btn_size - icon_sz.y) * 0.5f);
                    DrawGlassText(tab_icons, 18.0f, icon_pos, icon_col, tab_icons_letters[i]);

                    // Tooltip on dock hover
                    if (is_hover) {
                        ImGui::BeginTooltip();
                        ImGui::PushFont(bold_font);
                        ImGui::Text("%s", tab_names[i]);
                        ImGui::PopFont();
                        ImGui::EndTooltip();
                    }
                }

                // -------------------------------------------------------------
                // 3B. FLOATING TOP TITLEBAR PILL
                // -------------------------------------------------------------
                ImVec2 top_bar_min = ImVec2(pos.x + 76.0f, pos.y + 12.0f);
                ImVec2 top_bar_max = ImVec2(pos.x + size.x - 48.0f, pos.y + 44.0f);
                float top_bar_round = 12.0f;

                ImU32 top_bar_fill = (g_glass_theme == GLASS_THEME_OBSIDIAN)
                    ? IM_COL32(16, 22, 34, 38)
                    : IM_COL32(255, 255, 255, 18);

                DrawLiquidGlass(draw, top_bar_min, top_bar_max, top_bar_round, top_bar_fill, IM_COL32(255, 255, 255, 50), 0.6f, 1.15f, 0, 0.0f, false);

                // Electronic Flickering Title in Top Bar
                static float flicker_master_timer = 0.0f;
                static bool is_flash_active = false;
                static int flash_idx1 = -1, flash_idx2 = -1;
                flicker_master_timer += delta;
                int t_len = (int)strlen(target_title);

                if (flicker_master_timer > 3.2f && !is_flash_active) {
                    int num_to_flash = (std::rand() % 2) + 1;
                    flash_idx1 = std::rand() % t_len;
                    while (target_title[flash_idx1] == ' ') flash_idx1 = std::rand() % t_len;
                    if (num_to_flash == 2) {
                        flash_idx2 = std::rand() % t_len;
                        while (target_title[flash_idx2] == ' ' || flash_idx2 == flash_idx1) flash_idx2 = std::rand() % t_len;
                    }
                    else flash_idx2 = -1;
                    is_flash_active = true;
                    flicker_master_timer = 0.0f;
                }
                if (is_flash_active && flicker_master_timer > 0.25f) {
                    is_flash_active = false;
                    flash_idx1 = -1;
                    flash_idx2 = -1;
                    flicker_master_timer = 0.0f;
                }

                float title_start_x = top_bar_min.x + 16.0f;
                float title_start_y = top_bar_min.y + 6.0f;

                ImGui::PushFont(title_font);
                for (int i = 0; i < t_len; i++) {
                    char char_buf[2] = { target_title[i], '\0' };
                    ImVec2 char_offset = title_font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, target_title, target_title + i);
                    ImU32 c_color = IM_COL32(210, 225, 245, 255);
                    bool is_flashing = (i == flash_idx1 || i == flash_idx2);
                    bool is_currently_flashing = false;

                    if (is_flashing) {
                        int ms = (int)(flicker_master_timer * 1000.0f);
                        if ((ms > 0 && ms < 40) || (ms > 80 && ms < 110) || (ms > 150 && ms < 250)) is_currently_flashing = true;
                    }

                    if (is_currently_flashing) {
                        c_color = IM_COL32(255, 255, 255, 255);
                        for (int r = 1; r <= 4; r++) {
                            float af = 1.0f - ((float)r / 5.0f);
                            draw->AddText(title_font, 18.0f, ImVec2(title_start_x + char_offset.x - r, title_start_y), ACCENT_U32((int)(180 * af)), char_buf);
                            draw->AddText(title_font, 18.0f, ImVec2(title_start_x + char_offset.x + r, title_start_y), ACCENT_U32((int)(180 * af)), char_buf);
                        }
                    }
                    draw->AddText(title_font, 18.0f, ImVec2(title_start_x + char_offset.x, title_start_y), c_color, char_buf);
                }
                ImGui::PopFont();

                // VisionOS Subtitle Tag
                const char* badge_tag = "LIQUID GLASS CORE • V2.2";
                ImGui::PushFont(sub_font);
                ImVec2 badge_sz = ImGui::CalcTextSize(badge_tag);
                ImGui::PopFont();
                ImVec2 badge_pos = ImVec2(top_bar_max.x - badge_sz.x - 14.0f, top_bar_min.y + 8.0f);
                draw->AddRectFilled(badge_pos - ImVec2(6, 2), badge_pos + badge_sz + ImVec2(6, 2), IM_COL32(255, 255, 255, 20), 4.0f);
                draw->AddRect(badge_pos - ImVec2(6, 2), badge_pos + badge_sz + ImVec2(6, 2), ACCENT_U32(110), 4.0f);
                DrawGlassText(sub_font, 12.0f, badge_pos, ACCENT_U32(240), badge_tag);

                // -------------------------------------------------------------
                // 3C. CONTENT CONTAINER CHILD AREA
                // -------------------------------------------------------------
                tab_alpha += delta * 7.0f;
                if (tab_alpha > 1.0f) tab_alpha = 1.0f;

                ImGui::SetCursorPos(ImVec2(76, 52 + (8.0f * (1.0f - tab_alpha))));
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

                ImGui::BeginChild("FeaturesChild", ImVec2(size.x - 76 - 14, size.y - 62), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
                {
                    // Helper lambda to draw polished section headers
                    auto DrawSectionHeader = [&](const char* title_text) {
                        ImGui::Dummy(ImVec2(0, 4.0f));
                        ImVec2 cur = ImGui::GetCursorScreenPos();
                        ImDrawList* cd = ImGui::GetWindowDrawList();
                        // Luminous accent jewel pill before title
                        cd->AddRectFilled(cur + ImVec2(0, 3), cur + ImVec2(4, 17), ACCENT_U32(255), 2.0f);
                        cd->AddRectFilled(cur + ImVec2(-1, 2), cur + ImVec2(5, 18), ACCENT_U32(60), 2.0f);
                        ImGui::SetCursorScreenPos(cur + ImVec2(12, 0));
                        ImGui::PushFont(bold_font);
                        ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1.0f), "%s", title_text);
                        ImGui::PopFont();
                        ImGui::Dummy(ImVec2(0, 2.0f));
                    };

                    // --- TAB 0: RAGE (AIMBOT) ---
                    if (tab == rage) {
                        DrawSectionHeader("External Aimbot System");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomSelectable("Drag", selection == 0)) selection = 0;
                        ImGui::NextColumn();
                        if (CustomSelectable("Drag Fix", selection == 1)) selection = 1;
                        ImGui::NextColumn();
                        if (CustomSelectable("Head", selection == 2)) selection = 2;
                        ImGui::NextColumn();
                        if (CustomSelectable("AI Head", selection == 3)) selection = 3;
                        ImGui::Columns(1);

                        ImGui::Dummy(ImVec2(0, 4.0f));
                        CustomKeybind("External Key", "Assign Key For Auto Aim Trigger", &dragKey);
                        ImGui::Dummy(ImVec2(0, 2.0f));
                        if (CustomButton("Enable External Engine")) {
                            std::thread fast([]() { loadAll(); });
                            fast.detach();
                        }
                    }
                    // --- TAB 1: VISUALS ---
                    else if (tab == visuals) {
                        DrawSectionHeader("Visual Features Matrix");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Chams Menu", "Inject Chams Menu DLL", &var::chams_menu, &stats::chams_menu)) {
                            if (var::chams_menu) {
                                std::thread fast([]() {
                                    if (Attach()) {
                                        std::string url = "https://gitlab.com/banzoro00/esp-free/-/raw/main/System32.dll";
                                        std::string localFilePath = "C:\\Windows\\System32\\menu.dll";
                                        URLDownloadToFileA(NULL, url.c_str(), localFilePath.c_str(), 0, NULL);
                                        LPCSTR DllPath = "C:\\Windows\\System32\\menu.dll";
                                        HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
                                        LPVOID pDllPath = VirtualAllocEx(handle, 0, strlen(DllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
                                        WriteProcessMemory(handle, pDllPath, (LPVOID)DllPath, strlen(DllPath) + 1, 0);
                                        HANDLE hLoadThread = CreateRemoteThread(handle, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), pDllPath, 0, 0);
                                        WaitForSingleObject(hLoadThread, INFINITE);
                                        VirtualFreeEx(handle, pDllPath, strlen(DllPath) + 1, MEM_RELEASE);
                                        stats::chams_menu = 0;
                                        AddNotification("Chams Menu Injected!", Notify_Success);
                                    }
                                    else {
                                        stats::chams_menu = 2;
                                        AddNotification("Failed to Inject Chams!", Notify_Error);
                                    }
                                });
                                fast.detach();
                            }
                        }
                        ImGui::NextColumn();
                        if (CustomToggleButton("Streamer Esp", "Inject Streamer ESP Module", &var::streamer_esp, &stats::streamer_esp)) {
                            if (var::streamer_esp) {
                                std::thread fast([]() {
                                    if (Attach()) {
                                        std::string url = "https://github.com/fkhan3273/desktop-tutorial/blob/bf726d80361f2a47ec496e9bac5c5c5e699d9122/data.dll";
                                        std::string localFilePath = "C:\\Windows\\System32\\streamerEsp.dll";
                                        URLDownloadToFileA(NULL, url.c_str(), localFilePath.c_str(), 0, NULL);
                                        LPCSTR DllPath = "C:\\Windows\\System32\\streamerEsp.dll";
                                        HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
                                        LPVOID pDllPath = VirtualAllocEx(handle, 0, strlen(DllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
                                        WriteProcessMemory(handle, pDllPath, (LPVOID)DllPath, strlen(DllPath) + 1, 0);
                                        HANDLE hLoadThread = CreateRemoteThread(handle, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), pDllPath, 0, 0);
                                        WaitForSingleObject(hLoadThread, INFINITE);
                                        VirtualFreeEx(handle, pDllPath, strlen(DllPath) + 1, MEM_RELEASE);
                                        stats::streamer_esp = 0;
                                        AddNotification("Streamer Esp Injected!", Notify_Success);
                                    }
                                    else {
                                        stats::streamer_esp = 2;
                                        AddNotification("Failed to Inject Streamer ESP!", Notify_Error);
                                    }
                                });
                                fast.detach();
                            }
                        }
                        ImGui::Columns(1);

                        ImGui::Dummy(ImVec2(0, 4.0f));
                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Streamer Proof", "Hide window from screen capture", &var::streamer_proof, &stats::streamer_proof)) {
                            if (var::streamer_proof) {
                                std::thread fast([]() {
                                    if (SetWindowDisplayAffinity(g_hwnd, WDA_EXCLUDEFROMCAPTURE)) {
                                        SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, GetWindowLongPtr(g_hwnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
                                        SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                                        stats::streamer_proof = 1; AddNotification("Streamer Proof: ON", Notify_Success);
                                    }
                                    else { stats::streamer_proof = 0; AddNotification("Capture exclusion failed", Notify_Error); }
                                });
                                fast.detach();
                            } else {
                                std::thread fast([]() {
                                    SetWindowDisplayAffinity(g_hwnd, WDA_NONE);
                                    SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, GetWindowLongPtr(g_hwnd, GWL_EXSTYLE) & ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW));
                                    SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                                    stats::streamer_proof = 0; AddNotification("Streamer Proof: OFF", Notify_Success);
                                });
                                fast.detach();
                            }
                        }
                        ImGui::NextColumn();
                        ImGui::Columns(1);

                        ImGui::Dummy(ImVec2(0, 4.0f));
                        CustomInputText("Destination Path", dest_path, sizeof(dest_path));
                        ImGui::Dummy(ImVec2(0, 2.0f));
                        if (CustomButton("Inject Opengl32.dll")) {
                            std::thread([path = std::string(dest_path)]() {
                                Attach();
                                std::string dllPath = path + "\\opengl32.dll";
                                URLDownloadToFileA(NULL, "https://drive.google.com/uc?id=1HWJ4a_4L8xPwdziFgcVMtnsHzdv2qIAC&export=download", dllPath.c_str(), 0, NULL);
                                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
                                LPVOID pMem = VirtualAllocEx(hProcess, 0, dllPath.length() + 1, MEM_COMMIT, PAGE_READWRITE);
                                WriteProcessMemory(hProcess, pMem, dllPath.c_str(), dllPath.length() + 1, 0);
                                HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), pMem, 0, 0);
                                WaitForSingleObject(hThread, INFINITE);
                                CloseHandle(hThread);
                                VirtualFreeEx(hProcess, pMem, 0, MEM_RELEASE);
                                CloseHandle(hProcess);
                                AddNotification("Opengl32.dll Injected!", Notify_Success);
                            }).detach();
                        }
                    }
                    // --- TAB 2: BRUTAL ---
                    else if (tab == brutal) {
                        DrawSectionHeader("Sniper Precision Matrix");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Sniper Scope", "Activate Sniper Scope Overlay", &var::sniper, &stats::sniper)) {
                            if (var::sniper) { std::thread fast([]() { sniperScope(stats::sniper); }); fast.detach(); }
                        }
                        ImGui::NextColumn();
                        CustomKeybind("Sniper Scope Key", "Trigger Keybind", &sniperKey);
                        ImGui::Columns(1);

                        CustomSliderInt("Sniper Scope Fov", &sliderSniper, 0, 100, "%d%%");

                        if (CustomToggleButton("Quick Switch", "Instant Weapon Switch Action", &var::quick, &stats::quick)) {
                            if (var::quick) { std::thread fast([]() { quickSw(stats::quick); }); fast.detach(); }
                        }

                        DrawSectionHeader("Brutal Tactical Modifiers");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Female Changer", "Activate in Loading", &var::female, &stats::female)) {
                            if (var::female) { std::thread fast([]() { femaleChanger(stats::female); }); fast.detach(); }
                        }
                        ImGui::NextColumn();
                        if (CustomToggleButton("Guest Reset", "Activate in Loading", &var::guest, &stats::guest)) {
                            if (var::guest) { std::thread fast([]() { guest(stats::guest); }); fast.detach(); }
                        }
                        ImGui::Columns(1);

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Speed [Risk]", "Activate in Game", &var::speed, &stats::speed)) {
                            if (var::speed) { std::thread fast([]() { speed(stats::speed); }); fast.detach(); }
                        }
                        ImGui::NextColumn();
                        CustomKeybind("Speed Key", "Hold Key for Speed Boost", &speedKey);
                        ImGui::Columns(1);

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Camera Hack", "Activate in Game", &var::camera, &stats::camera)) {
                            if (var::camera) { std::thread fast([]() { camera(stats::camera); }); fast.detach(); }
                        }
                        ImGui::NextColumn();
                        CustomKeybind("Camera Key", "Toggle Camera Angle Key", &cameraKey);
                        ImGui::Columns(1);

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Fast Landing", "Activate in Game", &var::landing, &stats::landing)) {
                            if (var::landing) { std::thread fast([]() { fastLanding(stats::landing); }); fast.detach(); }
                        }
                        ImGui::NextColumn();
                        CustomKeybind("Fast Landing Key", "Trigger Fast Fall Key", &landingKey);
                        ImGui::Columns(1);

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Bypass Loading", "Activate in Loading Screen", &var::bLoading, &stats::bLoading)) {
                            if (var::bLoading) { std::thread fast([]() { bypassLoading(stats::bLoading); }); fast.detach(); }
                        }
                        ImGui::NextColumn();
                        if (CustomToggleButton("Bypass Lobby", "Activate in Matchmaking Lobby", &var::bLobby, &stats::bLobby)) {
                            if (var::bLobby) { std::thread fast([]() { bypassLobby(stats::bLobby); }); fast.detach(); }
                        }
                        ImGui::Columns(1);
                    }
                    // --- TAB 3: SETTINGS ---
                    else if (tab == settings) {
                        DrawSectionHeader("Liquid Glass Materials & Shading");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomSelectable("Obsidian Glass (VisionOS)", g_glass_theme == GLASS_THEME_OBSIDIAN)) {
                            g_glass_theme = GLASS_THEME_OBSIDIAN;
                            EnableBlurBehind(g_hwnd);
                        }
                        ImGui::NextColumn();
                        if (CustomSelectable("Crystal Frost (Acrylic)", g_glass_theme == GLASS_THEME_CRYSTAL)) {
                            g_glass_theme = GLASS_THEME_CRYSTAL;
                            EnableBlurBehind(g_hwnd);
                        }
                        ImGui::Columns(1);
                        ImGui::Dummy(ImVec2(0, 4.0f));

                        CustomColorEdit("Accent Neon Tint", "Liquid Glow & Grabber Backlight", &global_accent);
                        ImGui::Dummy(ImVec2(0, 4.0f));

                        if (CustomSliderFloat("Glass Transparency", &g_glass_opacity_scale, 0.10f, 1.0f, "%.2f")) {
                            EnableBlurBehind(g_hwnd);
                        }
                        ImGui::Dummy(ImVec2(0, 4.0f));

                        DrawSectionHeader("Refractive Optics & Particles");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Under-Glass Photons", "Animated particles underneath glass", &b_particle_anim, &status_particle_anim)) {
                            status_particle_anim = 0;
                        }
                        ImGui::NextColumn();
                        if (CustomToggleButton("Refraction Beams", "Luminous connecting glow lines", &b_particle_glow, &status_particle_glow)) {
                            status_particle_glow = 0;
                        }
                        ImGui::Columns(1);

                        ImGui::Columns(2, nullptr, false);
                        if (CustomToggleButton("Prism Caustics", "Chromatic edge dispersion sheen", &b_prism_caustics, &status_prism_caustics)) {
                            status_prism_caustics = 0;
                        }
                        ImGui::NextColumn();
                        ImGui::Columns(1);

                        DrawSectionHeader("Memory Mode Architecture");

                        ImGui::Columns(2, nullptr, false);
                        if (CustomSelectable("Normal Memory", memMode == 0)) memMode = 0;
                        ImGui::NextColumn();
                        if (CustomSelectable("Fast Memory Engine", memMode == 1)) memMode = 1;
                        ImGui::Columns(1);
                        ImGui::Dummy(ImVec2(0, 4.0f));

                        DrawSectionHeader("Wallpaper Frosted Overlay");
                        CustomInputText("Background Path (PNG/JPG/GIF)", wallpaper_path, sizeof(wallpaper_path));

                        ImGui::Columns(2, nullptr, false);
                        if (CustomButton("Apply Wallpaper Texture")) {
                            strcpy_s(pending_path, sizeof(pending_path), wallpaper_path);
                            WritePrivateProfileStringA("Settings", "Wallpaper", wallpaper_path, config_path);
                            wallpaper_pending_action = 1;
                            AddNotification("Wallpaper Applied", Notify_Success);
                        }
                        ImGui::NextColumn();
                        if (CustomButton("Reset To Liquid Glass")) {
                            wallpaper_pending_action = 2;
                            AddNotification("Reset to Default Glass", Notify_Warning);
                        }
                        ImGui::Columns(1);
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(3);
            }
        }
        ImGui::End();

        if (app_state >= 1) RenderNotifications(pos, ImGui::GetIO().DeltaTime);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::PopFont();

        ImGui::Render();

        // True alpha compositing clear (0.0 alpha)
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    for (auto& frame : bg_frames) { if (frame.tex) { frame.tex->Release(); frame.tex = nullptr; } }
    bg_frames.clear();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY: ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
