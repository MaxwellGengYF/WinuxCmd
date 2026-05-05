```bash
pip install pytest
```

Here are 5 common usage patterns for pytest:

**Pattern 1: Basic test with assert**
```python
# test_basic.py
def test_addition():
    assert 1 + 1 == 2

def test_string():
    assert "hello".upper() == "HELLO"
```

**Pattern 2: Using fixtures for setup/teardown**
```python
# test_fixture.py
import pytest

@pytest.fixture
def sample_data():
    return {"key": "value", "number": 42}

def test_fixture_usage(sample_data):
    assert sample_data["key"] == "value"
    assert sample_data["number"] == 42
```

**Pattern 3: Parametrized tests**
```python
# test_parametrize.py
import pytest

@pytest.mark.parametrize("input,expected", [
    (1, 2),
    (2, 4),
    (3, 6),
])
def test_double(input, expected):
    assert input * 2 == expected
```

**Pattern 4: Testing exceptions**
```python
# test_exceptions.py
import pytest

def test_raises_exception():
    with pytest.raises(ZeroDivisionError):
        1 / 0

def test_exception_message():
    with pytest.raises(ValueError, match="invalid value"):
        raise ValueError("invalid value")
```

**Pattern 5: Using conftest.py for shared fixtures**
```python
# conftest.py
import pytest

@pytest.fixture
def db_connection():
    # Setup
    conn = {"connected": True}
    yield conn
    # Teardown
    conn["connected"] = False

# test_conftest.py
def test_db_connection(db_connection):
    assert db_connection["connected"] is True
```
