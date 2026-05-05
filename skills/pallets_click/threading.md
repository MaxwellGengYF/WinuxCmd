Thread safety considerations and concurrent access patterns in Click applications.

**Click is NOT thread-safe for CLI parsing:**
```python
# BAD - concurrent CLI parsing
import click
import threading

@click.command()
@click.option('--name')
def greet(name):
    click.echo(f"Hello {name}")

# This is unsafe - Click decorators are not thread-safe
threads = []
for i in range(5):
    t = threading.Thread(target=greet, args=(f"User{i}",))
    threads.append(t)
    t.start()
```
```python
# GOOD - serialize CLI access
import click
import threading
from concurrent.futures import ThreadPoolExecutor

lock = threading.Lock()

@click.command()
@click.option('--name')
def greet(name):
    with lock:  # Serialize access to Click
        click.echo(f"Hello {name}")

# Safe concurrent execution
with ThreadPoolExecutor(max_workers=3) as executor:
    futures = [executor.submit(greet, f"User{i}") for i in range(5)]
```

**Thread-safe context management:**
```python
import click
import threading
from contextlib import contextmanager

class ThreadSafeConfig:
    def __init__(self):
        self._lock = threading.RLock()
        self._config = {}
    
    def get(self, key, default=None):
        with self._lock:
            return self._config.get(key, default)
    
    def set(self, key, value):
        with self._lock:
            self._config[key] = value
    
    @contextmanager
    def update(self):
        with self._lock:
            yield self._config

config = ThreadSafeConfig()

@click.group()
@click.option('--debug', is_flag=True)
@click.pass_context
def cli(ctx, debug):
    """CLI with thread-safe config."""
    ctx.ensure_object(dict)
    ctx.obj['config'] = config
    config.set('debug', debug)

@cli.command()
@click.pass_context
def status(ctx):
    """Show status."""
    debug = ctx.obj['config'].get('debug', False)
    click.echo(f"Debug: {debug}")
```

**Thread pool for I/O operations:**
```python
import click
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed

def process_file(filename):
    """Process a single file (I/O bound)."""
    with open(filename, 'r') as f:
        data = f.read()
    return len(data)

@click.command()
@click.argument('files', nargs=-1, type=click.Path(exists=True))
@click.option('--workers', default=4, help='Number of worker threads')
def process_files(files, workers):
    """Process multiple files concurrently."""
    results = {}
    
    with ThreadPoolExecutor(max_workers=workers) as executor:
        future_to_file = {
            executor.submit(process_file, f): f for f in files
        }
        
        for future in as_completed(future_to_file):
            filename = future_to_file[future]
            try:
                size = future.result()
                results[filename] = size
                click.echo(f"Processed {filename}: {size} bytes")
            except Exception as e:
                click.echo(f"Error processing {filename}: {e}", err=True)
    
    total = sum(results.values())
    click.echo(f"Total: {total} bytes from {len(results)} files")

if __name__ == '__main__':
    process_files()
```

**Thread-safe progress bars:**
```python
import click
import threading
import time

class ThreadSafeProgress:
    def __init__(self, total):
        self._lock = threading.Lock()
        self._progress = 0
        self._total = total
    
    def update(self, n=1):
        with self._lock:
            self._progress += n
            return self._progress
    
    @property
    def progress(self):
        with self._lock:
            return self._progress

@click.command()
@click.option('--items', default=100, help='Number of items')
def process_items(items):
    """Process items with thread-safe progress."""
    progress = ThreadSafeProgress(items)
    
    def worker(item_id):
        time.sleep(0.1)  # Simulate work
        current = progress.update()
        return current
    
    threads = []
    for i in range(items):
        t = threading.Thread(target=worker, args=(i,))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
    
    click.echo(f"Processed {progress.progress} items")

if __name__ == '__main__':
    process_items()
```
