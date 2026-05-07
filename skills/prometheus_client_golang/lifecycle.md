# Lifecycle

**Construction**

```go
// Create a counter with options
counter := prometheus.NewCounter(prometheus.CounterOpts{
    Name: "my_counter",
    Help: "A counter for tracking events",
})

// Create a counter vec with labels
counterVec := prometheus.NewCounterVec(
    prometheus.CounterOpts{
        Name: "my_counter_vec",
        Help: "A counter vec with labels",
    },
    []string{"label1", "label2"},
)
```

**Registration**

```go
// Register with default registry
prometheus.MustRegister(counter)

// Register with custom registry
registry := prometheus.NewRegistry()
registry.MustRegister(counter)
```

**Destruction**

```go
// Metrics cannot be unregistered from the default registry
// Use custom registries for lifecycle management
registry := prometheus.NewRegistry()
registry.MustRegister(counter)

// To remove metrics, create a new registry
newRegistry := prometheus.NewRegistry()
```

**Move Semantics**

```go
// Metrics are pointers, so they can be moved
type AppMetrics struct {
    Counter *prometheus.CounterVec
}

func NewAppMetrics() *AppMetrics {
    counter := prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "app_requests",
            Help: "App requests",
        },
        []string{"endpoint"},
    )
    prometheus.MustRegister(counter)
    return &AppMetrics{Counter: counter}
}

// Move the pointer
metrics := NewAppMetrics()
otherMetrics := metrics // Both point to same metric
```

**Resource Management**

```go
// Use sync.Once for safe initialization
var (
    once     sync.Once
    counter  *prometheus.CounterVec
)

func GetCounter() *prometheus.CounterVec {
    once.Do(func() {
        counter = prometheus.NewCounterVec(
            prometheus.CounterOpts{
                Name: "my_counter",
                Help: "My counter",
            },
            []string{"label"},
        )
        prometheus.MustRegister(counter)
    })
    return counter
}
```