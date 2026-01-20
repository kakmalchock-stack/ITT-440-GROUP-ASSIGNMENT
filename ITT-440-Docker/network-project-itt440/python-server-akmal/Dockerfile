# Use a slim Python image
FROM python:3.10-slim

WORKDIR /app

# Install dependencies (mysql-connector)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy server code
COPY server.py .

# Expose the internal port (for documentation)
EXPOSE 5006

# Run the server application
CMD ["python", "-u", "server.py"]