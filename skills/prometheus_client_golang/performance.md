# Performance

**Performance Characteristics**

The Prometheus Go client library uses atomic operations for counter and gauge increments, making them very fast. Histograms and summaries have higher overhead due to bucket calculations.

```go
// Counter increment is O(1) with atomic operations
counter.Inc() // Fast, no allocations

// Histogram observation involves bucket search
histogram.Observe(value) // O(log n) where n is number of buckets
```

**Allocation Patterns**

```go
// Avoid allocations in hot paths
// BAD: Creates new labels each time
func handleRequest() {
    counter.WithLabelValues("GET", "/api").Inc()
}

// GOOD: Pre-create label values
var getAPICounter prometheus.Counter

func init() {
    counter := prometheus.NewCounterVec(
        prometheus.CounterOpts{Name: "requests", Help: "Requests"},
        []string{"method", "path"},
    )
    prometheus.MustRegister(counter)
    getAPICounter = counter.WithLabelValues("GET", "/api")
}

func handleRequest() {
    getAPICounter.Inc() // No allocation
}
```

**Optimization Tips**

```go
// Use With() instead of WithLabelValues() for repeated use
var (
    counterVec = prometheus.NewCounterVec(
        prometheus.CounterOpts{Name: "requests", Help: "Requests"},
        []string{"method", "path"},
    )
    cachedCounter prometheus.Counter
)

func init() {
    prometheus.MustRegister(counterVec)
    cachedCounter = counterVec.With(prometheus.Labels{
        "method": "GET",
        "path":   "/api",
    })
}

// Use Inc() instead of Add(1) for single increments
cachedCounter.Inc() // Faster than cachedCounter.Add(1)
```

**Benchmark Considerations**

```go
// Benchmark counter increment
func BenchmarkCounterInc(b *testing.B) {
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "bench_counter",
        Help: "Benchmark counter",
    })
    for i := 0; i < b.N; i++ {
        counter.Inc()
    }
}

// Benchmark histogram observation
func BenchmarkHistogramObserve(b *testing.B) {
    histogram := prometheus.NewHistogram(prometheus.HistogramOpts{
        Name: "bench_histogram",
        Help: "Benchmark histogram",
    })
    for i := 0; i < b.N; i++ {
        histogram.Observe(float64(i))
    }
}
```