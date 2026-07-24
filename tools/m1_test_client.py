#!/usr/bin/env python3
"""
M1 测试客户端：验证 Go 服务端的协议闭环
连接 -> join -> 接收 welcome/snapshot -> 持续发送 input

依赖：pip install websocket-client
"""

import json
import time
import random
import threading
import sys

try:
    import websocket
except ImportError:
    print("请先安装：pip install websocket-client")
    sys.exit(1)

WS_URL = "ws://127.0.0.1:8765/ws"
running = True
my_player_id = 0
world_width = 3000
world_height = 3000


def on_message(ws, message):
    global my_player_id, world_width, world_height

    try:
        env = json.loads(message)
    except json.JSONDecodeError:
        print(f"[recv] non-JSON: {message[:100]}")
        return

    msg_type = env.get("type", "unknown")
    payload_raw = env.get("payload", {})

    if msg_type == "welcome":
        my_player_id = payload_raw.get("playerId", 0)
        world_width = payload_raw.get("worldWidth", 3000)
        world_height = payload_raw.get("worldHeight", 3000)
        print(f"[welcome] playerId={my_player_id} world={world_width}x{world_height} "
              f"room={payload_raw.get('roomName')}")

    elif msg_type == "snapshot":
        snap_raw = payload_raw.get("snapshot")
        tick = payload_raw.get("tickId", 0)
        # snapshot 是 RawMessage 形式（base64 不必要，gorilla 直接 utf8 JSON）
        if isinstance(snap_raw, str):
            snap = json.loads(snap_raw)
        elif isinstance(snap_raw, dict):
            snap = snap_raw
        else:
            print(f"[snapshot] unexpected snap type: {type(snap_raw)}")
            return

        if tick % 30 == 0:
            players = snap.get("players", [])
            local = next((p for p in players if p.get("id") == my_player_id), None)
            print(f"\n[tick={tick}] gameTime={snap.get('gameTime', 0):.1f}s "
                  f"players={len(players)} foods={len(snap.get('foods', []))}")
            if local:
                cells = local.get("cells", [])
                total_mass = sum(c.get("mass", 0) for c in cells)
                if cells:
                    c0 = cells[0]
                    print(f"  Local: pos=({c0['x']:.0f},{c0['y']:.0f}) mass={total_mass:.1f}")

    elif msg_type == "error":
        print(f"[error] {payload_raw}")
    else:
        print(f"[recv] {msg_type}: {str(payload_raw)[:200]}")


def on_error(ws, error):
    print(f"[ws_error] {error}")


def on_close(ws, close_status, msg):
    print(f"[closed] {close_status} {msg}")
    global running
    running = False


def on_open(ws):
    print(f"[connected] {WS_URL}")

    # 1. 发送 join
    join_msg = {
        "type": "join",
        "payload": {"name": "PyTester", "mode": "free"}
    }
    ws.send(json.dumps(join_msg))
    print("[sent] join")

    # 2. 启动输入循环：每 100ms 发送一次 cursor
    def input_loop():
        t = 0
        while running:
            # 圆周运动 + 随机偏移
            angle = t * 0.05
            cx = world_width / 2 + 700 * (1 + 0.3 * random.random())
            cy = world_height / 2 + 700 * (1 + 0.3 * random.random())
            cx += 200 * (0.5 - random.random())
            cy += 200 * (0.5 - random.random())

            msg = {
                "type": "input",
                "payload": {
                    "input": {
                        "cursor": {"x": cx, "y": cy},
                        "wantSplit": False,
                        "wantEject": False,
                        "mouseWorld": {"x": cx, "y": cy}
                    }
                }
            }
            try:
                ws.send(json.dumps(msg))
            except Exception as e:
                print(f"[send error] {e}")
                break
            time.sleep(0.1)
            t += 1

    threading.Thread(target=input_loop, daemon=True).start()


def main():
    print("BallBattle M1 Test Client")
    print(f"Connecting to {WS_URL} ...\n")

    ws = websocket.WebSocketApp(
        WS_URL,
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close,
    )

    try:
        ws.run_forever()
    except KeyboardInterrupt:
        global running
        running = False
        ws.close()


if __name__ == "__main__":
    main()
