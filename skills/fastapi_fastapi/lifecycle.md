FastAPI applications have a clear lifecycle for resource management.

**1. Application startup and shutdown events:**
```python
from fastapi import FastAPI
from contextlib import asynccontextmanager

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Startup
    print("Starting up...")
    app.state.db = await create_database_pool()
    app.state.cache = await create_cache_connection()
    yield
    # Shutdown
    print("Shutting down...")
    await app.state.db.close()
    await app.state.cache.close()

app = FastAPI(lifespan=lifespan)
```

**2. Dependency lifecycle management:**
```python
from fastapi import FastAPI, Depends
from contextlib import asynccontextmanager

class DatabaseSession:
    async def __aenter__(self):
        self.session = await create_session()
        return self.session
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.session.close()

async def get_db():
    async with DatabaseSession() as session:
        yield session

app = FastAPI()

@app.get("/items/")
async def read_items(db=Depends(get_db)):
    return await db.fetch_all("SELECT * FROM items")
```

**3. Resource cleanup with try/finally:**
```python
from fastapi import FastAPI, HTTPException

app = FastAPI()

@app.post("/process/")
async def process_data(data: dict):
    file_handle = None
    try:
        file_handle = open("/tmp/data.txt", "w")
        file_handle.write(str(data))
        return {"status": "success"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        if file_handle:
            file_handle.close()
```

**4. Connection pooling for databases:**
```python
from fastapi import FastAPI
from databases import Database

app = FastAPI()
database = Database("postgresql://user:pass@localhost/db")

@app.on_event("startup")
async def startup():
    await database.connect()
    # Create connection pool
    app.state.pool = await database.create_pool(min_size=5, max_size=20)

@app.on_event("shutdown")
async def shutdown():
    await database.disconnect()
    app.state.pool.close()
```
