
import sys
import asyncio
import aiohttp
from aiohttp import web
import random
import os

choices = {
    1: "Rock",
    2: "Paper",
    3: "Scissors",
    4: "Pen",
    5: "Fire",
    6: "Water",
    7: "Lemonade"
}

def get_random_item():
    choice = random.randint(1, len(choices))
    return choices.get(choice)

async def sleep_random_interval():
    processing_time = random.uniform(0, 1)
    await asyncio.sleep(processing_time)

class RpsCounterMockServer:
    def __init__(self, port = 8080, ip = '0.0.0.0'):
        self.rps = 0
        self.ip  = ip
        self.port = port
        self.event_loop = asyncio.get_event_loop()
        self.web_app = aiohttp.web.Application()

    async def print_rps(self):
        while True:
            await asyncio.sleep(1)
            os.system('clear')
            print(f"ip={self.ip}:{self.port}\nrps: {self.rps}")
            self.rps = 0

    async def handle_get(self, request):
        self.rps += 1
        random_item = get_random_item()
        await sleep_random_interval()
        return aiohttp.web.Response(text=random_item)

    async def start_server(self):
        runner = aiohttp.web.AppRunner(self.web_app)
        self.web_app.router.add_get('/', self.handle_get)
        await runner.setup()
        site = aiohttp.web.TCPSite(runner, self.ip, self.port)
        await site.start()

    def run(self):
        self.event_loop.create_task(self.start_server())
        self.event_loop.create_task(self.print_rps())
        self.event_loop.run_forever()

def main():
    if len(sys.argv) < 2:
        print("Missed tcp-port number")
        sys.exit(1)

    port = int(sys.argv[1])
    server = RpsCounterMockServer(port=port)
    try:
        server.run()
    except KeyboardInterrupt:
        print('Terminated by SIGINT')


if __name__ == "__main__":
    main()