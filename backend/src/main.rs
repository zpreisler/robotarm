mod handlers;
mod models;
mod serial;

use axum::{
    routing::{get, post},
    Router,
};
use handlers::AppState;
use serial::SerialManager;
use std::env;
use std::sync::Arc;
use tower_http::cors::{Any, CorsLayer};
use tracing::info;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

#[tokio::main]
async fn main() {
    // Initialize tracing
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "robotarm_backend=debug,tower_http=debug".into()),
        )
        .with(tracing_subscriber::fmt::layer())
        .init();

    // Get configuration from environment
    let serial_port = env::var("SERIAL_PORT").unwrap_or_else(|_| "/dev/ttyUSB0".to_string());
    let serial_baud: u32 = env::var("SERIAL_BAUD")
        .unwrap_or_else(|_| "115200".to_string())
        .parse()
        .expect("SERIAL_BAUD must be a number");
    let bind_addr = env::var("BIND_ADDR").unwrap_or_else(|_| "0.0.0.0:3000".to_string());

    info!("Starting robot arm backend");
    info!("Serial port: {} @ {} baud", serial_port, serial_baud);

    // Try initial connection (non-blocking)
    let initial_serial = match SerialManager::new(&serial_port, serial_baud) {
        Ok(manager) => {
            info!("Serial connection established");
            Some(Arc::new(manager))
        }
        Err(e) => {
            tracing::warn!("Serial device not available at startup: {}", e);
            tracing::warn!("Will retry connection in background");
            None
        }
    };

    // Create shared state
    let state = Arc::new(AppState {
        serial: Arc::new(std::sync::Mutex::new(initial_serial)),
        serial_port_name: serial_port.clone(),
        serial_baud_rate: serial_baud,
    });

    // Background task for automatic reconnection
    let reconnect_state = state.clone();
    let reconnect_port = serial_port.clone();
    let reconnect_baud = serial_baud;

    tokio::spawn(async move {
        use std::time::Duration;
        use tracing::debug;

        let mut interval = tokio::time::interval(Duration::from_secs(5));
        interval.tick().await; // Skip first immediate tick

        loop {
            interval.tick().await;

            // Check if we need to reconnect
            let needs_connection = {
                let serial = reconnect_state.serial.lock().unwrap();
                serial.is_none()
            };

            if needs_connection {
                debug!("Attempting to reconnect to serial device...");
                match SerialManager::new(&reconnect_port, reconnect_baud) {
                    Ok(manager) => {
                        info!("Serial connection re-established");
                        let mut serial = reconnect_state.serial.lock().unwrap();
                        *serial = Some(Arc::new(manager));
                    }
                    Err(e) => {
                        debug!("Reconnection failed: {}", e);
                    }
                }
            }
        }
    });

    info!("Background reconnection task started (checks every 5 seconds)");

    // Configure CORS
    let cors = CorsLayer::new()
        .allow_origin(Any)
        .allow_methods(Any)
        .allow_headers(Any);

    // Build router
    let app = Router::new()
        // Health check
        .route("/api/health", get(handlers::health_check))
        // Serial mode control
        .route("/api/serial/start", post(handlers::start_serial_mode))
        .route("/api/serial/stop", post(handlers::stop_serial_mode))
        // Single servo control
        .route("/api/servo/:id/angle", post(handlers::set_servo_angle))
        .route("/api/servo/:id/pwm", post(handlers::set_servo_pwm))
        .route("/api/servo/:id", get(handlers::get_servo_position))
        // Multi-servo commands
        .route("/api/pose", post(handlers::execute_pose))
        .route("/api/move", post(handlers::execute_move))
        // All servos query
        .route("/api/servos", get(handlers::get_all_servos))
        .layer(cors)
        .with_state(state);

    // Start server
    let listener = tokio::net::TcpListener::bind(&bind_addr)
        .await
        .expect("Failed to bind to address");

    info!("Server listening on {}", bind_addr);
    info!("API endpoints:");
    info!("  GET  /api/health");
    info!("  POST /api/serial/start");
    info!("  POST /api/serial/stop");
    info!("  POST /api/servo/:id/angle");
    info!("  POST /api/servo/:id/pwm");
    info!("  GET  /api/servo/:id");
    info!("  POST /api/pose");
    info!("  POST /api/move");
    info!("  GET  /api/servos");

    axum::serve(listener, app)
        .await
        .expect("Failed to start server");
}
