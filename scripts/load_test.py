import asyncio
import websockets
import json
import random
import time
from concurrent.futures import ThreadPoolExecutor

class LoadTester:
    def __init__(self, uri="ws://localhost:9001", num_clients=10):
        self.uri = uri
        self.num_clients = num_clients
        self.results = []
        
    async def simulated_client(self, client_id, orders_per_client=100):
        latencies = []
        
        try:
            async with websockets.connect(self.uri) as websocket:
                for i in range(orders_per_client):
                    start_time = time.time()
                    
                    order = {
                        "type": "order",
                        "symbol": "BTC-USDT",
                        "order_type": random.choice(["limit", "market"]),
                        "side": random.choice(["buy", "sell"]),
                        "quantity": random.uniform(0.1, 10.0),
                        "price": random.uniform(49000, 51000),
                        "order_id": f"load_{client_id}_{i}"
                    }
                    
                    await websocket.send(json.dumps(order))
                    response = await websocket.recv()
                    
                    latency = (time.time() - start_time) * 1000
                    latencies.append(latency)
                    
                    await asyncio.sleep(random.uniform(0.01, 0.1))
                    
        except Exception as e:
            print(f"Client {client_id} error: {e}")
            
        return latencies

    async def run_load_test(self, duration_seconds=60, orders_per_second=100):
        print(f"Running load test for {duration_seconds}s at {orders_per_second} ops...")
        
        start_time = time.time()
        total_orders = 0
        
        while time.time() - start_time < duration_seconds:
            tasks = []
            for client_id in range(self.num_clients):
                task = self.simulated_client(client_id, orders_per_second // self.num_clients)
                tasks.append(task)
                
            results = await asyncio.gather(*tasks)
            
            all_latencies = []
            for latencies in results:
                all_latencies.extend(latencies)
                
            total_orders += len(all_latencies)
            
            if all_latencies:
                avg_latency = sum(all_latencies) / len(all_latencies)
                print(f"Processed {len(all_latencies)} orders, Avg latency: {avg_latency:.2f}ms")
                
            await asyncio.sleep(1)
            
        elapsed = time.time() - start_time
        throughput = total_orders / elapsed
        
        print(f"\nLoad Test Results:")
        print(f"Total Orders: {total_orders}")
        print(f"Total Time: {elapsed:.2f}s")
        print(f"Throughput: {throughput:.2f} orders/sec")

async def main():
    tester = LoadTester(num_clients=20)
    await tester.run_load_test(duration_seconds=30, orders_per_second=500)

if __name__ == "__main__":
    asyncio.run(main())