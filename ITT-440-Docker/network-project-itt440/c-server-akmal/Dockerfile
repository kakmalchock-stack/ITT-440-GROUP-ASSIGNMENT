# /c-server/Dockerfile (FINAL MODIFICATION)
FROM ubuntu:latest

# Install tools for C compilation, pthreads, and ALL required MySQL client dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    net-tools \
    libpthread-stubs0-dev \
    # The crucial package is often libmariadb3 or libmysqlclient-dev
    mysql-client \
    libmariadb3 \
    # Ensure bash is available
    bash \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the server code
COPY server.c .

# Compile the C server code, linking with pthreads (-pthread)
RUN gcc -o server server.c -pthread

# Expose the internal port (for documentation)
EXPOSE 5005

# Run the server
# Use a simple direct execution
CMD ["./server"]