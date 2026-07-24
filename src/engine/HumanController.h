#pragma once

#include "IController.h"
#include "entity/Player.h"

// 由人类玩家直接控制的 Controller（鼠标键盘）
// 输入通过 setMousePos/requestSplit/requestEject 由 GLWidget 填充
class HumanController : public IController {
public:
    HumanController() = default;

    PlayerInput sample(const Player& self, const World& world, float dt) override {
        PlayerInput in;
        in.mouseWorldPos = m_pendingCursor;
        in.wantSplit = m_pendingSplit;
        in.wantEject = m_pendingEject;
        // 消费一次性动作
        m_pendingSplit = false;
        m_pendingEject = false;
        return in;
    }

    QString type() const override { return "human"; }

    // GLWidget 调用以下方法填充输入
    void setCursor(Vec2 worldPos) { m_pendingCursor = worldPos; }
    void requestSplit() { m_pendingSplit = true; }
    void requestEject() { m_pendingEject = true; }

private:
    Vec2 m_pendingCursor;
    bool m_pendingSplit = false;
    bool m_pendingEject = false;
};
