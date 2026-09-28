#pragma once

namespace menu
{
    void style();
    void render(bool* open);
    bool key_active(int vk, int mode);
    bool listening();
}
