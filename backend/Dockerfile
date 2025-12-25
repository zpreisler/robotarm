# Build stage
FROM rust:1.75 AS builder

WORKDIR /app

# Copy manifest files
COPY Cargo.toml ./

# Create a dummy main.rs to build dependencies
RUN mkdir src && \
    echo "fn main() {}" > src/main.rs && \
    cargo build --release && \
    rm -rf src

# Copy actual source code
COPY src ./src

# Build the application
RUN touch src/main.rs && \
    cargo build --release

# Runtime stage
FROM debian:bookworm-slim

# Install runtime dependencies for serial communication
RUN apt-get update && \
    apt-get install -y libudev-dev && \
    rm -rf /var/lib/apt/lists/*

# Copy the binary from builder
COPY --from=builder /app/target/release/robotarm-backend /usr/local/bin/robotarm-backend

# Set environment variables
ENV RUST_LOG=info
ENV SERIAL_PORT=/dev/ttyUSB0
ENV SERIAL_BAUD=115200
ENV BIND_ADDR=0.0.0.0:3000

# Expose port
EXPOSE 3000

# Run the application
CMD ["robotarm-backend"]
