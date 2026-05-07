# Threading

**Thread Safety Guarantees**

All Prometheus metric types are thread-safe and can be used concurrently from multiple goroutines without external synchronization.

```go
// Safe to use from multiple goroutines
var counter = prometheus.NewCounter(prometheus.CounterOpts{
    Name: "concurrent_counter",
    Help: "A counter used from multiple goroutines",
})

func worker() {
    for i := 0; i < 1000; i++ {
        counter.Inc()
    }
}

func main() {
    go worker()
    go worker()
    go worker()
    time.Sleep(time.Second)
    fmt.Println(counter) // Will show 3000
}
```

**Concurrent Access Patterns**

```go
// Safe concurrent access to counter vec
var counterVec = prometheus.NewCounterVec(
    prometheus.CounterOpts{
        Name: "concurrent_requests",
        Help: "Concurrent requests by endpoint",
    },
    []string{"endpoint"},
)

func handleRequest(endpoint string) {
    counterVec.WithLabelValues(endpoint).Inc()
}

// Multiple goroutines can safely access different label combinations
func main() {
    go handleRequest("/api")
    go handleRequest("/health")
    go handleRequest("/api")
}
```

**Registry Thread Safety**

```go
// Registry operations are thread-safe
registry := prometheus.NewRegistry()

// Safe to register from multiple goroutines
go func() {
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "counter1",
        Help: "Counter 1",
    })
    registry.MustRegister(counter)
}()

go func() {
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "counter2",
        Help: "Counter 2",
    })
    registry.MustRegister(counter)
}()
```

**Atomic Operations**

```go
// Counter and Gauge use atomic operations internally
var counter = prometheus.NewCounter(prometheus.CounterOpts{
    Name: "atomic_counter",
    Help: "Counter with atomic operations",
})

// These operations are atomic and thread-safe
counter.Inc()     // Atomic increment
counter.Add(5)    // Atomic addition

var gauge = prometheus.NewGauge(prometheus.GaugeOpts{
    Name: "atomic_gauge",
    Help: "Gauge with atomic operations",
})

gauge.Set(10)     // Atomic set
gauge.Inc()       // Atomic increment
gauge.Dec()       // Atomic decrement
```