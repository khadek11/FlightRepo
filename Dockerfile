# Backend build stage
FROM gcc:latest AS backend
WORKDIR /app

# Copy project files
COPY CMakeLists.txt /app/
COPY src /app/src
COPY build /app/build
COPY public /app/public

# Build the project (if needed)
RUN cmake . && make

# Create index.html if not exists
RUN if [ ! -f /app/public/index.html ]; then \
    echo '<html><head><meta http-equiv="refresh" content="0;url=register.html"></head></html>' > /app/public/index.html; \
    fi

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