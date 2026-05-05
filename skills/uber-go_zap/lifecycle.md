**Construction: Initialize logger at application startup**

```go
package main

import (
    "go.uber.org/zap"
    "go.uber.org/zap/zapcore"
)

func main() {
    // Production configuration
    config := zap.NewProductionConfig()
    config.EncoderConfig.TimeKey = "timestamp"
    config.EncoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder

    logger, err := config.Build()
    if err != nil {
        panic(err)
    }
    defer logger.Sync()

    // Use logger throughout application
    runApp(logger)
}

func runApp(logger *zap.Logger) {
    logger.Info("application started")
}
```

**Initialization: Set up global logger for convenience**

```go
package main

import (
    "go.uber.org/zap"
)

func init() {
    logger, err := zap.NewProduction()
    if err != nil {
        panic(err)
    }
    zap.ReplaceGlobals(logger)
}

func main() {
    defer zap.L().Sync()

    // Use global logger anywhere
    zap.L().Info("application started")
    zap.S().Infof("using sugared global logger")
}
```

**Cleanup: Properly flush and close logger**

```go
package main

import (
    "os"
    "os/signal"
    "syscall"
    "go.uber.org/zap"
)

func main() {
    logger, _ := zap.NewProduction()

    // Handle graceful shutdown
    sigCh := make(chan os.Signal, 1)
    signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

    go func() {
        <-sigCh
        logger.Info("shutting down")
        logger.Sync() // Flush before exit
        os.Exit(0)
    }()

    logger.Info("application running")
    select {} // Block forever
}
```

**Resource management: Use With() to avoid recreating loggers**

```go
package main

import (
    "go.uber.org/zap"
)

type RequestHandler struct {
    baseLogger *zap.Logger
}

func (h *RequestHandler) HandleRequest(id string) {
    // Create request-scoped logger without allocating new encoder
    requestLogger := h.baseLogger.With(
        zap.String("request_id", id),
    )
    requestLogger.Info("processing request")
    // requestLogger is garbage collected after function returns
}
```
