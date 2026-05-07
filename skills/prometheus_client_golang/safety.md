# Safety

**Condition 1: NEVER register metrics with the same name and label values**

BAD:
```go
func init() {
    prometheus.MustRegister(prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_metric",
        Help: "First metric",
    }))
    prometheus.MustRegister(prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_metric",
        Help: "Second metric",
    }))
}
```

GOOD:
```go
var myMetric = prometheus.NewCounter(prometheus.CounterOpts{
    Name: "my_metric",
    Help: "My unique metric",
})

func init() {
    prometheus.MustRegister(myMetric)
}
```

**Condition 2: NEVER use nil metric pointers**

BAD:
```go
var myCounter *prometheus.Counter

func handleRequest() {
    myCounter.Inc() // nil pointer dereference
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

func handleRequest() {
    myCounter.Inc()
}
```

**Condition 3: NEVER modify metric names after registration**

BAD:
```go
var myCounter = prometheus.NewCounter(prometheus.CounterOpts{
    Name: "my_counter",
    Help: "A counter",
})

func init() {
    prometheus.MustRegister(myCounter)
}

func updateMetricName() {
    myCounter = prometheus.NewCounter(prometheus.CounterOpts{
        Name: "new_counter",
        Help: "A counter",
    })
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

**Condition 4: NEVER ignore registration errors in production**

BAD:
```go
func setupMetrics() {
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "my_counter",
        Help: "A counter",
    })
    prometheus.Register(counter) // Error silently ignored
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
        return fmt.Errorf("metric registration failed: %w", err)
    }
    return nil
}
```

**Condition 5: NEVER use labels with unbounded cardinality**

BAD:
```go
var requestLatency = prometheus.NewHistogramVec(
    prometheus.HistogramOpts{
        Name: "request_latency_seconds",
        Help: "Request latency",
    },
    []string{"request_id"}, // Each request has unique ID
)
```

GOOD:
```go
var requestLatency = prometheus.NewHistogramVec(
    prometheus.HistogramOpts{
        Name: "request_latency_seconds",
        Help: "Request latency",
    },
    []string{"method", "status_code"}, // Bounded label set
)
```