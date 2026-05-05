FastAPI is a modern, high-performance web framework for building APIs with Python, leveraging standard Python type hints. It is built on top of Starlette for the web parts and Pydantic for the data parts, providing automatic interactive API documentation (Swagger UI and ReDoc).

**When to use FastAPI:**
- Building RESTful APIs that need automatic OpenAPI documentation
- Projects requiring high performance (on par with NodeJS and Go)
- Applications that benefit from Python type hints for validation and editor support
- Microservices and async-heavy workloads

**When NOT to use FastAPI:**
- Simple static websites (use Flask or Django instead)
- Projects where you don't need API documentation
- When you need a full-featured web framework with built-in ORM and admin panels (use Django)

**Key design principles:**
- Based on standard Python type hints for request validation and serialization
- Async-first but supports synchronous endpoints
- Automatic OpenAPI and JSON Schema generation
- Dependency injection system for reusable components

**Installation:**
```bash
pip install "fastapi[standard]"
```

This installs FastAPI with all standard dependencies including uvicorn for running the server.
