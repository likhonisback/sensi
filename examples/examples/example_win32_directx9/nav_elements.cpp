#include "nav_elements.h"

struct tab_element {
    float element_opacity, rect_opacity, text_opacity;
};

bool elements::tab(const char* name, bool boolean)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(name);
    const ImVec2 label_size = tab_icons->CalcTextSizeA(16.0f, FLT_MAX, 0.0f, name);
    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, ImVec2(pos.x + 36, pos.y + 36));
    ImGui::ItemSize(ImVec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y + 4), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, NULL);

    static std::map<ImGuiID, tab_element> anim;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end()) {
        anim.insert({ id, { 0.0f, 0.0f, 0.0f } });
        it_anim = anim.find(id);
    }

    float delta = ImGui::GetIO().DeltaTime;
    it_anim->second.element_opacity = ImLerp(it_anim->second.element_opacity, (boolean ? 0.28f : hovered ? 0.16f : 0.0f), ImClamp(delta * 14.0f, 0.0f, 1.0f));
    it_anim->second.rect_opacity = ImLerp(it_anim->second.rect_opacity, (boolean ? 1.0f : 0.0f), ImClamp(delta * 16.0f, 0.0f, 1.0f));
    it_anim->second.text_opacity = ImLerp(it_anim->second.text_opacity, (boolean ? 1.0f : hovered ? 0.90f : 0.55f), ImClamp(delta * 14.0f, 0.0f, 1.0f));

    // Under-glass neon bloom
    if (it_anim->second.rect_opacity > 0.01f) {
        window->DrawList->AddRectFilled(rect.Min - ImVec2(2, 2), rect.Max + ImVec2(2, 2), IM_COL32(0, 215, 255, (int)(40 * it_anim->second.rect_opacity)), 12.0f);
    }

    // Translucent liquid glass plate
    window->DrawList->AddRectFilled(rect.Min, rect.Max, IM_COL32(255, 255, 255, (int)(255 * it_anim->second.element_opacity)), 10.0f);
    window->DrawList->AddRect(rect.Min, rect.Max, IM_COL32(255, 255, 255, (int)(60 + 120 * it_anim->second.rect_opacity)), 10.0f, 0, 1.0f);

    // Specular top meniscus highlight
    if (it_anim->second.element_opacity > 0.01f) {
        window->DrawList->AddLine(rect.Min + ImVec2(5, 0), rect.Max - ImVec2(5, 36), IM_COL32(255, 255, 255, (int)(150 * it_anim->second.element_opacity)), 1.0f);
    }

    // Crisp text with drop shadow
    ImVec2 icon_pos = (rect.Min + rect.Max - label_size) * 0.5f;
    window->DrawList->AddText(tab_icons, 16.0f, icon_pos + ImVec2(1.0f, 1.0f), IM_COL32(0, 0, 0, (int)(140 * it_anim->second.text_opacity)), name);
    window->DrawList->AddText(tab_icons, 16.0f, icon_pos, IM_COL32(255, 255, 255, (int)(255 * it_anim->second.text_opacity)), name);

    // Luminous vertical active pill
    if (it_anim->second.rect_opacity > 0.01f) {
        ImVec2 ind_min = ImVec2(rect.Max.x + 3, rect.Min.y + 7);
        ImVec2 ind_max = ImVec2(rect.Max.x + 6, rect.Max.y - 7);
        window->DrawList->AddRectFilled(ind_min, ind_max, IM_COL32(0, 215, 255, (int)(255 * it_anim->second.rect_opacity)), 2.0f);
    }

    return pressed;
}

struct subtab_element {
    float element_opacity, rect_opacity, text_opacity;
};

bool elements::subtab(const char* name, bool boolean)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(name);
    const ImVec2 label_size = ImGui::CalcTextSize(name);
    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, ImVec2(pos.x + 145, pos.y + 34));
    ImGui::ItemSize(ImVec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y + 3), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, NULL);

    static std::map<ImGuiID, subtab_element> anim;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end()) {
        anim.insert({ id, { 0.0f, 0.0f, 0.0f } });
        it_anim = anim.find(id);
    }

    float delta = ImGui::GetIO().DeltaTime;
    it_anim->second.element_opacity = ImLerp(it_anim->second.element_opacity, (boolean ? 0.28f : hovered ? 0.16f : 0.0f), ImClamp(delta * 14.0f, 0.0f, 1.0f));
    it_anim->second.rect_opacity = ImLerp(it_anim->second.rect_opacity, (boolean ? 1.0f : 0.0f), ImClamp(delta * 16.0f, 0.0f, 1.0f));
    it_anim->second.text_opacity = ImLerp(it_anim->second.text_opacity, (boolean ? 1.0f : hovered ? 0.90f : 0.55f), ImClamp(delta * 14.0f, 0.0f, 1.0f));

    // Multi-layer liquid glass plate
    window->DrawList->AddRectFilled(rect.Min, rect.Max, IM_COL32(255, 255, 255, (int)(255 * it_anim->second.element_opacity)), 9.0f);
    window->DrawList->AddRect(rect.Min, rect.Max, IM_COL32(255, 255, 255, (int)(55 + 130 * it_anim->second.rect_opacity)), 9.0f, 0, 1.0f);

    // Specular meniscus highlight
    if (it_anim->second.element_opacity > 0.01f) {
        window->DrawList->AddLine(rect.Min + ImVec2(8, 0), rect.Max - ImVec2(8, 34), IM_COL32(255, 255, 255, (int)(150 * it_anim->second.element_opacity)), 1.0f);
    }

    // Text with drop shadow
    ImVec2 t_pos = ImVec2(rect.Min.x + 14, (rect.Min.y + rect.Max.y) * 0.5f - label_size.y * 0.5f);
    window->DrawList->AddText(t_pos + ImVec2(1.0f, 1.0f), IM_COL32(0, 0, 0, (int)(140 * it_anim->second.text_opacity)), name);
    window->DrawList->AddText(t_pos, IM_COL32(255, 255, 255, (int)(255 * it_anim->second.text_opacity)), name);

    if (it_anim->second.rect_opacity > 0.01f) {
        window->DrawList->AddRectFilled(ImVec2(rect.Max.x + 4, rect.Min.y + 8), ImVec2(rect.Max.x + 7, rect.Max.y - 8), IM_COL32(0, 215, 255, (int)(255 * it_anim->second.rect_opacity)), 2.0f);
    }

    return pressed;
}
