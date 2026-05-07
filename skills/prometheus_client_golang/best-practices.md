# Best Practices

**Pattern 1: Use Custom Registries for Component Isolation**

```go
type ServiceMetrics struct {
    registry *prometheus.Registry
    requests *prometheus.CounterVec
}

func NewServiceMetrics() *ServiceMetrics {
    registry := prometheus.NewRegistry()
    requests := prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "service_requests_total",
            Help: "Total requests to the service",
        },
        []string{"endpoint"},
    )
    registry.MustRegister(requests)
    return &ServiceMetrics{
        registry: registry,
        requests: requests,
    }
}

func (sm *ServiceMetrics) Handler() http.Handler {
    return promhttp.HandlerFor(sm.registry, promhttp.HandlerOpts{})
}
```

**Pattern 2: Use Middleware for Automatic Instrumentation**

```go
func MetricsMiddleware(metrics *ServiceMetrics) func(http.Handler) http.Handler {
    return func(next http.Handler) http.Handler {
        return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
            metrics.requests.WithLabelValues(r.URL.Path).Inc()
            next.ServeHTTP(w, r)
        })
    }
}
```

**Pattern 3: Use Constants for Metric Names**

```go
const (
    MetricRequestsTotal = "http_requests_total"
    MetricRequestDuration = "http_request_duration_seconds"
)

var (
    requestsTotal = prometheus.NewCounter(prometheus.CounterOpts{
        Name: MetricRequestsTotal,
        Help: "Total number of HTTP requests",
    })
    requestDuration = prometheus.NewHistogram(prometheus.HistogramOpts{
        Name: MetricRequestDuration,
        Help: "Histogram of request durations",
    })
)
```

**Pattern 4: Use Labels for Multi-Dimensional Metrics**

```go
var httpRequestsTotal = prometheus.NewCounterVec(
    prometheus.CounterOpts{
        Name: "http_requests_total",
        Help: "Total number of HTTP requests",
    },
    []string{"method", "endpoint", "status_code"},
)
```

**Pattern 5: Use Histograms with Appropriate Buckets**

```go
var requestDuration = prometheus.NewHistogram(prometheus.HistogramOpts{
    Name:    "request_duration_seconds",
    Help:    "Request duration in seconds",
    Buckets: []float64{0.01, 0.05, 0.1, 0.5, 1, 5, 10},
})
```