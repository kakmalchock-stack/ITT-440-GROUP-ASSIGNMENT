FROM python:3.10-slim

WORKDIR /app

# Copy client code
COPY client.py .

# Expose the internal port (for documentation)
EXPOSE 5002

# Command is run in docker-compose.yml: CMD ["python", "client.py"]