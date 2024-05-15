# import aiohttp
# import asyncio


# async def main():
#     while True:
#         async with aiohttp.ClientSession() as session:
#             response = await session.get("http://127.0.0.1:8081")
#             content = await response.text()
#             print(content)


# if __name__ == "__main__":
#     asyncio.run(main())
from locust import HttpUser, task

class LoadTest(HttpUser):
    @task
    def index(self):
        self.client.get("/")