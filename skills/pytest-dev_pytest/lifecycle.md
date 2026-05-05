**Construction and Initialization:**
pytest tests follow a clear lifecycle: discovery, collection, and execution. Fixtures are created before tests and cleaned up after.

```python
# test_lifecycle.py
import pytest

@pytest.fixture
def resource():
    print("\n[SETUP] Creating resource")
    resource = {"initialized": True}
    yield resource
    print("[TEARDOWN] Cleaning up resource")

def test_resource_lifecycle(resource):
    print("[TEST] Using resource")
    assert resource["initialized"] is True
```

**Fixture scopes and their lifecycle:**
```python
# test_scopes.py
import pytest

@pytest.fixture(scope="session")
def session_resource():
    print("\n[SESSION] Created once per test session")
    yield
    print("[SESSION] Cleaned up after all tests")

@pytest.fixture(scope="module")
def module_resource():
    print("\n[MODULE] Created once per module")
    yield
    print("[MODULE] Cleaned up after module tests")

@pytest.fixture(scope="class")
def class_resource():
    print("\n[CLASS] Created once per class")
    yield
    print("[CLASS] Cleaned up after class tests")

@pytest.fixture(scope="function")
def function_resource():
    print("\n[FUNCTION] Created for each test")
    yield
    print("[FUNCTION] Cleaned up after each test")
```

**Resource management with context managers:**
```python
# test_resource_mgmt.py
import pytest
from contextlib import contextmanager

@contextmanager
def managed_resource():
    print("[SETUP] Acquiring resource")
    resource = {"available": True}
    try:
        yield resource
    finally:
        print("[TEARDOWN] Releasing resource")
        resource["available"] = False

@pytest.fixture
def managed_fixture():
    with managed_resource() as resource:
        yield resource

def test_managed_resource(managed_fixture):
    assert managed_fixture["available"] is True
```

**Cleanup with addfinalizer:**
```python
# test_finalizer.py
import pytest

@pytest.fixture
def resource_with_finalizer():
    resource = {"initialized": True}
    def cleanup():
        resource["initialized"] = False
        print("[FINALIZER] Cleanup executed")
    request.addfinalizer(cleanup)
    return resource
```
