# Overview

The Prometheus Go client library provides instrumentation primitives for exposing application metrics in a format that Prometheus can scrape. It includes counters, gauges, histograms, and summaries, along with HTTP handlers for serving metrics endpoints. The library also offers a client for querying the Prometheus HTTP API.

**When to use:**
- You need to monitor application performance and behavior in production
- You want to expose metrics in Prometheus format for scraping
- You need standard metric types like request counts, latency distributions, or resource usage

**When not to use:**
- For simple logging-based monitoring where metrics aren't needed
- When you need real-time alerting without a scraping infrastructure
- For applications that don't require historical metric data

**Key design principles:**
- Thread-safe metric collection with atomic operations
- Label-based metric identification for multi-dimensional data
- Custom registries for isolation between components
- Built-in process and Go runtime metric collectors
- HTTP handler for exposing metrics in Prometheus text format