#include "IController.h"
#include <QJsonObject>

QJsonObject PlayerInput::toJson() const {
    QJsonObject o;
    QJsonObject cur;
    cur["x"] = static_cast<double>(virtualCursor.x);
    cur["y"] = static_cast<double>(virtualCursor.y);
    o["virtualCursor"] = cur;
    o["wantSplit"] = wantSplit;
    o["wantEject"] = wantEject;
    return o;
}

PlayerInput PlayerInput::fromJson(const QJsonObject& o) {
    PlayerInput in;
    QJsonObject cur = o.value("virtualCursor").toObject();
    in.virtualCursor.x = static_cast<float>(cur.value("x").toDouble(0.0));
    in.virtualCursor.y = static_cast<float>(cur.value("y").toDouble(0.0));
    in.mouseWorldPos = in.virtualCursor;
    in.wantSplit = o.value("wantSplit").toBool(false);
    in.wantEject = o.value("wantEject").toBool(false);
    return in;
}
