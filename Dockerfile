# Backend build stage
FROM buildpack-deps:bullseye AS backend
WORKDIR /app

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Copy project files
COPY . /app

# Build the project
RUN cmake . && make

# Create index.html
RUN echo '<html><head><meta http-equiv="refresh" content="0;url=register.html"></head></html>' > /app/public/index.html

# Final stage
FROM nginx:alpine
WORKDIR /usr/share/nginx/html

# Copy backend executable
COPY --from=backend /app/build/bin/Debug/flight_booking.exe /usr/local/bin/flight_booking

# Copy frontend assets
COPY --from=backend /app/public/* /usr/share/nginx/html/

# Expose port
EXPOSE 80

# Start nginx and executable
CMD ["sh", "-c", "nginx -g 'daemon off;' & /usr/local/bin/flight_booking"]