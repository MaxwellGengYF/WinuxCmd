# Quickstart

```go
// Pattern 1: Basic Counter
package main

import (
    "net/http"
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
)

var (
    requestsTotal = prometheus.NewCounter(prometheus.CounterOpts{
        Name: "http_requests_total",
        Help: "Total number of HTTP requests",
    })
)

func init() {
    prometheus.MustRegister(requestsTotal)
}

func handler(w http.ResponseWriter, r *http.Request) {
    requestsTotal.Inc()
    w.Write([]byte("OK"))
}

func main() {
    http.Handle("/metrics", promhttp.Handler())
    http.HandleFunc("/", handler)
    http.ListenAndServe(":8080", nil)
}
```

```go
// Pattern 2: CounterVec with Labels
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
)

var (
    httpRequestsTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "http_requests_total",
            Help: "Total number of HTTP requests by status code and method",
        },
        []string{"code", "method"},
    )
)

func init() {
    prometheus.MustRegister(httpRequestsTotal)
}

func handler(w http.ResponseWriter, r *http.Request) {
    httpRequestsTotal.WithLabelValues("200", r.Method).Inc()
    w.Write([]byte("OK"))
}

func main() {
    http.Handle("/metrics", promhttp.Handler())
    http.HandleFunc("/", handler)
    http.ListenAndServe(":8080", nil)
}
```

```go
// Pattern 3: Gauge for Current Values
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
    "time"
)

var (
    queueSize = prometheus.NewGauge(prometheus.GaugeOpts{
        Name: "queue_size",
        Help: "Current size of the work queue",
    })
)

func init() {
    prometheus.MustRegister(queueSize)
}

func main() {
    go func() {
        for {
            queueSize.Set(float64(len(getQueue())))
            time.Sleep(5 * time.Second)
        }
    }()
    http.Handle("/metrics", promhttp.Handler())
    http.ListenAndServe(":8080", nil)
}

func getQueue() []int {
    return []int{1, 2, 3}
}
```

```go
// Pattern 4: Histogram for Latency
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
    "time"
)

var (
    requestDuration = prometheus.NewHistogram(prometheus.HistogramOpts{
        Name:    "request_duration_seconds",
        Help:    "Histogram of request durations",
        Buckets: prometheus.DefBuckets,
    })
)

func init() {
    prometheus.MustRegister(requestDuration)
}

func handler(w http.ResponseWriter, r *http.Request) {
    start := time.Now()
    defer func() {
        requestDuration.Observe(time.Since(start).Seconds())
    }()
    time.Sleep(100 * time.Millisecond)
    w.Write([]byte("OK"))
}

func main() {
    http.Handle("/metrics", promhttp.Handler())
    http.HandleFunc("/", handler)
    http.ListenAndServe(":8080", nil)
}
```

```go
// Pattern 5: Summary for Quantiles
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
    "time"
)

var (
    requestLatency = prometheus.NewSummary(prometheus.SummaryOpts{
        Name:       "request_latency_seconds",
        Help:       "Request latency summary",
        Objectives: map[float64]float64{0.5: 0.05, 0.9: 0.01, 0.99: 0.001},
    })
)

func init() {
    prometheus.MustRegister(requestLatency)
}

func handler(w http.ResponseWriter, r *http.Request) {
    start := time.Now()
    defer func() {
        requestLatency.Observe(time.Since(start).Seconds())
    }()
    time.Sleep(50 * time.Millisecond)
    w.Write([]byte("OK"))
}

func main() {
    http.Handle("/metrics", promhttp.Handler())
    http.HandleFunc("/", handler)
    http.ListenAndServe(":8080", nil)
}
```

```go
// Pattern 6: Custom Registry
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
)

func main() {
    registry := prometheus.NewRegistry()
    counter := prometheus.NewCounter(prometheus.CounterOpts{
        Name: "custom_counter",
        Help: "A counter in a custom registry",
    })
    registry.MustRegister(counter)

    http.Handle("/metrics", promhttp.HandlerFor(registry, promhttp.HandlerOpts{}))
    http.ListenAndServe(":8080", nil)
}
```

```go
// Pattern 7: GaugeVec with Dynamic Labels
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
)

var (
    activeConnections = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "active_connections",
            Help: "Active connections by protocol",
        },
        []string{"protocol"},
    )
)

func init() {
    prometheus.MustRegister(activeConnections)
}

func main() {
    activeConnections.WithLabelValues("http").Set(10)
    activeConnections.WithLabelValues("grpc").Set(5)
    http.Handle("/metrics", promhttp.Handler())
    http.ListenAndServe(":8080", nil)
}
```

```go
// Pattern 8: Process and Go Runtime Metrics
package main

import (
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/collectors"
    "github.com/prometheus/client_golang/prometheus/promhttp"
    "net/http"
)

func main() {
    registry := prometheus.NewRegistry()
    registry.MustRegister(collectors.NewProcessCollector(collectors.ProcessCollectorOpts{}))
    registry.MustRegister(collectors.NewGoCollector())

    http.Handle("/metrics", promhttp.HandlerFor(registry, promhttp.HandlerOpts{}))
    http.ListenAndServe(":8080", nil)
}
```