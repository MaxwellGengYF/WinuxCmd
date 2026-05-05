`uber-go/zap` is a high-performance, structured, leveled logging library for Go. It provides two main logger types: `Logger` for maximum performance with strongly typed fields, and `SugaredLogger` for convenience with `printf`-style APIs. Zap is designed for applications where logging performance matters, especially in hot paths.

**When to use:**
- Production applications requiring structured logging
- Performance-critical code paths where allocations matter
- Microservices needing consistent log formats (JSON)
- Applications requiring log levels (Debug, Info, Warn, Error, Panic, Fatal)

**When NOT to use:**
- Simple scripts where standard `log` package suffices
- Applications needing human-readable console output (use `NewDevelopment()` instead)
- Projects already using `log/slog` with acceptable performance

**Key design:**
- Reflection-free JSON encoder for zero-allocation in hot paths
- Two-tier API: `Logger` (fast, typed) and `SugaredLogger` (convenient, slower)
- Level-based logging with caller information and stack traces
- Global logger support via `zap.L()` and `zap.ReplaceGlobals()`

**Installation:**
```bash
go get -u go.uber.org/zap
```
