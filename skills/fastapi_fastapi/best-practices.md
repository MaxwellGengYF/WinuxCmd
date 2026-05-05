Recommended patterns for production FastAPI applications.

**1. Use application factory pattern:**
```python
# app.py
from fastapi import FastAPI
from routers import items, users

def create_app() -> FastAPI:
    app = FastAPI(
        title="My API",
        version="1.0.0",
        docs_url="/docs",
        redoc_url="/redoc"
    )
    app.include_router(items.router, prefix="/items", tags=["items"])
    app.include_router(users.router, prefix="/users", tags=["users"])
    return app

app = create_app()
```

**2. Implement proper error handling:**
```python
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse

app = FastAPI()

class AppException(Exception):
    def __init__(self, status_code: int, detail: str):
        self.status_code = status_code
        self.detail = detail

@app.exception_handler(AppException)
async def app_exception_handler(request: Request, exc: AppException):
    return JSONResponse(
        status_code=exc.status_code,
        content={"detail": exc.detail}
    )

@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    return JSONResponse(
        status_code=500,
        content={"detail": "Internal server error"}
    )
```

**3. Use background tasks for non-critical operations:**
```python
from fastapi import FastAPI, BackgroundTasks

app = FastAPI()

def send_email(email: str, message: str):
    # Simulate email sending
    print(f"Sending email to {email}: {message}")

@app.post("/users/")
async def create_user(user: UserCreate, background_tasks: BackgroundTasks):
    # Create user in database
    user_id = await create_user_in_db(user)
    # Send welcome email in background
    background_tasks.add_task(send_email, user.email, "Welcome!")
    return {"user_id": user_id}
```

**4. Implement rate limiting:**
```python
from fastapi import FastAPI, Request, HTTPException
from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address

app = FastAPI()
limiter = Limiter(key_func=get_remote_address)
app.state.limiter = limiter
app.add_exception_handler(429, _rate_limit_exceeded_handler)

@app.get("/items/")
@limiter.limit("100/minute")
async def read_items(request: Request):
    return [{"name": "Foo"}]
```

**5. Use structured logging:**
```python
import logging
from fastapi import FastAPI, Request

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI()

@app.middleware("http")
async def log_requests(request: Request, call_next):
    logger.info(f"Request: {request.method} {request.url}")
    response = await call_next(request)
    logger.info(f"Response: {response.status_code}")
    return response
```
