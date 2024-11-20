class FlightBookingSystem {
  constructor() {
    this.initializeEventListeners();
    this.loadAvailableFlights();
    
    // Add event listener for email input
    document.getElementById('passenger_email')?.addEventListener('change', (e) => {
        localStorage.setItem('userEmail', e.target.value);
        this.loadBookedFlights();
    });
  }

  initializeEventListeners() {
    // Booking form submission
    document.getElementById("bookingForm")?.addEventListener("submit", (e) => {
      e.preventDefault();
      this.handleBooking();
    });

    // Reschedule form submission
    document
      .getElementById("rescheduleForm")
      ?.addEventListener("submit", (e) => {
        e.preventDefault();
        this.handleReschedule();
      });

    // Add flight form submission
    document
      .getElementById("addFlightForm")
      ?.addEventListener("submit", (e) => {
        e.preventDefault();
        this.handleAddFlight();
      });

    // Flight selection change
    document.getElementById("flightSelect")?.addEventListener("change", (e) => {
      this.loadAvailableSeats(e.target.value);
    });
  }

  showSuccess(message) {
    const alertDiv = document.createElement("div");
    alertDiv.className = "alert alert-success alert-dismissible fade show";
    alertDiv.role = "alert";
    alertDiv.innerHTML = `
        ${message}
        <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
    `;

    // Use the correct form ID here
    const form = document.getElementById("bookingForm");
    if (form) {
        form.parentNode.insertBefore(alertDiv, form);
    } else {
        console.error("Booking form not found");
    }

    setTimeout(() => {
        alertDiv.remove();
    }, 3000);
}

showError(message) {
    const alertDiv = document.createElement("div");
    alertDiv.className = "alert alert-danger alert-dismissible fade show";
    alertDiv.role = "alert";
    alertDiv.innerHTML = `
        ${message}
        <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
    `;

    // Use the correct form ID here
    const form = document.getElementById("bookingForm");
    if (form) {
        form.parentNode.insertBefore(alertDiv, form);
    } else {
        console.error("Booking form not found");
    }

    setTimeout(() => {
        alertDiv.remove();
    }, 3000);
}
  async loadAvailableFlights() {
    try {
      const response = await fetch("/api/flights");
      const flights = await response.json();
      this.updateFlightLists(flights);
      this.updateFlightTable(flights);
    } catch (error) {
      console.error("Error loading flights:", error);
      this.showError("Failed to load available flights");
    }
  }

  updateFlightLists(flights) {
    const flightSelect = document.getElementById("flightSelect");
    const newFlightSelect = document.getElementById("newFlightSelect");

    if (flightSelect && newFlightSelect) {
      const options = flights
        .map(
          (flight) =>
            `<option value="${flight.flight_id}">${flight.flight_number} - ${flight.destination} - ${flight.departure_date}</option>`
        )
        .join("");

      flightSelect.innerHTML =
        '<option value="">Select Flight</option>' + options;
      newFlightSelect.innerHTML =
        '<option value="">Select New Flight</option>' + options;
    }
  }

  updateFlightTable(flights) {
    const flightList = document.getElementById("flightList");
    if (flightList) {
      const table = `
                <table class="table table-striped">
                    <thead>
                        <tr>
                            <th>Flight Number</th>
                            <th>Destination</th>
                            <th>Departure</th>
                            <th>Class</th>
                            <th>Price</th>
                            <th>Available Seats</th>
                        </tr>
                    </thead>
                    <tbody>
                        ${flights
                          .map(
                            (flight) => `
                            <tr>
                                <td>${flight.flight_number}</td>
                                <td>${flight.destination}</td>
                                <td>${flight.departure_date}</td>
                                <td>${flight.class_type}</td>
                                <td>$${flight.price}</td>
                                <td>${flight.available_seats}</td>
                            </tr>
                        `
                          )
                          .join("")}
                    </tbody>
                </table>
            `;
      flightList.innerHTML = table;
    }
  }

  async handleAddFlight() {
    const form = document.getElementById("addFlightForm");
    const formData = new FormData(form);

    // Create an object from the form data
    const flightData = {
      flight_number: formData.get("flight_number"),
      destination: formData.get("destination"),
      departure_date: formData.get("departure_date"),
      total_seats: parseInt(formData.get("total_seats")),
      class_type: formData.get("class_type"),
      price: parseFloat(formData.get("price")),
    };

    try {
      const response = await fetch("/api/flights", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(flightData),
      });

      if (response.ok) {
        this.showSuccess("Flight added successfully!");
        form.reset();
        await this.loadAvailableFlights();
      } else {
        const errorData = await response.json();
        this.showError(
          errorData.message || "Failed to add flight. Please try again."
        );
      }
    } catch (error) {
      console.error("Error adding flight:", error);
      this.showError(
        "Failed to add flight. Please check your connection and try again."
      );
    }
  }

  async loadAvailableSeats(flightId) {
    if (!flightId) return;

    try {
      const response = await fetch(`/api/flights/${flightId}/seats`);
      const seats = await response.json();
      const seatSelect = document.getElementById("seatSelect");

      if (seatSelect) {
        seatSelect.innerHTML =
          '<option value="">Select Seat</option>' +
          seats
            .map((seat) => `<option value="${seat}">${seat}</option>`)
            .join("");
      }
    } catch (error) {
      console.error("Error loading seats:", error);
      this.showError("Failed to load available seats");
    }
  }

  async loadBookedFlights() {
    try {
      const email = document.getElementById('passenger_email')?.value || localStorage.getItem('userEmail');
      if (!email) {
        this.showError('Please enter your email to view bookings');
        return;
      }

      const response = await fetch(`/api/bookings?email=${encodeURIComponent(email)}`);
      const bookings = await response.json();
      this.updateBookingsTable(bookings);
    } catch (error) {
      console.error('Error loading bookings:', error);
      this.showError('Failed to load bookings');
    }
  }

  updateBookingsTable(bookings) {
    const bookingsList = document.getElementById('bookingsList');
    if (bookingsList) {
      const table = `
            <table class="table table-striped">
                <thead style="background-color: #0ef; color: white;">
                    <tr>
                        <th>Booking ID</th>
                        <th>Flight</th>
                        <th>Destination</th>
                        <th>Departure</th>
                        <th>Seat</th>
                        <th>Status</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody>
                    ${bookings.map(booking => `
                        <tr>
                            <td>${booking.booking_id}</td>
                            <td>${booking.flight_number}</td>
                            <td>${booking.destination}</td>
                            <td>${booking.departure_date}</td>
                            <td>${booking.seat_number}</td>
                            <td>${booking.status}</td>
                            <td>
                                
                               <button class="btn btn-sm btn-danger" 
                                onclick="window.bookingSystem.deleteBooking(${booking.booking_id})">
                                Cancel
                            </button>
                            </td>
                        </tr>
                    `).join('')}
                </tbody>
            </table>
        `;
      bookingsList.innerHTML = table;
    }
  }

  async deleteBooking(bookingId) {
    if (!confirm('Are you sure you want to delete this booking?')) {
        return;
    }

    try {
        const response = await fetch(`/api/bookings/${bookingId}`, {
            method: 'DELETE',
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (response.ok) {
            this.showSuccess('Booking deleted successfully!');
            await this.loadBookedFlights();
            await this.loadAvailableFlights();
        } else {
            const errorData = await response.json();
            this.showError(errorData.message || 'Failed to delete booking. Please try again.');
        }
    } catch (error) {
        console.error('Error deleting booking:', error);
        this.showError('Failed to delete booking. Please check your connection and try again.');
    }
}

  prepareReschedule(bookingId) {
    // Auto-fill the booking ID in the reschedule form
    const bookingIdInput = document.getElementById('booking_id');
    if (bookingIdInput) {
      bookingIdInput.value = bookingId;
    }
    
    // Scroll to the reschedule form
    document.querySelector('#reschedule-flight')?.scrollIntoView({ behavior: 'smooth' });
  }

  async handleBooking() {
    const form = document.getElementById("bookingForm");
    if (!form) return;

    const formData = new FormData(form);
    const bookingData = {
      flight_id: parseInt(formData.get("flight_id")),
      passenger_name: formData.get("passenger_name"),
      passenger_email: formData.get("passenger_email"),
      seat_number: parseInt(formData.get("seat_number")),
    };

    // Validate the data
    if (
      !bookingData.flight_id ||
      !bookingData.passenger_name ||
      !bookingData.passenger_email ||
      !bookingData.seat_number
    ) {
      this.showError("Please fill in all required fields");
      return;
    }

    try {
      const response = await fetch("/api/bookings", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(bookingData),
      });

      const result = await response.json();

      if (response.ok) {
        this.showSuccess("Flight booked successfully!");
        form.reset();
        await this.loadAvailableFlights();
        await this.loadBookedFlights();   
      } else {
        this.showError(result.message || "Booking failed. Please try again.");
      }
    } catch (error) {
      console.error("Error booking flight:", error);
      this.showError(
        "Failed to book flight. Please check your connection and try again."
      );
    }
  }

  async handleReschedule() {
    const form = document.getElementById("rescheduleForm");
    if (!form) return;

    const formData = new FormData(form);
    const rescheduleData = {
      booking_id: parseInt(formData.get("booking_id")),
      new_flight_id: parseInt(formData.get("new_flight_id")),
      new_date: formData.get("new_date"),
    };

    // Validate the data
    if (
      !rescheduleData.booking_id ||
      !rescheduleData.new_flight_id ||
      !rescheduleData.new_date
    ) {
      this.showError("Please fill in all required fields for rescheduling");
      return;
    }

    try {
      const response = await fetch("/api/bookings/reschedule", {
        method: "PUT",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(rescheduleData),
      });

      const result = await response.json();

      if (response.ok) {
        this.showSuccess("Flight rescheduled successfully!");
        form.reset();
        await this.loadAvailableFlights();
        await this.loadBookedFlights();
      } else {
        this.showError(
          result.message || "Rescheduling failed. Please try again."
        );
      }
    } catch (error) {
      console.error("Error rescheduling flight:", error);
      this.showError(
        "Failed to reschedule flight. Please check your connection and try again."
      );
    }
  }
}

// Initialize the system when the DOM is loaded
document.addEventListener("DOMContentLoaded", () => {
  window.bookingSystem = new FlightBookingSystem();
});