**Use Logger for hot paths, SugaredLogger for convenience**

```go
package main

import (
    "go.uber.org/zap"
)

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()

    // Hot path: use Logger with typed fields
    for i := 0; i < 1000; i++ {
        logger.Info("processing item",
            zap.Int("index", i),
            zap.Duration("elapsed", getDuration()),
        )
    }

    // Convenience: use SugaredLogger for complex messages
    sugar := logger.Sugar()
    sugar.Infof("Processed %d items in %v", 1000, getDuration())
}

func getDuration() int {
    return 0
}
```

**Create logger with context for each component**

```go
package main

import (
    "go.uber.org/zap"
)

type Service struct {
    logger *zap.Logger
}

func NewService(logger *zap.Logger) *Service {
    return &Service{
        logger: logger.With(
            zap.String("component", "service"),
            zap.String("version", "1.0.0"),
        ),
    }
}

func (s *Service) HandleRequest(id string) {
    s.logger.Info("handling request",
        zap.String("request_id", id),
    )
}
```

**Use atomic level for dynamic log level changes**

```go
package main

import (
    "go.uber.org/zap"
    "go.uber.org/zap/zapcore"
)

func main() {
    atomicLevel := zap.NewAtomicLevel()
    atomicLevel.SetLevel(zapcore.InfoLevel)

    logger := zap.New(zapcore.NewCore(
        zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
        zapcore.AddSync(os.Stdout),
        atomicLevel,
    ))
    defer logger.Sync()

    // Change level at runtime
    atomicLevel.SetLevel(zapcore.DebugLevel)
    logger.Debug("debug message now visible")
}
```

**Implement structured error logging**

```go
package main

import (
    "errors"
    "go.uber.org/zap"
)

type AppError struct {
    Code    int
    Message string
    Err     error
}

func (e *AppError) Error() string {
    return e.Message
}

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()

    err := &AppError{
        Code:    500,
        Message: "internal server error",
        Err:     errors.New("database timeout"),
    }

    logger.Error("request failed",
        zap.Int("error_code", err.Code),
        zap.String("error_message", err.Message),
        zap.Error(err.Err),
    )
}
```
