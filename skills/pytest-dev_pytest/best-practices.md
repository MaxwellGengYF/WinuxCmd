**Pattern 1: Use conftest.py for shared fixtures**
```python
# conftest.py
import pytest
import tempfile
import os

@pytest.fixture
def temp_dir():
    with tempfile.TemporaryDirectory() as tmpdir:
        yield tmpdir

@pytest.fixture
def sample_file(temp_dir):
    filepath = os.path.join(temp_dir, "test.txt")
    with open(filepath, "w") as f:
        f.write("test data")
    return filepath
```

**Pattern 2: Use markers for test categorization**
```python
# test_markers.py
import pytest

@pytest.mark.slow
def test_heavy_computation():
    import time
    time.sleep(5)
    assert True

@pytest.mark.smoke
def test_critical_function():
    assert 1 + 1 == 2

# Run with: pytest -m smoke
```

**Pattern 3: Use fixtures with scope for expensive setup**
```python
# test_scope.py
import pytest

@pytest.fixture(scope="session")
def database_connection():
    # Setup once per test session
    conn = {"connected": True}
    yield conn
    conn["connected"] = False

@pytest.fixture(scope="module")
def module_data():
    # Setup once per module
    return {"module": "data"}
```

**Pattern 4: Use tmp_path fixture for file operations**
```python
# test_tmp_path.py
def test_file_operations(tmp_path):
    d = tmp_path / "sub"
    d.mkdir()
    p = d / "hello.txt"
    p.write_text("content")
    assert p.read_text() == "content"
    assert len(list(tmp_path.iterdir())) == 1
```

**Pattern 5: Use monkeypatch for mocking**
```python
# test_monkeypatch.py
import os

def test_env_variable(monkeypatch):
    monkeypatch.setenv("MY_VAR", "test_value")
    assert os.environ["MY_VAR"] == "test_value"

def test_function_mocking(monkeypatch):
    def mock_get_data():
        return {"mocked": True}
    monkeypatch.setattr("module.get_data", mock_get_data)
```
