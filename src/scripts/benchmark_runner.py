import asyncio
import websockets
import json
import time
import statistics
import argparse
from concurrent.futures import ThreadPoolExecutor

class BenchmarkRunner:
    def __init__(self, uri="ws://localhost:9001"):
        self.uri = uri
        self.results = []
        
    async def run_order_benchmark(self, num_orders=1000, symbol="BTC-USDT"):
        print(f"Running benchmark with {num_orders} orders...")
        
        async with websockets.connect(self.uri) as websocket:
            start_time = time.time()
            
            for i in range(num_orders):
                order = {
                    "type": "order",
                    "symbol": symbol,
                    "order_type": "limit",
                    "side": "buy" if i % 2 == 0 else "sell",
                    "quantity": 1.0,
                    "price": 50000.0 + (i % 100),
                    "order_id": f"bench_{i}"
                }
                await websocket.send(json.dumps(order))
                
            end_time = time.time()
            
            elapsed = end_time - start_time
            throughput = num_orders / elapsed
            
            print(f"Completed {num_orders} orders in {elapsed:.2f}s")
            print(f"Throughput: {throughput:.2f} orders/sec")
            
            self.results.append({
                'orders': num_orders,
                'elapsed': elapsed,
                'throughput': throughput
            })

    async def run_latency_test(self, num_requests=100):
        print(f"Running latency test with {num_requests} requests...")
        
        latencies = []
        
        async with websockets.connect(self.uri) as websocket:
            for i in range(num_requests):
                start = time.time()
                
                order = {
                    "type": "order",
                    "symbol": "BTC-USDT",
                    "order_type": "limit",
                    "side": "buy",
                    "quantity": 1.0,
                    "price": 50000.0,
                    "order_id": f"latency_{i}"
                }
                await websocket.send(json.dumps(order))
                
                response = await websocket.recv()
                end = time.time()
                
                latency = (end - start) * 1000
                latencies.append(latency)
                
            avg_latency = statistics.mean(latencies)
            min_latency = min(latencies)
            max_latency = max(latencies)
            p95_latency = statistics.quantiles(latencies, n=20)[18]
            
            print(f"Latency Results (ms):")
            print(f"  Average: {avg_latency:.2f}")
            print(f"  Min: {min_latency:.2f}")
            print(f"  Max: {max_latency:.2f}")
            print(f"  P95: {p95_latency:.2f}")

async def main():
    parser = argparse.ArgumentParser(description='Run matching engine benchmarks')
    parser.add_argument('--orders', type=int, default=1000, help='Number of orders to send')
    parser.add_argument('--latency-requests', type=int, default=100, help='Number of latency requests')
    parser.add_argument('--uri', default='ws://localhost:9001', help='WebSocket URI')
    
    args = parser.parse_args()
    
    runner = BenchmarkRunner(args.uri)
    
    print("=== GoQuant Matching Engine Benchmark ===")
    
    await runner.run_order_benchmark(args.orders)
    await runner.run_latency_test(args.latency_requests)

if __name__ == "__main__":
    asyncio.run(main())