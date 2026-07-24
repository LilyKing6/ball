#!/usr/bin/env python3
"""
BallBattle 外部 Agent 测试客户端
验证 WebSocket 通信 + JSON 协议

用法：
    pip install websocket-client
    python agent_test_client.py

工作流：
    1. 连接 ws://127.0.0.1:8765
    2. 接收 snapshot，打印本地玩家信息和游戏状态
    3. 每帧发送随机 cursor 位置（向画面中央移动）
    4. Ctrl+C 退出
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


WS_URL = "ws://127.0.0.1:8765"
running = True


def on_message(ws, message):
    try:
        data = json.loads(message)
    except json.JSONDecodeError:
        print(f"[recv] non-JSON: {message[:100]}")
        return

    msg_type = data.get("type", "unknown")

    if msg_type == "snapshot":
        snap = data.get("snapshot", {})
        tick = snap.get("tickId", 0)
        players = snap.get("players", [])
        local = next((p for p in players if p.get("isLocal")), None)

        if tick % 30 == 0:  # 每 30 帧（3秒）打印一次
            print(f"\n[tick={tick}] gameTime={snap.get('gameTime', 0):.1f}s "
                  f"mode={snap.get('gameMode')}")

            if local:
                cells = local.get("cells", [])
                total_mass = sum(c.get("mass", 0) for c in cells)
                print(f"  Local: id={local['id']} name='{local['name']}' "
                      f"team={local['team']} mass={total_mass:.1f} "
                      f"cells={len(cells)} shield={local.get('shield', 0)}")
                if cells:
                    c = cells[0]
                    print(f"  Cell[0]: pos=({c['x']:.0f},{c['y']:.0f}) mass={c['mass']:.1f}")
            else:
                print(f"  (no local player, total players: {len(players)})")

            if snap.get("safeZoneRadius", 0) > 0:
                print(f"  SafeZone: r={snap['safeZoneRadius']:.0f} "
                      f"center=({snap['safeZoneCenterX']:.0f},{snap['safeZoneCenterY']:.0f}) "
                      f"shrink_phase={snap.get('shrinkPhase')} "
                      f"next={snap.get('timeToNextShrink', -1):.1f}s")

            print(f"  Foods={len(snap.get('foods', []))} "
                  f"Viruses={len(snap.get('viruses', []))}")

    elif msg_type == "welcome":
        print(f"[welcome] {data}")
    elif msg_type == "error":
        print(f"[error] {data}")
    else:
        print(f"[recv] {msg_type}: {str(data)[:200]}")


def on_error(ws, error):
    print(f"[error] {error}")


def on_close(ws, close_status, msg):
    print(f"[closed] {close_status} {msg}")
    global running
    running = False


def on_open(ws):
    print(f"[connected] {WS_URL}")

    # 发送输入线程：每 100ms 发送一次 cursor 位置
    def input_loop():
        while running:
            # 向地图中央附近发送游标位置（带随机抖动）
            cursor_x = 1500 + random.uniform(-800, 800)
            cursor_y = 1500 + random.uniform(-800, 800)

            msg = {
                "type": "input",
                "playerId": 1,  # 本地玩家 ID
                "input": {
                    "cursor": {"x": cursor_x, "y": cursor_y},
                    "wantSplit": False,
                    "wantEject": False
                }
            }
            try:
                ws.send(json.dumps(msg))
            except Exception as e:
                print(f"[send error] {e}")
                break
            time.sleep(0.1)

    threading.Thread(target=input_loop, daemon=True).start()


def main():
    print(f"BallBattle Agent Test Client")
    print(f"Connecting to {WS_URL} ...")
    print("(make sure the game is running with enableAgentServer=true in config.json)")
    print("Press Ctrl+C to quit\n")

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
        print("\n[exit] shutting down...")
        global running
        running = False
        ws.close()


if __name__ == "__main__":
    main()
