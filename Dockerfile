# Backend build stage
FROM gcc:latest AS backend
WORKDIR /app
COPY . /app

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libstdc++-11-dev \
    && rm -rf /var/lib/apt/lists/*

# Try to configure CMake with verbose output
RUN cmake -DCMAKE_BUILD_TYPE=Debug . || cat CMakeFiles/CMakeError.log

# Build the project
RUN make || cat CMakeFiles/CMakeOutput.log

# Final stage
FROM nginx:alpine
WORKDIR /usr/share/nginx/html

# Copy backend executable
COPY --from=backend /app/build/bin/Debug/flight-booking.exe /usr/local/bin/

# Copy frontend assets
COPY --from=backend /app/public .

# Create custom Nginx configuration
RUN echo "server { listen 80; location / { index register.html; } }" > /etc/nginx/conf.d/default.conf

# Expose port
EXPOSE 80

# Start nginx and executable
CMD ["sh", "-c", "nginx -g 'daemon off;' & /usr/local/bin/flight-booking.exe"]