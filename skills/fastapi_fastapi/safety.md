Five conditions that must NEVER occur in FastAPI applications.

**1. NEVER expose database credentials in code:**
```python
# BAD: Hardcoded credentials
DATABASE_URL = "postgresql://admin:password123@localhost:5432/mydb"

# GOOD: Use environment variables
import os
from dotenv import load_dotenv

load_dotenv()
DATABASE_URL = os.getenv("DATABASE_URL")
if not DATABASE_URL:
    raise ValueError("DATABASE_URL environment variable not set")
```

**2. NEVER disable validation on user input:**
```python
# BAD: Disabling validation
@app.post("/items/")
async def create_item(item: dict):  # No validation
    return item

# GOOD: Always validate with Pydantic
from pydantic import BaseModel, Field

class Item(BaseModel):
    name: str = Field(..., min_length=1, max_length=100)
    price: float = Field(..., gt=0, lt=10000)

@app.post("/items/")
async def create_item(item: Item):
    return item
```

**3. NEVER use eval() or exec() with user input:**
```python
# BAD: Dangerous eval usage
@app.post("/calculate/")
async def calculate(expression: str):
    result = eval(expression)  # Security risk!
    return {"result": result}

# GOOD: Use safe alternatives
import ast

@app.post("/calculate/")
async def calculate(expression: str):
    try:
        tree = ast.parse(expression, mode='eval')
        # Only allow specific operations
        allowed_nodes = (ast.Expression, ast.Constant, ast.BinOp, ast.UnaryOp)
        for node in ast.walk(tree):
            if not isinstance(node, allowed_nodes):
                raise ValueError("Invalid expression")
        result = eval(compile(tree, '<string>', 'eval'))
        return {"result": result}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))
```

**4. NEVER trust user-provided file paths:**
```python
# BAD: Path traversal vulnerability
@app.get("/files/{file_path:path}")
async def read_file(file_path: str):
    with open(f"/data/{file_path}", "r") as f:  # Security risk!
        return {"content": f.read()}

# GOOD: Validate and sanitize paths
import os

@app.get("/files/{file_name}")
async def read_file(file_name: str):
    safe_path = os.path.join("/data", os.path.basename(file_name))
    if not os.path.exists(safe_path):
        raise HTTPException(status_code=404)
    with open(safe_path, "r") as f:
        return {"content": f.read()}
```

**5. NEVER disable HTTPS in production:**
```python
# BAD: HTTP in production
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)  # No HTTPS

# GOOD: Use HTTPS with proper configuration
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=443,
        ssl_keyfile="/etc/ssl/private/key.pem",
        ssl_certfile="/etc/ssl/certs/cert.pem"
    )
```
