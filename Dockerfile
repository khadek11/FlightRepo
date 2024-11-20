# Backend build stage
FROM gcc:latest AS backend
WORKDIR /app
COPY ./build /app/build
COPY ./CMakeLists.txt /app
COPY ./src /app/src

# Final stage
FROM nginx:alpine
WORKDIR /usr/share/nginx/html

# Copy backend executable
COPY --from=backend /app/build/bin/Debug/flight-booking.exe /usr/local/bin/

# Copy frontend assets
COPY ./public .

# Expose port
EXPOSE 80

# Start nginx and executable
CMD ["sh", "-c", "nginx -g 'daemon off;' & /usr/local/bin/flight-booking.exe"]