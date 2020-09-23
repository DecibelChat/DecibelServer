#!/usr/bin/env python

from argparse import ArgumentParser, Namespace
import asyncio
from pathlib import Path
import json
import ssl
import websockets
from typing import Dict, Tuple

chatRooms: Dict[
    str, Tuple[Tuple[str, int], websockets.server.WebSocketServerProtocol]
] = {}


def prune_all():
    for room_id in chatRooms:
        prune_room(room_id)


def prune_room(room_id: str):
    remove = [
        connection_id for connection_id, ws in chatRooms[room_id].items() if ws.closed
    ]
    for connection_id in remove:
        removed_connection = chatRooms[room_id].pop(connection_id)


def prune_connection(room_id: str, connection_id: Tuple[str, int]):
    removed_connection = chatRooms[room_id].pop(connection_id)


async def wss_handler(websocket: websockets.server.WebSocketServerProtocol, path: str):
    while True:
        try:
            raw_data = await websocket.recv()
            parsed_data = json.loads(raw_data)
            room_id = parsed_data["code"]
            connection_id = websocket.remote_address

            parsed_data["user_id"] = hash(connection_id)
            updated_data = json.dumps(parsed_data)

            if room_id not in chatRooms:
                chatRooms[room_id] = {}

            if connection_id not in chatRooms[room_id]:
                chatRooms[room_id][connection_id] = websocket

            prune_room(room_id)

            messages = []
            for peer_id, peer_socket in chatRooms[room_id].items():
                if peer_id != connection_id:
                    messages.append(peer_socket.send(updated_data))

            if len(messages) > 0:
                await asyncio.wait(messages)

            print("done")
        except Exception as e:
            prune_all()
            print(e)
            break


def parse_args() -> Namespace:
    args = ArgumentParser()
    args.add_argument("-p", "--port", type=int, default=16666)
    args.add_argument("-d", "--root_dir", type=Path)

    return args.parse_args()


if __name__ == "__main__":
    args = parse_args()

    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    cert_pem = args.root_dir / "cert.pem"
    key_pem = args.root_dir / "privkey.pem"
    ssl_context.load_cert_chain(cert_pem, key_pem)

    start_server = websockets.serve(wss_handler, port=args.port, ssl=ssl_context)

    asyncio.get_event_loop().run_until_complete(start_server)
    asyncio.get_event_loop().run_forever()
