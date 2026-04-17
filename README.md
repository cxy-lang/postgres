# @postgres

A PostgreSQL client library for Cxy that wraps libpq with a high-level, type-safe API.

## Features

- 🔄 **Connection Pooling** - Automatic connection management with configurable keep-alive
- ⚡ **Async Support** - Non-blocking queries using Cxy's coroutine scheduler
- 🔍 **Type Safety** - Compile-time type checking with reflection-based struct mapping
- 🏗️ **Reflection-Based Tables** - Automatic table creation from struct definitions
- 🔒 **Transactions** - Full transaction support with savepoints
- 📦 **Prepared Statements** - Statement caching for improved performance
- 🎯 **Flexible Queries** - Parameterized queries with automatic type conversion


## Installation

Add to your `Cxyfile.yaml`:

```yaml
dependencies:
  - name: postgres
    version: 0.1.0
```

Then install:

```bash
cxy package install
```

### System Requirements

#### Build-Time Dependencies

For compiling your Cxy application with the `@postgres` package:

- **macOS**: `brew install postgresql`
- **Ubuntu/Debian**: `apt-get install libpq-dev`
- **Alpine**: `apk add libpq-dev`

#### Runtime Dependencies

For running the compiled binary, you only need the PostgreSQL client libraries (not the `-dev` packages):

- **macOS**: `brew install libpq` (or `postgresql` - same thing)
- **Ubuntu/Debian**: `apt-get install libpq5`
- **Alpine**: `apk add libpq`

**Docker Multi-Stage Build Example:**

```dockerfile
# Build stage
FROM alpine:latest AS builder
RUN apk add --no-cache libpq-dev build-base cxy
COPY . /app
WORKDIR /app
RUN cxy build --release

# Runtime stage - much smaller!
FROM alpine:latest
RUN apk add --no-cache libpq
COPY --from=builder /app/build/myapp /usr/local/bin/
CMD ["myapp"]
```

**Note:** The `-dev` packages include headers and static libraries needed for compilation. Runtime containers only need the shared libraries (`.so` files).

## Quick Start

```cxy
import { Database } from "@postgres"

func main() {
    // Open database connection
    var db = Database.open("host=localhost dbname=mydb user=postgres password=secret") catch {
        println("Connection failed: ", ex!)
        return
    }

    // Execute a query
    var result = db.connection().exec("SELECT * FROM users WHERE id = $1", 1) catch {
        println("Query failed: ", ex!)
        return
    }

    // Read results
    while result.next() {
        var id = result.column[i64](0) catch 0
        var name = result.column[String](1) catch "unknown"
        println(f"User {id}: {name}")
    }
}
```

## Basic Usage

### Connecting to a Database

```cxy
import { Database } from "@postgres"

// URI format (recommended)
var db = Database.open("postgresql://user:pass@localhost/mydb")
var db = Database.open("postgresql://user:pass@localhost:5432/mydb")
var db = Database.open("postgres://user@localhost/mydb")  // No password

// Key-value format
var db = Database.open("host=localhost dbname=mydb user=postgres password=secret")
var db = Database.open("host=192.168.1.100 port=5433 dbname=mydb user=admin")

// With all options
var db = Database.open(
    "postgresql://user:pass@localhost/mydb",
    "public",      // schema name (default: "public")
    false,         // async mode (default: false)
    5000,          // timeout in ms (default: -1 = no timeout)
    60000          // keep-alive in ms (default: -1 = no keep-alive)
) catch {
    println("Failed to connect: ", ex!)
    return
}
```

**Connection String Formats:**
- **URI**: `postgresql://[user[:password]@][host][:port][/dbname][?param=value]`
- **Key-Value**: `host=localhost port=5432 dbname=mydb user=postgres password=secret`

### Executing Queries

```cxy
// Simple query
var result = db.connection().exec("SELECT version()")

// Parameterized query
var result = db.connection().exec(
    "SELECT * FROM users WHERE age > $1 AND city = $2",
    18,
    "New York"
)

// INSERT with auto-increment
var result = db.connection().exec(
    "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id",
    "Alice",
    "alice@example.com"
)
```

### Reading Results

#### Single Values

```cxy
var result = db.connection().exec("SELECT COUNT(*) FROM users") catch {
    println("Query failed: ", ex!)
    return
}

if result.next() {
    var count = result.column[i64](0) catch 0
    println(f"Total users: {count}")
}
```

#### Multiple Columns

```cxy
var result = db.connection().exec("SELECT id, name, email FROM users") catch {
    println("Query failed: ", ex!)
    return
}

while result.next() {
    var id = result.column[i64](0) catch 0
    var name = result.column[String](1) catch "unknown"
    var email = result.column[String](2) catch "no-email"
    println(f"{id}: {name} <{email}>")
}
```

#### Into Tuples

```cxy
var result = db.connection().exec("SELECT id, name, active FROM users LIMIT 1") catch {
    println("Query failed: ", ex!)
    return
}

if result.next() {
    var user = result.read[(i64, String, bool)]() catch {
        println("Failed to read row: ", ex!)
        return
    }
    println(f"User: {user.0}, {user.1}, {user.2}")
}
```

## Struct Mapping (Reflection)

The library supports automatic mapping of query results to structs using reflection:

```cxy
struct User {
    id: i64
    name: String
    email: String
    active: bool
    created_at: String
}

var result = db.connection().exec("SELECT * FROM users WHERE id = $1", 1) catch {
    println("Query failed: ", ex!)
    return
}

if result.next() {
    var user = result.read[User]() catch {
        println("Failed to read user: ", ex!)
        return
    }
    println(f"User: {user.name} ({user.email})")
}
```

### Reading into Vectors

```cxy
import { Vector } from "stdlib/vector.cxy"

var result = db.connection().exec("SELECT * FROM users") catch {
    println("Query failed: ", ex!)
    return
}

var users = Vector[User]()
result >> users catch {
    println("Failed to read users: ", ex!)
    return
}

for user, _ in users {
    println(f"{user.id}: {user.name}")
}
```

### Field Attributes

Use `@sql` attributes to customize field mapping (same as sqlite.cxy):

```cxy
struct Product {
    @sql(id: 0)
    product_id: i64        // Maps to column 0
    
    @sql(name: "product_name")
    name: String           // Maps to column named "product_name"
    
    @sql(ignore)
    cached_value: i32      // Ignored during mapping
    
    price: f64             // Auto-mapped by field name
}
```

## Reflection-Based Table Creation

Create database tables automatically from struct definitions using `createTable[T]()`:

```cxy
struct User {
    @sql(primaryKey, autoIncrement)
    id: i64
    
    @sql(notNull)
    name: String
    
    @sql(unique)
    email: String
    
    age: i32
}

var conn = db.connection()
conn.createTable[User]("users") catch {
    println("Failed to create table: ", ex!)
    return
}
// Creates: CREATE TABLE IF NOT EXISTS users (
//   id bigint PRIMARY KEY GENERATED ALWAYS AS IDENTITY,
//   name text NOT NULL,
//   email text UNIQUE,
//   age integer
// )
```

**Note:** `createTable` can throw exceptions (e.g., permission errors, invalid schema). Always handle errors appropriately.

### Table Definition Attributes

Use `@sql` attributes to control table schema generation:

```cxy
struct Order {
    @sql(primaryKey, autoIncrement)
    id: i64
    
    @sql(references: "customers(id)", notNull)
    customer_id: i64
    
    @sql(name: "order_total", notNull)
    total: f64
    
    @sql(ignore)
    cachedData: String  // Not created in database
}
```

**Available attributes:**
- `@sql(primaryKey)` - PRIMARY KEY constraint
- `@sql(autoIncrement)` - GENERATED ALWAYS AS IDENTITY (integers only)
- `@sql(unique)` - UNIQUE constraint
- `@sql(notNull)` - NOT NULL constraint
- `@sql(references: "table(column)")` - Foreign key constraint
- `@sql(name: "column_name")` - Custom column name
- `@sql(ignore)` - Exclude from table definition

**Type mappings:**
- `i16`, `u16` → SMALLINT
- `i32`, `u32` → INTEGER
- `i64`, `u64` → BIGINT
- `f32` → REAL
- `f64` → DOUBLE PRECISION
- `bool` → BOOLEAN
- `char` → CHAR(1)
- String types → TEXT
- enum → INTEGER

## Transactions

```cxy
import { PgTransaction } from "@postgres"

var conn = db.connection() catch {
    println("Failed to get connection: ", ex!)
    return
}

var tx = PgTransaction(&conn, true)  // auto-commit = true

// Execute statements within transaction
tx.exec("INSERT INTO accounts (name, balance) VALUES ($1, $2)", "Alice", 1000.0) catch {
    println("Transaction failed: ", ex!)
    return
}
tx.exec("INSERT INTO accounts (name, balance) VALUES ($1, $2)", "Bob", 500.0) catch {
    println("Transaction failed: ", ex!)
    return
}

// Explicit commit (auto-commit on scope exit if not called)
tx.commit() catch {
    println("Commit failed: ", ex!)
}
```

### Savepoints

```cxy
var tx = PgTransaction(&conn, false)  // manual commit

tx.exec("UPDATE accounts SET balance = balance - 100 WHERE name = 'Alice'")

tx.savepoint("sp1")
tx.exec("UPDATE accounts SET balance = balance + 100 WHERE name = 'Bob'")

// Rollback to savepoint
tx.rollback("sp1")

// Or release savepoint
tx.release("sp1")

tx.commit()
```

## Async Mode

Enable async mode for non-blocking queries using Cxy's coroutine scheduler:

```cxy
// Create database with async enabled
var db = Database.open(
    "postgresql://user:pass@localhost/mydb",
    "public",
    true,     // async = true
    5000      // timeout = 5 seconds
)

// Queries automatically use async I/O
async {
    var result = db.connection().exec("SELECT * FROM large_table")
    while (result.next()) {
        // Process results
    }
}
```

## Connection Pooling

The `Database` class automatically manages a connection pool:

```cxy
var db = Database.open(
    "postgresql://user:pass@localhost/mydb",
    "public",
    false,
    -1,
    60000  // keep-alive: 60 seconds
)

// Get connection from pool
var conn = db.connection()

// Use connection
var result = conn.exec("SELECT * FROM users")

// Connection is automatically returned to pool when `conn` goes out of scope
// Idle connections are cleaned up after keep-alive timeout
```

Check pool size:

```cxy
println(f"Pool size: {db.poolSize()}")
```

## Supported Types

### Parameter Binding

- Integers: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- Floats: `f32`, `f64`
- Strings: `string`, `String`, `__string`
- Boolean: `bool`
- Character: `char`

### Result Reading

All parameter types plus:

- Optionals: `i32?`, `String?`, etc. (maps NULL values)
- Tuples: `(i64, String, bool)`
- Structs: Any struct with default constructor

## Error Handling

```cxy
import { PgError, PgConnectionError, PgQueryError } from "@postgres"

var db = Database.open("postgresql://localhost/mydb") catch {
    match ex! {
        PgConnectionError as err => {
            println("Connection error: ", err.what())
            return
        }
        PgError as err => {
            println("General error: ", err.what())
            return
        }
        ... => {
            println("Unknown error: ", ex!.what())
            return
        }
    }
}

var result = db.connection().exec("SELECT * FROM users") catch {
    if ex! is PgQueryError {
        println("Query failed: ", ex!.what())
    }
    return
}
```

## Advanced Examples

### Checking if Table Exists

```cxy
var conn = db.connection()

if conn.hasTable("users") {
    println("Table 'users' exists")
}

if conn.hasTable("public", "users") {
    println("Table 'public.users' exists")
}
```

### Prepared Statements

```cxy
var conn = db.connection()

// Prepare a statement (cached internally)
var stmt = conn.prepare("SELECT * FROM users WHERE age > $1")

// Execute multiple times with different parameters
var result1 = stmt.exec(18)
var result2 = stmt.exec(25)
var result3 = stmt.exec(30)
```

### Optional Fields (NULL Handling)

```cxy
struct User {
    id: i64
    name: String
    email: String?        // Can be NULL
    phone: String?        // Can be NULL
}

var result = db.connection().exec("SELECT id, name, email, phone FROM users")
while result.next() {
    var user = result.read[User]()
    
    println(f"User: {user.name}")
    
    if !!user.email {
        println(f"  Email: {*user.email}")
    }
    
    if !!user.phone {
        println(f"  Phone: {*user.phone}")
    }
}
```

### Complex Queries

```cxy
struct OrderSummary {
    order_id: i64
    customer_name: String
    total_amount: f64
    item_count: i32
    order_date: String
}

var result = db.connection().exec("""
    SELECT 
        o.id as order_id,
        c.name as customer_name,
        SUM(oi.price * oi.quantity) as total_amount,
        COUNT(oi.id) as item_count,
        o.created_at as order_date
    FROM orders o
    JOIN customers c ON o.customer_id = c.id
    JOIN order_items oi ON oi.order_id = o.id
    WHERE o.created_at > $1
    GROUP BY o.id, c.name, o.created_at
    ORDER BY total_amount DESC
""", "2024-01-01")

var summaries = Vector[OrderSummary]()
result >> summaries

for summary, _ in summaries {
    println(f"Order {summary.order_id}: ${summary.total_amount}")
}
```

## API Reference

### `Database`

**Static Methods:**
- `open(connStr: string, dbname: string = "public", async: bool = false, timeout: i64 = -1, keepAlive: i64 = -1): !Database` - Open database connection with optional schema name, async mode, timeout, and keep-alive settings

**Instance Methods:**
- `connection(): !PgConnection` - Get a connection from the pool
- `dbname(): String` - Get database/schema name
- `poolSize(): u64` - Get current pool size

### `PgConnection`

**Methods:**
- `exec(sql: string, ...args: auto): !PgResult` - Execute query with variadic parameters (uses PostgreSQL `$1`, `$2`, etc. placeholders)
- `prepare(sql: string): !PgStatement` - Prepare and cache statement for reuse
- `createTable[T](name: string): !void` - Create table from struct definition using reflection
- `hasTable(name: string): !bool` - Check if table exists in current schema
- `hasTable(schema: string, name: string): !bool` - Check if table exists in specific schema
- `close()` - Close connection and return to pool
- `isOpen(): bool` - Check if connection is open

### `PgResult`

**Methods:**
- `next(): bool` - Advance to next row (returns `false` when no more rows)
- `column[T](index: i32): !T` - Read column by zero-based index with type conversion
- `read[T](): !T` - Read current row into type T (tuple or struct)
- `>>[T](vec: &Vector[T]): !void` - Read all remaining rows into vector
- `empty(): bool` - Check if result set is empty
- `size(): i32` - Get total number of rows in result
- `reset()` - Reset row cursor to beginning

### `PgStatement`

**Methods:**
- `exec(...args: auto): !PgResult` - Execute prepared statement with variadic parameters
- `sql(): String` - Get the SQL query string
- `isAsync(): bool` - Check if async mode is enabled for this statement

### `PgTransaction`

**Constructor:**
- `PgTransaction(conn: &PgConnection, autoCommit: bool)` - Create transaction with auto-commit option

**Methods:**
- `begin(): !void` - Explicitly begin transaction (called automatically)
- `commit(): !void` - Commit transaction
- `rollback(savepoint: string = null): !void` - Rollback entire transaction or to specific savepoint
- `savepoint(name: string): !void` - Create named savepoint
- `release(name: string): !void` - Release savepoint (makes it inaccessible)
- `exec(sql: string, ...args: auto): !void` - Execute query within transaction context
- `isActive(): bool` - Check if transaction is currently active

### Exception Types

- `PgError` - Base exception for all PostgreSQL errors
- `PgConnectionError` - Connection-related errors (extends `PgError`)
- `PgQueryError` - Query execution errors (extends `PgError`)

## Performance Tips

1. **Use Prepared Statements** - For queries executed multiple times, use `prepare()` to cache and reuse statements
2. **Enable Connection Pooling** - Set appropriate `keepAlive` timeout to reuse connections
3. **Batch Operations** - Use transactions for multiple INSERT/UPDATE operations
4. **Use Async Mode** - Enable async for I/O-bound workloads with many concurrent queries
5. **Parameterized Queries** - Always use `$1`, `$2` placeholders instead of string concatenation

## Thread Safety and Best Practices

### Thread Safety

- **Database**: Thread-safe. Multiple threads can call `db.connection()` safely
- **PgConnection**: NOT thread-safe. Each thread should get its own connection from the pool
- **PgResult**: NOT thread-safe. Results should not be shared between threads
- **PgTransaction**: NOT thread-safe. Transactions are bound to a single connection

### Concurrent Access Pattern

```cxy
import { Database } from "@postgres"

// Shared database instance (thread-safe)
var db = Database.open("postgresql://localhost/mydb", "public", false, -1, 60000)

// Spawn multiple threads
launch {
    // Each thread gets its own connection
    var conn = db.connection() catch {
        println("Thread 1 failed to get connection")
        return
    }
    var result = conn.exec("SELECT * FROM users WHERE id = $1", 1)
    // Process result...
}

launch {
    // Another thread with its own connection
    var conn = db.connection() catch {
        println("Thread 2 failed to get connection")
        return
    }
    var result = conn.exec("SELECT * FROM products LIMIT 10")
    // Process result...
}
```

### Best Practices

**Connection Management:**
- Always get a fresh connection from the pool for each thread
- Set appropriate `keepAlive` timeout to balance connection reuse and resource cleanup
- Monitor pool size with `db.poolSize()` to detect connection leaks

**Query Optimization:**
- Use prepared statements for repeated queries
- Use parameterized queries (`$1`, `$2`) instead of string concatenation
- Batch multiple operations in transactions when possible

**Error Handling:**
- Always handle errors from `exec()`, `column[T]()`, and `read[T]()`
- Use specific exception types (`PgConnectionError`, `PgQueryError`) for targeted error handling
- Log connection errors for monitoring and debugging

**Production Deployment:**
- Use connection pooling with reasonable `keepAlive` values (e.g., 60000ms)
- Set query timeouts to prevent long-running queries from blocking connections
- Monitor pool size and adjust based on load
- Use async mode for I/O-bound applications with many concurrent queries

**Security:**
- Never concatenate user input directly into SQL strings
- Always use parameterized queries with `$1`, `$2` placeholders
- Use environment variables for connection strings (avoid hardcoding credentials)
- Enable SSL/TLS in production: `"postgresql://user@host/db?sslmode=require"`

## Troubleshooting

### Connection Issues

**Problem:** `PgConnectionError: could not connect to server`

**Solutions:**
- Verify PostgreSQL is running: `pg_isready -h localhost -p 5432`
- Check connection string format and credentials
- Ensure firewall allows connections to PostgreSQL port (default: 5432)
- Verify `pg_hba.conf` allows connections from your host
- Check if PostgreSQL is listening on the correct interface in `postgresql.conf`

**Problem:** `FATAL: database "mydb" does not exist`

**Solutions:**
- Create the database: `CREATE DATABASE mydb;`
- Verify database name in connection string matches existing database
- Use `\l` in `psql` to list available databases

**Problem:** `FATAL: role "username" does not exist`

**Solutions:**
- Create the user: `CREATE USER username WITH PASSWORD 'password';`
- Grant necessary privileges: `GRANT ALL PRIVILEGES ON DATABASE mydb TO username;`
- Verify username in connection string is correct

### Query Errors

**Problem:** `PgQueryError: column "xyz" does not exist`

**Solutions:**
- Verify column name spelling and case sensitivity
- Use `\d tablename` in `psql` to see table schema
- Check if column exists in the correct table/schema

**Problem:** `PgQueryError: syntax error at or near "$1"`

**Solutions:**
- PostgreSQL uses `$1`, `$2` for parameters, not `?` (SQLite style)
- Ensure parameter count matches placeholder count
- Check SQL syntax is valid PostgreSQL dialect

**Problem:** Type conversion errors when reading results

**Solutions:**
- Verify column type matches the type you're requesting with `column[T]()`
- Use appropriate Cxy type (e.g., `i64` for PostgreSQL `bigint`, `String` for `text`)
- For NULL values, use optional types: `column[i64?](0)`

### Performance Issues

**Problem:** Slow queries

**Solutions:**
- Add indexes to frequently queried columns
- Use `EXPLAIN ANALYZE` to identify slow operations
- Use prepared statements for repeated queries
- Consider using async mode for I/O-bound workloads

**Problem:** Too many connections / connection pool exhausted

**Solutions:**
- Reduce `keepAlive` timeout to release idle connections faster
- Monitor pool size with `db.poolSize()`
- Ensure connections are properly closed (let them go out of scope)
- Increase PostgreSQL `max_connections` setting if needed

### Compilation Issues

**Problem:** `undefined reference to PQconnectdb` or similar libpq errors

**Solutions:**
- Install PostgreSQL development libraries (build-time):
  - macOS: `brew install postgresql`
  - Ubuntu/Debian: `apt-get install libpq-dev`
  - Alpine: `apk add libpq-dev`
- For runtime-only containers, install runtime libraries:
  - macOS: `brew install libpq`
  - Ubuntu/Debian: `apt-get install libpq5`
  - Alpine: `apk add libpq`
- Ensure `libpq` is in your library path
- Check that `@__cc:lib "pq"` is present in the package

**Problem:** Type inference errors with struct mapping

**Solutions:**
- Ensure struct has a default constructor (parameterless `init`)
- All struct fields must match query column types
- Use `@sql(ignore)` for fields not in the database
- Use `@sql(name: "column_name")` for fields with different names

### Runtime Issues

**Problem:** Segmentation fault or crash

**Solutions:**
- Ensure you're not sharing `PgConnection` or `PgResult` between threads
- Verify you're not accessing result columns after moving to next row without re-reading
- Check for moved variables in loops (see AGENTS.md warning about moving in loops)
- Enable debug mode to get better stack traces

**Problem:** Transaction automatically rolled back

**Solutions:**
- Check for exceptions thrown within transaction that weren't caught
- Verify `commit()` is called before transaction goes out of scope (if auto-commit is false)
- Look for constraint violations or deadlocks in PostgreSQL logs

### Getting Help

If you encounter issues not covered here:

1. Check PostgreSQL server logs for detailed error messages
2. Enable verbose error output: `println("Error: ", ex!.what())`
3. Verify your query works in `psql` command-line tool first
4. Check the [PostgreSQL documentation](https://www.postgresql.org/docs/) for PostgreSQL-specific issues
5. Review the package source code in the repository
6. Submit an issue with a minimal reproducible example

## License

MIT

## Contributing

Contributions are welcome! Please submit issues and pull requests on the repository.