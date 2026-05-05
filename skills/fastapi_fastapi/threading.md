FastAPI is primarily async but supports synchronous code through thread pools.

**1. Thread safety with shared state:**
```python
from fastapi import FastAPI
import threading

app = FastAPI()
counter = 0
counter_lock = threading.Lock()

@app.get("/increment/")
async def increment():
    global counter
    with counter_lock:
        counter += 1
    return {"counter": counter}

# For async-safe counters
import asyncio

async_counter = 0
async_counter_lock = asyncio.Lock()

@app.get("/async-increment/")
async def async_increment():
    global async_counter
    async with async_counter_lock:
        async_counter += 1
    return {"counter": async_counter}
```

**2. Running blocking code in thread pool:**
```python
from fastapi import FastAPI
import asyncio
import time

app = FastAPI()

def blocking_operation():
    time.sleep(5)
    return "Done"

@app.get("/blocking/")
async def handle_blocking():
    # Run blocking code in thread pool
    loop = asyncio.get_event_loop()
    result = await loop.run_in_executor(None, blocking_operation)
    return {"result": result}
```

**3. GIL considerations with CPU-bound tasks:**
```python
from fastapi import FastAPI
import asyncio
from concurrent.futures import ProcessPoolExecutor

app = FastAPI()
process_pool = ProcessPoolExecutor(max_workers=4)

def cpu_intensive_task(data: list):
    # CPU-bound work that benefits from multiprocessing
    result = sum(x * x for x in data)
    return result

@app.post("/compute/")
async def compute(data: list):
    loop = asyncio.get_event_loop()
    # Use ProcessPoolExecutor to bypass GIL
    result = await loop.run_in_executor(process_pool, cpu_intensive_task, data)
    return {"result": result}
```

**4. Thread-safe database connections:**
```python
from fastapi import FastAPI
from databases import Database
import threading

app = FastAPI()
database = Database("postgresql://user:pass@localhost/db")
local_storage = threading.local()

@app.on_event("startup")
async def startup():
    await database.connect()

@app.get("/items/")
async def get_items():
    # Each request gets its own connection from pool
    async with database.transaction():
        items = await database.fetch_all("SELECT * FROM items")
    return items

# Thread-local storage for request-specific data
@app.middleware("http")
async def add_request_id(request, call_next):
    request_id = str(uuid.uuid4())
    local_storage.request_id = request_id
    response = await call_next(request)
    return response
```

**5. Async context managers for thread safety:**
```python
from fastapi import FastAPI
import asyncio

app = FastAPI()

class AsyncResource:
    def __init__(self):
        self._lock = asyncio.Lock()
        self._data = {}
    
    async def update(self, key: str, value: str):
        async with self._lock:
            self._data[key] = value
            return self._data

resource = AsyncResource()

@app.post("/update/{key}/{value}")
async def update_resource(key: str, value: str):
    result = await resource.update(key, value)
    return result
```
