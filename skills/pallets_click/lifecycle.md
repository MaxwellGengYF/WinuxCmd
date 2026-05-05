Understanding the lifecycle of Click applications: construction, initialization, cleanup, and resource management.

**Construction and initialization:**
```python
import click
from contextlib import contextmanager

@contextmanager
def managed_resource(name):
    """Context manager for resource lifecycle."""
    click.echo(f"Acquiring {name}")
    resource = {"name": name, "active": True}
    try:
        yield resource
    finally:
        click.echo(f"Releasing {name}")
        resource["active"] = False

@click.group()
@click.option('--verbose', is_flag=True)
@click.pass_context
def cli(ctx, verbose):
    """CLI with resource management."""
    ctx.ensure_object(dict)
    ctx.obj['verbose'] = verbose
    ctx.obj['resources'] = []
    click.echo("CLI initialized")

@cli.command()
@click.argument('name')
@click.pass_context
def use_resource(ctx, name):
    """Use a managed resource."""
    with managed_resource(name) as resource:
        ctx.obj['resources'].append(resource)
        click.echo(f"Using {resource['name']}")
        # Resource is automatically released when exiting the with block

@cli.result_callback()
def cleanup(process_result, **kwargs):
    """Cleanup after all commands."""
    click.echo("Performing cleanup...")
    # Resources are already released by context managers
    click.echo("Cleanup complete")

if __name__ == '__main__':
    cli()
```

**Proper cleanup with atexit:**
```python
import click
import atexit
import tempfile
import shutil

class TempManager:
    def __init__(self):
        self.temp_dirs = []
        atexit.register(self.cleanup)
    
    def create_temp_dir(self):
        temp_dir = tempfile.mkdtemp()
        self.temp_dirs.append(temp_dir)
        return temp_dir
    
    def cleanup(self):
        for temp_dir in self.temp_dirs:
            shutil.rmtree(temp_dir, ignore_errors=True)
        self.temp_dirs.clear()

temp_manager = TempManager()

@click.command()
@click.argument('filename')
def process(filename):
    """Process file with temporary directory."""
    temp_dir = temp_manager.create_temp_dir()
    click.echo(f"Created temp dir: {temp_dir}")
    # Process file...
    click.echo(f"Processing {filename}")

if __name__ == '__main__':
    process()
```

**Resource pooling and reuse:**
```python
import click
from contextlib import contextmanager
from queue import Queue

class ConnectionPool:
    def __init__(self, max_size=5):
        self._pool = Queue(maxsize=max_size)
        self._size = 0
        self._max_size = max_size
    
    @contextmanager
    def get_connection(self):
        conn = self._acquire()
        try:
            yield conn
        finally:
            self._release(conn)
    
    def _acquire(self):
        if not self._pool.empty():
            return self._pool.get()
        if self._size < self._max_size:
            self._size += 1
            return {"id": self._size, "active": True}
        return self._pool.get()  # Block until available
    
    def _release(self, conn):
        self._pool.put(conn)

pool = ConnectionPool(max_size=3)

@click.command()
@click.argument('query')
def query_database(query):
    """Execute database query."""
    with pool.get_connection() as conn:
        click.echo(f"Using connection {conn['id']}")
        # Execute query...
        click.echo(f"Executed: {query}")

if __name__ == '__main__':
    query_database()
```
