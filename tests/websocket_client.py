import asyncio
import websockets
import json
import time
import uuid

class GoQuantClient:
    def __init__(self, uri="ws://localhost:9001"):
        self.uri = uri
        self.websocket = None
        
    async def connect(self):
        self.websocket = await websockets.connect(self.uri)
        print(f"Connected to {self.uri}")
        
    async def send_order(self, symbol, order_type, side, quantity, price=0):
        order_id = str(uuid.uuid4())
        message = {
            "type": "order",
            "symbol": symbol,
            "order_type": order_type,
            "side": side,
            "quantity": quantity,
            "price": price,
            "order_id": order_id
        }
        await self.websocket.send(json.dumps(message))
        print(f"Sent order: {order_type} {side} {quantity} {symbol} @ {price}")
        return order_id
        
    async def subscribe(self, symbol, subscription_type):
        message = {
            "type": "subscribe",
            "symbol": symbol,
            "type": subscription_type
        }
        await self.websocket.send(json.dumps(message))
        print(f"Subscribed to {subscription_type} for {symbol}")
        
    async def listen(self):
        try:
            async for message in self.websocket:
                data = json.loads(message)
                print(f"Received: {data}")
        except websockets.exceptions.ConnectionClosed:
            print("Connection closed")
            
    async def run_demo(self):
        await self.connect()
        
        await self.subscribe("BTC-USDT", "bbo")
        await self.subscribe("BTC-USDT", "depth")
        await self.subscribe("BTC-USDT", "trades")
        await asyncio.sleep(1)
        await self.send_order("BTC-USDT", "limit", "buy", 1.0, 50000.0)
        await asyncio.sleep(1)
        await self.send_order("BTC-USDT", "limit", "sell", 0.5, 51000.0)
        await asyncio.sleep(1)
        await self.send_order("BTC-USDT", "market", "buy", 0.2, 0.0)
        await asyncio.sleep(1)
        
        await self.listen()

async def main():
    client = GoQuantClient()
    await client.run_demo()

if __name__ == "__main__":
    asyncio.run(main())