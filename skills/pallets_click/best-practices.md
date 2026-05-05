Recommended patterns for production Click applications.

**Use type annotations and validation:**
```python
import click
from typing import Optional

def validate_port(ctx, param, value):
    if value is not None and (value < 1 or value > 65535):
        raise click.BadParameter("Port must be between 1 and 65535")
    return value

@click.command()
@click.option('--host', default='localhost', help='Server host')
@click.option('--port', default=8080, callback=validate_port, type=int, help='Server port')
def serve(host: str, port: int):
    """Start the server."""
    click.echo(f"Starting server on {host}:{port}")
```

**Structure multi-command CLIs with groups:**
```python
import click

@click.group()
@click.version_option('1.0.0')
def cli():
    """My CLI tool - manage resources."""
    pass

@cli.group()
def config():
    """Configuration commands."""
    pass

@config.command()
@click.argument('key')
@click.argument('value')
def set(key, value):
    """Set a configuration value."""
    click.echo(f"Setting {key}={value}")

@config.command()
@click.argument('key')
def get(key):
    """Get a configuration value."""
    click.echo(f"Getting {key}")

if __name__ == '__main__':
    cli()
```

**Use context for dependency injection:**
```python
import click
from dataclasses import dataclass

@dataclass
class Config:
    debug: bool = False
    verbose: bool = False

@click.group()
@click.option('--debug', is_flag=True)
@click.option('--verbose', '-v', is_flag=True)
@click.pass_context
def cli(ctx, debug, verbose):
    """CLI with shared configuration."""
    ctx.ensure_object(dict)
    ctx.obj['config'] = Config(debug=debug, verbose=verbose)

@cli.command()
@click.pass_context
def status(ctx):
    """Show status."""
    config = ctx.obj['config']
    if config.debug:
        click.echo("Debug mode enabled")
    click.echo("Status: OK")
```

**Implement proper error handling:**
```python
import click
import sys

class AppError(click.ClickException):
    """Custom application error."""
    def __init__(self, message):
        super().__init__(message)
        self.exit_code = 1

@click.command()
@click.argument('filename')
def process(filename):
    """Process a file."""
    try:
        with open(filename, 'r') as f:
            data = f.read()
    except FileNotFoundError:
        raise AppError(f"File not found: {filename}")
    except PermissionError:
        raise AppError(f"Permission denied: {filename}")
    
    click.echo(f"Processed {len(data)} bytes")

if __name__ == '__main__':
    try:
        process()
    except AppError as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(e.exit_code)
```

**Use lazy loading for large subcommands:**
```python
import click

@click.group()
def cli():
    """Main CLI."""
    pass

@cli.group()
def database():
    """Database commands."""
    pass

@database.command()
def migrate():
    """Run database migrations."""
    # Lazy import for slow dependencies
    from myapp.database import run_migrations
    run_migrations()

@database.command()
def seed():
    """Seed the database."""
    from myapp.database import seed_data
    seed_data()

if __name__ == '__main__':
    cli()
```
