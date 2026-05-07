# Pitfalls

**Pitfall 1: Duplicate Metric Registration**

BAD:
```go
func init() {
    prometheus.MustRegister(prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_counter",
        Help: "A counter",
    }))
    prometheus.MustRegister(prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_counter",
        Help: "A counter",
    }))
}
```

GOOD:
```go
var myCounter = prometheus.NewCounter(prometheus.CounterOpts{
    Name: "my_counter",
    Help: "A counter",
})

func init() {
    prometheus.MustRegister(myCounter)
}
```

**Pitfall 2: Inconsistent Label Values**

BAD:
```go
var metric = prometheus.NewCounterVec(
    prometheus.CounterOpts{Name: "requests", Help: "Requests"},
    []string{"method"},
)

func handleRequest(method string) {
    metric.WithLabelValues(method).Inc()
    // Later with different case
    metric.WithLabelValues(strings.ToUpper(method)).Inc()
}
```

GOOD:
```go
func handleRequest(method string) {
    normalizedMethod := strings.ToUpper(method)
    metric.WithLabelValues(normalizedMethod).Inc()
}
```

**Pitfall 3: Not Handling Registration Errors**

BAD:
```go
func setupMetrics() {
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_counter",
        Help: "A counter",
    })
    prometheus.Register(counter) // Ignoring error
}
```

GOOD:
```go
func setupMetrics() error {
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_counter",
        Help: "A counter",
    })
    if err := prometheus.Register(counter); err != nil {
        return fmt.Errorf("failed to register counter: %w", err)
    }
    return nil
}
```

**Pitfall 4: Using Gauge Instead of Counter for Cumulative Values**

BAD:
```go
var totalRequests = prometheus.NewGauge(prometheus.GaugeOpts{
    Name: "total_requests",
    Help: "Total requests",
})

func handleRequest() {
    totalRequests.Inc()
}
```

GOOD:
```go
var totalRequests = prometheus.NewCounter(prometheus.CounterOpts{
    Name: "total_requests",
    Help: "Total requests",
})

func handleRequest() {
    totalRequests.Inc()
}
```

**Pitfall 5: Excessive Label Cardinality**

BAD:
```go
var userRequests = prometheus.NewCounterVec(
    prometheus.CounterOpts{Name: "user_requests", Help: "User requests"},
    []string{"user_id"},
)
```

GOOD:
```go
var userRequests = prometheus.NewCounterVec(
    prometheus.CounterOpts{Name: "user_requests", Help: "User requests"},
    []string{"user_type", "region"},
)
```

**Pitfall 6: Not Using MustRegister for Initialization**

BAD:
```go
func init() {
    err := prometheus.Register(myCounter)
    if err != nil {
        log.Fatal(err)
    }
}
```

GOOD:
```go
func init() {
    prometheus.MustRegister(myCounter)
}
```