**Pitfall 1: Forgetting to name test files/functions correctly**
```python
# BAD: Won't be discovered
def my_function_test():
    assert 1 == 1

# GOOD: Follows pytest naming convention
def test_my_function():
    assert 1 == 1
```

**Pitfall 2: Using assert with complex expressions**
```python
# BAD: Hard to debug
def test_complex():
    assert [x*2 for x in range(5) if x % 2 == 0] == [0, 4, 8]

# GOOD: Break down into simpler assertions
def test_complex():
    result = [x*2 for x in range(5) if x % 2 == 0]
    assert result == [0, 4, 8]
```

**Pitfall 3: Not using fixtures for shared setup**
```python
# BAD: Duplicated setup code
def test_first():
    data = {"key": "value"}
    assert data["key"] == "value"

def test_second():
    data = {"key": "value"}
    assert len(data) == 1

# GOOD: Use fixture
@pytest.fixture
def sample_data():
    return {"key": "value"}

def test_first(sample_data):
    assert sample_data["key"] == "value"

def test_second(sample_data):
    assert len(sample_data) == 1
```

**Pitfall 4: Not using parametrize for multiple test cases**
```python
# BAD: Duplicated test functions
def test_add_1():
    assert 1 + 1 == 2

def test_add_2():
    assert 2 + 2 == 4

# GOOD: Use parametrize
@pytest.mark.parametrize("a,b,expected", [
    (1, 1, 2),
    (2, 2, 4),
])
def test_add(a, b, expected):
    assert a + b == expected
```

**Pitfall 5: Not cleaning up resources in fixtures**
```python
# BAD: Resource leak
@pytest.fixture
def file_handle():
    return open("test.txt", "w")

# GOOD: Use yield for cleanup
@pytest.fixture
def file_handle():
    f = open("test.txt", "w")
    yield f
    f.close()
```

**Pitfall 6: Using mutable default arguments in test functions**
```python
# BAD: Shared mutable state
def test_mutable_default(data=[]):
    data.append(1)
    assert len(data) == 1

# GOOD: Create new list each time
def test_mutable_default():
    data = []
    data.append(1)
    assert len(data) == 1
```
