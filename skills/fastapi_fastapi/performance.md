FastAPI is designed for high performance, but proper usage patterns matter.

**1. Use async endpoints for I/O-bound operations:**
```python
# BAD: Blocking I/O in sync endpoint
@app.get("/users/{user_id}")
def get_user(user_id: int):
    import time
    time.sleep(2)  # Blocks the entire server
    return {"user_id": user_id}

# GOOD: Async I/O
import asyncio

@app.get("/users/{user_id}")
async def get_user(user_id: int):
    await asyncio.sleep(2)  # Non-blocking
    return {"user_id": user_id}
```

**2. Use response models for automatic serialization:**
```python
from pydantic import BaseModel
from typing import List

class ItemOut(BaseModel):
    id: int
    name: str
    price: float

# GOOD: FastAPI handles serialization efficiently
@app.get("/items/", response_model=List[ItemOut])
async def get_items():
    items = await fetch_items_from_db()
    return items  # FastAPI serializes automatically
```

**3. Profile your endpoints:**
```python
import time
from fastapi import FastAPI, Request

app = FastAPI()

@app.middleware("http")
async def add_process_time_header(request: Request, call_next):
    start_time = time.time()
    response = await call_next(request)
    process_time = time.time() - start_time
    response.headers["X-Process-Time"] = str(process_time)
    return response

# Use profiling tools
@app.get("/profile/")
async def profile_endpoint():
    import cProfile
    profiler = cProfile.Profile()
    profiler.enable()
    # Your code here
    result = await expensive_operation()
    profiler.disable()
    profiler.dump_stats("profile.stats")
    return result
```

**4. Use caching for expensive operations:**
```python
from functools import lru_cache
from fastapi import FastAPI

app = FastAPI()

@lru_cache(maxsize=128)
def get_expensive_data(param: str):
    # Simulate expensive computation
    import time
    time.sleep(2)
    return {"param": param, "data": "expensive"}

@app.get("/data/{param}")
async def get_data(param: str):
    return get_expensive_data(param)
```

**5. Optimize database queries:**
```python
# BAD: N+1 query problem
@app.get("/users/{user_id}")
async def get_user(user_id: int):
    user = await db.fetch_one("SELECT * FROM users WHERE id = $1", user_id)
    orders = []
    for order_id in user['order_ids']:
        order = await db.fetch_one("SELECT * FROM orders WHERE id = $1", order_id)
        orders.append(order)
    return {"user": user, "orders": orders}

# GOOD: Single query with JOIN
@app.get("/users/{user_id}")
async def get_user(user_id: int):
    query = """
        SELECT u.*, o.* 
        FROM users u 
        LEFT JOIN orders o ON u.id = o.user_id 
        WHERE u.id = $1
    """
    results = await db.fetch_all(query, user_id)
    return {"user": results[0], "orders": results[1:]}
```
