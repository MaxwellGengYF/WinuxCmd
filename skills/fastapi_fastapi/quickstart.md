```bash
pip install "fastapi[standard]"
```

FastAPI makes it incredibly fast to build APIs with automatic OpenAPI documentation. Here are 5 common patterns to get you started immediately.

**Basic GET endpoint with path parameter:**
```python
from fastapi import FastAPI

app = FastAPI()

@app.get("/items/{item_id}")
async def read_item(item_id: int):
    return {"item_id": item_id}
```

**POST endpoint with request body using Pydantic:**
```python
from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI()

class Item(BaseModel):
    name: str
    price: float
    is_offer: bool = False

@app.post("/items/")
async def create_item(item: Item):
    return {"item_name": item.name, "item_price": item.price}
```

**Query parameters with validation:**
```python
from fastapi import FastAPI, Query

app = FastAPI()

@app.get("/items/")
async def read_items(q: str = Query(None, min_length=3, max_length=50)):
    results = {"items": [{"item_id": "Foo"}, {"item_id": "Bar"}]}
    if q:
        results.update({"q": q})
    return results
```

**Dependency injection for shared logic:**
```python
from fastapi import FastAPI, Depends

app = FastAPI()

def common_parameters(q: str = None, skip: int = 0, limit: int = 100):
    return {"q": q, "skip": skip, "limit": limit}

@app.get("/items/")
async def read_items(commons: dict = Depends(common_parameters)):
    return commons
```

**File upload handling:**
```python
from fastapi import FastAPI, File, UploadFile

app = FastAPI()

@app.post("/files/")
async def create_file(file: UploadFile = File(...)):
    contents = await file.read()
    return {"filename": file.filename, "content_length": len(contents)}
```
