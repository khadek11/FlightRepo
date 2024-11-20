#include <httplib.h>
#include <sqlite3.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <nlohmann/json.hpp>
#include <vector>
#include <ctime>
#include <sstream>
#include <filesystem>

using namespace std;
using json = nlohmann::json;

class PasswordHasher
{
public:
    static std::string generateSalt(size_t length = 32)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        for (size_t i = 0; i < length; ++i)
        {
            ss << std::hex << dis(gen);
        }
        return ss.str();
    }

    static std::string hashPassword(const std::string &password, const std::string &salt)
    {
        std::stringstream ss;
        for (char c : password + salt)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)(c);
        }
        return ss.str();
    }
};

class UserRegistrationSystem
{
private:
    sqlite3 *db;

    void initDatabase()
    {
        const char *createTableSQL =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "email TEXT UNIQUE NOT NULL,"
            "password_hash TEXT NOT NULL,"
            "salt TEXT NOT NULL,"
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");";

        char *errMsg = nullptr;
        int rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            std::string error = "SQL error: ";
            error += errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }
    }

public:
    UserRegistrationSystem()
    {
        int rc = sqlite3_open("users.db", &db);
        if (rc)
        {
            throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
        }
        initDatabase();
    }

    bool registerUser(const std::string &name, const std::string &email, const std::string &password)
    {
        try
        {
            if (name.empty() || email.empty() || password.empty())
            {
                std::cout << "Error: All fields are required\n";
                return false;
            }

            std::string salt = PasswordHasher::generateSalt();
            std::string hashedPassword = PasswordHasher::hashPassword(password, salt);

            const char *insertSQL =
                "INSERT INTO users (name, email, password_hash, salt) VALUES (?, ?, ?, ?);";

            sqlite3_stmt *stmt;
            int rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
            if (rc != SQLITE_OK)
            {
                throw std::runtime_error("Failed to prepare statement");
            }

            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, hashedPassword.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, salt.c_str(), -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            if (rc == SQLITE_CONSTRAINT)
            {
                std::cout << "Error: Email already exists\n";
                return false;
            }
            else if (rc != SQLITE_DONE)
            {
                throw std::runtime_error("Failed to insert user");
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool loginUser(const std::string &email, const std::string &password)
    {
        try
        {
            const char *selectSQL =
                "SELECT password_hash, salt FROM users WHERE email = ?;";

            sqlite3_stmt *stmt;
            int rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
            if (rc != SQLITE_OK)
            {
                throw std::runtime_error("Failed to prepare statement");
            }

            sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);

            if (rc == SQLITE_ROW)
            {
                std::string storedHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
                std::string storedSalt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

                std::string inputHash = PasswordHasher::hashPassword(password, storedSalt);

                sqlite3_finalize(stmt);
                return (storedHash == inputHash);
            }

            sqlite3_finalize(stmt);
            return false;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    ~UserRegistrationSystem()
    {
        if (db)
        {
            sqlite3_close(db);
        }
    }
};

class Database
{
private:
    sqlite3 *db;

public:
    Database()
    {
        int rc = sqlite3_open("flights.db", &db);
        if (rc)
        {
            cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
            return;
        }
        else
        {
            cout << "Database opened successfully.\n"
                 << flush;
        }

        initializeTables();
    }

    ~Database()
    {
        sqlite3_close(db);
    }

    void initializeTables()
    {
        const char *flights_sql =
            "CREATE TABLE IF NOT EXISTS flights ("
            "flight_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "flight_number TEXT NOT NULL,"
            "destination TEXT NOT NULL,"
            "departure_date TEXT NOT NULL,"
            "total_seats INTEGER NOT NULL,"
            "class_type TEXT NOT NULL,"
            "price REAL NOT NULL);";

        const char *bookings_sql =
            "CREATE TABLE IF NOT EXISTS bookings ("
            "booking_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "flight_id INTEGER,"
            "passenger_name TEXT NOT NULL,"
            "passenger_email TEXT NOT NULL,"
            "seat_number INTEGER NOT NULL,"
            "booking_date TEXT NOT NULL,"
            "status TEXT NOT NULL,"
            "FOREIGN KEY(flight_id) REFERENCES flights(flight_id));";

        char *errMsg = 0;
        int rc = sqlite3_exec(db, flights_sql, 0, 0, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
        }

        rc = sqlite3_exec(db, bookings_sql, 0, 0, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
        }
    }

    bool addFlight(const string &flightNumber, const string &destination,
                   const string &departureDate, int totalSeats,
                   const string &classType, double price)
    {
        string sql = "INSERT INTO flights (flight_number, destination, departure_date, "
                     "total_seats, class_type, price) VALUES (?, ?, ?, ?, ?, ?);";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        if (rc != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_text(stmt, 1, flightNumber.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, destination.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, departureDate.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, totalSeats);
        sqlite3_bind_text(stmt, 5, classType.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 6, price);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return rc == SQLITE_DONE;
    }

    bool bookSeat(int flightId, const string &passengerName,
                  const string &passengerEmail, int seatNumber)
    {
        // First check if seat is available
        if (!isSeatAvailable(flightId, seatNumber))
        {
            return false;
        }

        time_t now = time(0);
        string bookingDate = ctime(&now);
        bookingDate = bookingDate.substr(0, bookingDate.length() - 1); // Remove newline

        string sql = "INSERT INTO bookings (flight_id, passenger_name, passenger_email, "
                     "seat_number, booking_date, status) VALUES (?, ?, ?, ?, ?, 'CONFIRMED');";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        if (rc != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_int(stmt, 1, flightId);
        sqlite3_bind_text(stmt, 2, passengerName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, passengerEmail.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, seatNumber);
        sqlite3_bind_text(stmt, 5, bookingDate.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return rc == SQLITE_DONE;
    }

    bool rescheduleBooking(int bookingId, int newFlightId, const string &newDate)
    {
        string sql = "UPDATE bookings SET flight_id = ?, status = 'RESCHEDULED' "
                     "WHERE booking_id = ?;";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        if (rc != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_int(stmt, 1, newFlightId);
        sqlite3_bind_int(stmt, 2, bookingId);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return rc == SQLITE_DONE;
    }

    bool cancelBooking(int bookingId) {
    string sql = "UPDATE bookings SET status = 'CANCELLED' WHERE booking_id = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, bookingId);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

    // Add this method to the Database class
    vector<json> getBookedFlights(const string &email = "")
    {
        vector<json> bookings;
        string sql;

        if (email.empty())
        {
            sql = "SELECT b.booking_id, b.passenger_name, b.passenger_email, "
                  "b.seat_number, b.booking_date, b.status, "
                  "f.flight_number, f.destination, f.departure_date, f.class_type, f.price "
                  "FROM bookings b "
                  "JOIN flights f ON b.flight_id = f.flight_id "
                  "WHERE b.status != 'CANCELLED' "
                  "ORDER BY b.booking_date DESC;";
        }
        else
        {
            sql = "SELECT b.booking_id, b.passenger_name, b.passenger_email, "
                  "b.seat_number, b.booking_date, b.status, "
                  "f.flight_number, f.destination, f.departure_date, f.class_type, f.price "
                  "FROM bookings b "
                  "JOIN flights f ON b.flight_id = f.flight_id "
                  "WHERE b.passenger_email = ? AND b.status != 'CANCELLED' "
                  "ORDER BY b.booking_date DESC;";
        }

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        if (!email.empty())
        {
            sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            json booking = {
                {"booking_id", sqlite3_column_int(stmt, 0)},
                {"passenger_name", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1))},
                {"passenger_email", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))},
                {"seat_number", sqlite3_column_int(stmt, 3)},
                {"booking_date", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))},
                {"status", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5))},
                {"flight_number", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6))},
                {"destination", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7))},
                {"departure_date", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8))},
                {"class_type", reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9))},
                {"price", sqlite3_column_double(stmt, 10)}};
            bookings.push_back(booking);
        }

        sqlite3_finalize(stmt);
        return bookings;
    }

    vector<json> getAvailableFlights()
    {
        vector<json> flights;
        string sql = "SELECT flight_id, flight_number, destination, departure_date, "
                     "class_type, price, total_seats FROM flights WHERE total_seats > "
                     "(SELECT COUNT(*) FROM bookings WHERE flight_id = flights.flight_id "
                     "AND status = 'CONFIRMED');";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int flightId = sqlite3_column_int(stmt, 0);
            string flightNumber = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            string destination = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            string departureDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            string classType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            double price = sqlite3_column_double(stmt, 5);
            int totalSeats = sqlite3_column_int(stmt, 6);

            // Get the count of booked seats
            int bookedSeats = getBookedSeatsCount(flightId);

            json flightObj = {
                {"flight_id", flightId},
                {"flight_number", flightNumber},
                {"destination", destination},
                {"departure_date", departureDate},
                {"class_type", classType},
                {"price", price},
                {"available_seats", totalSeats - bookedSeats}};
            flights.push_back(flightObj);
        }

        sqlite3_finalize(stmt);
        return flights;
    }

    int getBookedSeatsCount(int flightId)
    {
        string sql = "SELECT COUNT(*) FROM bookings WHERE flight_id = ? AND status = 'CONFIRMED';";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, flightId);
        rc = sqlite3_step(stmt);
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return count;
    }

    vector<int> getAvailableSeats(int flightId)
    {
        vector<int> seats;
        string sql = "SELECT seat_number FROM bookings WHERE flight_id = ? "
                     "AND status = 'CONFIRMED';";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        sqlite3_bind_int(stmt, 1, flightId);

        vector<bool> takenSeats(100, false); // Assuming max 100 seats
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int seatNum = sqlite3_column_int(stmt, 0);
            takenSeats[seatNum - 1] = true;
        }

        for (int i = 0; i < 100; i++)
        {
            if (!takenSeats[i])
            {
                seats.push_back(i + 1);
            }
        }

        sqlite3_finalize(stmt);
        return seats;
    }

private:
    bool isSeatAvailable(int flightId, int seatNumber)
    {
        string sql = "SELECT COUNT(*) FROM bookings WHERE flight_id = ? "
                     "AND seat_number = ? AND status = 'CONFIRMED';";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);

        sqlite3_bind_int(stmt, 1, flightId);
        sqlite3_bind_int(stmt, 2, seatNumber);

        rc = sqlite3_step(stmt);
        int count = sqlite3_column_int(stmt, 0);

        sqlite3_finalize(stmt);
        return count == 0;
    }
};

class FlightBookingSystem
{
private:
    Database db;

public:
    bool addFlight(const string &flightNumber, const string &destination,
                   const string &departureDate, int totalSeats,
                   const string &classType, double price)
    {
        return db.addFlight(flightNumber, destination, departureDate,
                            totalSeats, classType, price);
    }

    bool bookSeat(int flightId, const string &passengerName,
                  const string &passengerEmail, int seatNumber)
    {
        return db.bookSeat(flightId, passengerName, passengerEmail, seatNumber);
    }

    bool rescheduleBooking(int bookingId, int newFlightId, const string &newDate)
    {
        return db.rescheduleBooking(bookingId, newFlightId, newDate);
    }

    bool cancelBooking(int bookingId) {
    return db.cancelBooking(bookingId);
    }
    vector<nlohmann::json> getAvailableFlights()
    {
        return db.getAvailableFlights();
    }

    vector<int> getAvailableSeats(int flightId)
    {
        return db.getAvailableSeats(flightId);
    }
    vector<json> getBookedFlights(const string& email = "") {
    return db.getBookedFlights(email);
}

    
};
class CombinedServer
{
private:
    UserRegistrationSystem registrationSystem;
    FlightBookingSystem bookingSystem;
    httplib::Server server;

public:
    CombinedServer()
    {
        // Set up static file handling
        server.set_mount_point("/", "./public");

        // Handle CORS for all endpoints
        server.Options("/.*", [](const httplib::Request &req, httplib::Response &res)
                       {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type"); });

        // Registration endpoints
        setupRegistrationEndpoints();

        // Flight booking endpoints
        setupFlightBookingEndpoints();
    }

    void setupRegistrationEndpoints()
    {
        server.Post("/register", [this](const httplib::Request &req, httplib::Response &res)
                    {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");

            try {
                auto requestJson = json::parse(req.body);
                
                std::string name = requestJson["name"];
                std::string email = requestJson["email"];
                std::string password = requestJson["password"];

                if (registrationSystem.registerUser(name, email, password)) {
                    json response = {
                        {"success", true},
                        {"message", "Registration successful"}
                    };
                    res.status = 200;
                    res.body = response.dump();
                } else {
                    json response = {
                        {"success", false},
                        {"message", "Registration failed"}
                    };
                    res.status = 400;
                    res.body = response.dump();
                }
            } catch (const std::exception& e) {
                json response = {
                    {"success", false},
                    {"message", std::string("Error: ") + e.what()}
                };
                res.status = 500;
                res.body = response.dump();
            } });

            

        server.Post("/login", [this](const httplib::Request &req, httplib::Response &res)
                    {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");

            try {
                auto requestJson = json::parse(req.body);
                
                std::string email = requestJson["email"];
                std::string password = requestJson["password"];

                if (registrationSystem.loginUser(email, password)) {
                    json response = {
                        {"success", true},
                        {"message", "Login successful"}
                    };
                    res.status = 200;
                    res.body = response.dump();
                } else {
                    json response = {
                        {"success", false},
                        {"message", "Invalid email or password"}
                    };
                    res.status = 401;
                    res.body = response.dump();
                }
            } catch (const std::exception& e) {
                json response = {
                    {"success", false},
                    {"message", std::string("Error: ") + e.what()}
                };
                res.status = 500;
                res.body = response.dump();
            } });
    }

    void setupFlightBookingEndpoints()
    {
        // GET /api/flights - Get all available flights
        server.Get("/api/flights", [this](const httplib::Request &req, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        try {
            auto flights = bookingSystem.getAvailableFlights(); // This now returns complete flight details
            res.status = 200;
            res.body = json(flights).dump(); // Directly serialize the vector of JSON objects
        } catch (const std::exception& e) {
            json error = {
                {"error", std::string("Error: ") + e.what()}
            };
            res.status = 500;
            res.body = error.dump();
        } });

        server.Post("/api/flights", [this](const httplib::Request &req, httplib::Response &res)
                    {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");

            try {
                auto requestJson = json::parse(req.body);
                
                bool success = bookingSystem.addFlight(
                    requestJson["flight_number"],
                    requestJson["destination"],
                    requestJson["departure_date"],
                    requestJson["total_seats"],
                    requestJson["class_type"],
                    requestJson["price"]
                );

                if (success) {
                    json response = {
                        {"success", true},
                        {"message", "Flight added successfully"}
                    };
                    res.status = 200;
                    res.body = response.dump();
                } else {
                    json response = {
                        {"success", false},
                        {"message", "Failed to add flight"}
                    };
                    res.status = 400;
                    res.body = response.dump();
                }
            } catch (const std::exception& e) {
                json error = {
                    {"error", std::string("Error: ") + e.what()}
                };
                res.status = 500;
                res.body = error.dump();
            } });

        // GET /api/flights/{id}/seats - Get available seats for a flight
        server.Get(R"(/api/flights/(\d+)/seats)", [this](const httplib::Request &req, httplib::Response &res)
                   {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");

            try {
                int flightId = std::stoi(req.matches[1]);
                auto seats = bookingSystem.getAvailableSeats(flightId);
                
                json response = json::array();
                for (const auto& seat : seats) {
                    response.push_back(seat);
                }
                
                res.status = 200;
                res.body = response.dump();
            } catch (const std::exception& e) {
                json error = {
                    {"error", std::string("Error: ") + e.what()}
                };
                res.status = 500;
                res.body = error.dump();
            } });

        server.Post("/api/bookings", [this](const httplib::Request &req, httplib::Response &res)
                    {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        try {
            auto requestJson = json::parse(req.body);
            
            int flightId = requestJson["flight_id"];
            std::string passengerName = requestJson["passenger_name"];
            std::string passengerEmail = requestJson["passenger_email"];
            int seatNumber = requestJson["seat_number"];

            bool success = bookingSystem.bookSeat(
                flightId,
                passengerName,
                passengerEmail,
                seatNumber
            );

            if (success) {
                json response = {
                    {"success", true},
                    {"message", "Flight booked successfully"}
                };
                res.status = 200;
                res.body = response.dump();
            } else {
                json response = {
                    {"success", false},
                    {"message", "Failed to book flight. Seat may no longer be available."}
                };
                res.status = 400;
                res.body = response.dump();
            }
        } catch (const std::exception& e) {
            json error = {
                {"success", false},
                {"message", std::string("Error: ") + e.what()}
            };
            res.status = 500;
            res.body = error.dump();
        } });

        // Add this inside setupFlightBookingEndpoints()
           server.Get("/api/bookings", [this](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");

            try {
                string email = "";
                if (req.has_param("email")) {
                    email = req.get_param_value("email");
                }
                
                auto bookings = bookingSystem.getBookedFlights(email);  // Use bookingSystem instead of db
                res.status = 200;
                res.body = json(bookings).dump();
            } catch (const std::exception& e) {
                json error = {
                    {"error", std::string("Error: ") + e.what()}
                };
                res.status = 500;
                res.body = error.dump();
            }
});

server.Delete(R"(/api/bookings/(\d+))", [this](const httplib::Request &req, httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Content-Type", "application/json");

    try {
        int bookingId = std::stoi(req.matches[1]);
        bool success = bookingSystem.cancelBooking(bookingId);

        if (success) {
            json response = {
                {"success", true},
                {"message", "Booking cancelled successfully"}
            };
            res.status = 200;
            res.body = response.dump();
        } else {
            json response = {
                {"success", false},
                {"message", "Failed to cancel booking"}
            };
            res.status = 400;
            res.body = response.dump();
        }
    } catch (const std::exception &e) {
        json error = {
            {"error", std::string("Error: ") + e.what()}
        };
        res.status = 500;
        res.body = error.dump();
    }
});

        // PUT /api/bookings/reschedule - Reschedule a booking
        server.Put("/api/bookings/reschedule", [this](const httplib::Request &req, httplib::Response &res)
                   {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        try {
            auto requestJson = json::parse(req.body);
            
            int bookingId = requestJson["booking_id"];
            int newFlightId = requestJson["new_flight_id"];
            std::string newDate = requestJson["new_date"];

            bool success = bookingSystem.rescheduleBooking(bookingId, newFlightId, newDate);

            if (success) {
                json response = {
                    {"success", true},
                    {"message", "Flight rescheduled successfully"}
                };
                res.status = 200;
                res.body = response.dump();
            } else {
                json response = {
                    {"success", false},
                    {"message", "Failed to reschedule flight"}
                };
                res.status = 400;
                res.body = response.dump();
            }
        } catch (const std::exception& e) {
            json error = {
                {"success", false},
                {"message", std::string("Error: ") + e.what()}
            };
            res.status = 500;
            res.body = error.dump();
        } });
    }

    void start(const char *host = "localhost", int port = 8080)
    {
        std::cout << "Server starting on " << host << ":" << port << std::endl;

        // Ensure the public directory exists
        if (!std::filesystem::exists("public"))
        {
            std::filesystem::create_directory("public");
        }

        // Copy your HTML, CSS, and JS files to public directory if they don't exist
        if (!std::filesystem::exists("public/flight.html"))
        {
            std::filesystem::copy_file("flight.html", "public/flight.html");
        }
        if (!std::filesystem::exists("public/js/flight.js"))
        {
            std::filesystem::copy_file("flight.js", "public/js/flight.js");
        }

        server.listen(host, port);
    }
};

int main()
{
    try
    {
        CombinedServer server;
        server.start();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
}