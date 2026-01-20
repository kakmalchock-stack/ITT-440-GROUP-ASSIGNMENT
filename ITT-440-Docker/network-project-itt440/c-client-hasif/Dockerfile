FROM ubuntu:latest

# Install tools for C compilation
RUN apt-get update && apt-get install -y \
    gcc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the client code
COPY client.c .

# Compile the C client code
RUN gcc -o client client.c

# Command is run in docker-compose.yml: CMD ["./client"]