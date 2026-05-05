Common mistakes developers make when using FastAPI.

**1. Forgetting to use async for database operations:**
```python
# BAD: Blocking database call in async endpoint
@app.get("/users/{user_id}")
async def get_user(user_id: int):
    user = database.query(User).filter(User.id == user_id).first()  # Blocks event loop
    return user

# GOOD: Use async database driver
from databases import Database

database = Database("postgresql://user:pass@localhost/db")

@app.get("/users/{user_id}")
async def get_user(user_id: int):
    query = "SELECT * FROM users WHERE id = :id"
    user = await database.fetch_one(query, values={"id": user_id})
    return user
```

**2. Not using Pydantic models for request validation:**
```python
# BAD: Manual validation
@app.post("/items/")
async def create_item(name: str, price: float):
    if not name:
        raise HTTPException(status_code=400, detail="Name required")
    return {"name": name, "price": price}

# GOOD: Use Pydantic for automatic validation
from pydantic import BaseModel, Field

class Item(BaseModel):
    name: str = Field(..., min_length=1)
    price: float = Field(..., gt=0)

@app.post("/items/")
async def create_item(item: Item):
    return item
```

**3. Exposing sensitive information in error responses:**
```python
# BAD: Leaking stack traces
@app.get("/users/{user_id}")
async def get_user(user_id: int):
    result = await database.fetch_one("SELECT * FROM users WHERE id = $1", user_id)
    return result  # Might expose internal data

# GOOD: Use response models
from pydantic import BaseModel

class UserOut(BaseModel):
    id: int
    username: str
    email: str

@app.get("/users/{user_id}", response_model=UserOut)
async def get_user(user_id: int):
    result = await database.fetch_one("SELECT * FROM users WHERE id = $1", user_id)
    return UserOut(**result)
```

**4. Ignoring CORS configuration:**
```python
# BAD: No CORS middleware
app = FastAPI()

@app.get("/items/")
async def read_items():
    return [{"name": "Foo"}]

# GOOD: Configure CORS properly
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["https://myfrontend.com"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
```

**5. Not using dependency injection for authentication:**
```python
# BAD: Duplicate auth logic
@app.get("/items/")
async def read_items(token: str = Header(None)):
    if token != "secret":
        raise HTTPException(status_code=401)
    return [{"name": "Foo"}]

@app.post("/items/")
async def create_item(item: Item, token: str = Header(None)):
    if token != "secret":
        raise HTTPException(status_code=401)
    return item

# GOOD: Use dependency injection
from fastapi import Depends, HTTPException, Header

def verify_token(token: str = Header(None)):
    if token != "secret":
        raise HTTPException(status_code=401)
    return token

@app.get("/items/")
async def read_items(token: str = Depends(verify_token)):
    return [{"name": "Foo"}]

@app.post("/items/")
async def create_item(item: Item, token: str = Depends(verify_token)):
    return item
```

**6. Overlooking request size limits:**
```python
# BAD: No size limit on file uploads
@app.post("/upload/")
async def upload_file(file: UploadFile = File(...)):
    contents = await file.read()
    return {"size": len(contents)}

# GOOD: Implement size validation
from fastapi import HTTPException

MAX_SIZE = 10 * 1024 * 1024  # 10MB

@app.post("/upload/")
async def upload_file(file: UploadFile = File(...)):
    contents = await file.read()
    if len(contents) > MAX_SIZE:
        raise HTTPException(status_code=413, detail="File too large")
    return {"size": len(contents)}
```
