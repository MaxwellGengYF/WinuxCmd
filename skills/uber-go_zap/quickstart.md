```go
// Install zap:
// go get -u go.uber.org/zap

package main

import (
    "time"
    "go.uber.org/zap"
)

func main() {
    // Pattern 1: Production logger with SugaredLogger
    logger, _ := zap.NewProduction()
    defer logger.Sync()
    sugar := logger.Sugar()
    sugar.Infow("user login",
        "user", "alice",
        "ip", "192.168.1.1",
    )

    // Pattern 2: Production logger with typed fields
    logger.Info("user login",
        zap.String("user", "alice"),
        zap.String("ip", "192.168.1.1"),
    )

    // Pattern 3: Development logger with stack traces
    devLogger, _ := zap.NewDevelopment()
    defer devLogger.Sync()
    devLogger.Debug("debug message",
        zap.Int("attempt", 3),
    )

    // Pattern 4: Logger with global instance
    zap.ReplaceGlobals(logger)
    zap.L().Info("global logger message")

    // Pattern 5: Logger with fields
    requestLogger := logger.With(
        zap.String("service", "api"),
        zap.String("version", "1.0"),
    )
    requestLogger.Info("request started")

    // Pattern 6: Error logging with stack trace
    err := someFunction()
    if err != nil {
        logger.Error("operation failed",
            zap.Error(err),
            zap.Duration("duration", time.Second),
        )
    }
}

func someFunction() error {
    return nil
}
```
