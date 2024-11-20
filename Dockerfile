# Backend stage
FROM gcc:latest as backend
WORKDIR /app
COPY ./src /app/src
COPY ./CMakeLists.txt /app
RUN apt-get update && apt-get install -y cmake
RUN cmake . && make

# Frontend stage
FROM nginx:latest
WORKDIR /usr/share/nginx/html
COPY ./public . 

# Start nginx server
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
