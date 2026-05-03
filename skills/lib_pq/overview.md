# Overview

`lib/pq` is a pure Go PostgreSQL driver that implements the `database/sql` interface. It provides a robust, idiomatic way to connect to PostgreSQL databases from Go applications without C dependencies or external libraries.

**When to use:**
- Building Go applications that need PostgreSQL connectivity
- Projects requiring standard `database/sql` interface compliance
- Applications needing PostgreSQL-specific features like LISTEN/NOTIFY, COPY, or advanced authentication
- When you want a pure Go solution without CGO dependencies

**When not to use:**
- If you need LastInsertId support (PostgreSQL doesn't support it; use RETURNING instead)
- For very high-performance requirements where a lower-level driver might be needed
- When you need features not yet supported (check the issue tracker)

**Key design decisions:**
- Implements `database/sql` interfaces for standard Go database access
- Uses PostgreSQL's native wire protocol (no libpq dependency)
- Supports all maintained PostgreSQL versions
- Provides `pq.Config` for structured connection configuration
- Includes `pq.Listener` for async notification handling
- Supports COPY for bulk operations
- Handles authentication (MD5, SCRAM-SHA256, password) natively
- Returns structured errors via `pq.Error` with code-based error checking