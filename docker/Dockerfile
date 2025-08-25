# Multi-stage build for SIL-Harness Hydraulics Simulation System

# Stage 1: Build the C++ simulator
# Using debian:bookworm-slim with platform specification for Apple Silicon compatibility
FROM --platform=$BUILDPLATFORM debian:bookworm-slim AS builder

# Install build dependencies with proper GPG handling for multi-arch
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy C++ source code
COPY cpp_simulator/ ./cpp_simulator/
COPY config/ ./config/

# Build the C++ simulator
WORKDIR /app/cpp_simulator
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Stage 2: Final runtime image
# Using python:3.10-slim-bookworm with platform specification for Apple Silicon compatibility
FROM --platform=$BUILDPLATFORM python:3.10-slim-bookworm AS final

# Install runtime dependencies for Pygame (SDL libraries) with proper GPG handling
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    ca-certificates \
    iproute2 \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy built C++ simulator from builder stage
COPY --from=builder /app/cpp_simulator/build/simulator ./cpp_simulator/build/simulator
COPY --from=builder /app/config/ ./config/

# Copy Python client code
COPY python_client/ ./python_client/

# Install Python dependencies
RUN pip install --no-cache-dir -r python_client/requirements.txt

# Create non-root user for security
RUN useradd -m -u 1000 simuser && \
    chown -R simuser:simuser /app

USER simuser

# Set environment variables
ENV PYTHONPATH=/app/python_client
ENV DISPLAY=:0

# Expose UDP port
EXPOSE 8080/udp
EXPOSE 8081/udp

# Default command (can be overridden)
CMD ["./cpp_simulator/build/simulator", "./config/base_config.json"]
