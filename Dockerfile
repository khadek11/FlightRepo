# Backend build stage
FROM gcc:latest AS backend

# Set the working directory to a temporary location in the container
WORKDIR /app

# Copy the entire project into the Docker image
COPY . .

# Install build dependencies and git
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    curl \
    unzip \
    && rm -rf /var/lib/apt/lists/*
# Clone vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg

# Bootstrap vcpkg
RUN /vcpkg/bootstrap-vcpkg.sh

# Set environment variable for vcpkg
ENV VCPKG_ROOT=/vcpkg
ENV PATH="$VCPKG_ROOT:$PATH"

# Install required libraries using vcpkg
RUN /vcpkg/vcpkg install unofficial-sqlite3:x64-linux nlohmann-json:x64-linux

# Configure CMake with vcpkg toolchain
RUN cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug .

# Build the project
RUN make

# Final stage
FROM nginx:alpine

# Set the working directory for Nginx
WORKDIR /usr/share/nginx/html

# Copy backend executable from the build stage
COPY --from=backend /app/build/bin/Debug/flight_booking /usr/local/bin/

# Copy frontend assets from the build stage
COPY --from=backend /app/public .

# Create custom Nginx configuration
RUN echo "server { listen 80; location / { index register.html; } }" > /etc/nginx/conf.d/default.conf

# Expose port 80 for the web server
EXPOSE 80

# Start Nginx and the backend executable
CMD ["sh", "-c", "nginx -g 'daemon off;' & /usr/local/bin/flight_booking"]