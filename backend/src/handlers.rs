use axum::{
    extract::{Path, State},
    http::StatusCode,
    Json,
};
use std::sync::{Arc, Mutex};
use tracing::{error, warn};

use crate::models::*;
use crate::serial::SerialManager;

/// Shared application state
pub struct AppState {
    pub serial: Arc<Mutex<Option<Arc<SerialManager>>>>,
    pub serial_port_name: String,
    pub serial_baud_rate: u32,
}

impl AppState {
    fn get_serial(&self) -> Option<Arc<SerialManager>> {
        self.serial.lock().unwrap().clone()
    }
}

/// Handle serial errors and detect disconnections
fn handle_serial_error(
    state: &AppState,
    error: &anyhow::Error,
) -> (StatusCode, Json<ErrorResponse>) {
    // If error indicates I/O failure, drop the serial manager
    let error_msg = error.to_string();
    if error_msg.contains("Failed to clear input buffer")
        || error_msg.contains("Failed to write")
        || error_msg.contains("Failed to read")
    {
        warn!(
            "Serial I/O error detected, dropping connection for reconnection: {}",
            error
        );
        let mut serial = state.serial.lock().unwrap();
        *serial = None;
        (
            StatusCode::SERVICE_UNAVAILABLE,
            Json(ErrorResponse {
                error: "Serial device disconnected, reconnecting...".to_string(),
            }),
        )
    } else {
        (
            StatusCode::BAD_REQUEST,
            Json(ErrorResponse {
                error: error.to_string(),
            }),
        )
    }
}

/// Health check endpoint
pub async fn health_check(State(state): State<Arc<AppState>>) -> Json<HealthResponse> {
    let serial_status = match state.get_serial() {
        Some(_) => "connected".to_string(),
        None => "not_connected".to_string(),
    };

    let overall_status = if serial_status == "connected" {
        "ok".to_string()
    } else {
        "degraded".to_string()
    };

    Json(HealthResponse {
        status: overall_status,
        serial: serial_status,
    })
}

/// Enter serial mode
pub async fn start_serial_mode(
    State(state): State<Arc<AppState>>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.start_serial_mode() {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "serial_mode".to_string(),
        })),
        Err(e) => {
            error!("Failed to start serial mode: {}", e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Exit serial mode
pub async fn stop_serial_mode(
    State(state): State<Arc<AppState>>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.stop_serial_mode() {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "button_mode".to_string(),
        })),
        Err(e) => {
            error!("Failed to stop serial mode: {}", e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Set servo angle
pub async fn set_servo_angle(
    State(state): State<Arc<AppState>>,
    Path(id): Path<u8>,
    Json(req): Json<SetAngleRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.set_servo_angle(id, req.angle) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to set servo {} angle: {}", id, e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Set servo PWM pulse width
pub async fn set_servo_pwm(
    State(state): State<Arc<AppState>>,
    Path(id): Path<u8>,
    Json(req): Json<SetPwmRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.set_servo_pwm(id, req.pulse_us) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to set servo {} PWM: {}", id, e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Get servo position
pub async fn get_servo_position(
    State(state): State<Arc<AppState>>,
    Path(id): Path<u8>,
) -> Result<Json<ServoPosition>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.get_servo_angle(id) {
        Ok(angle) => Ok(Json(ServoPosition {
            channel: id,
            angle,
        })),
        Err(e) => {
            error!("Failed to get servo {} position: {}", id, e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Get all servo positions
pub async fn get_all_servos(
    State(state): State<Arc<AppState>>,
) -> Result<Json<ServoPositions>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.get_all_servos() {
        Ok(servos) => {
            let positions = servos
                .into_iter()
                .map(|(channel, angle)| ServoPosition { channel, angle })
                .collect();
            Ok(Json(ServoPositions { servos: positions }))
        }
        Err(e) => {
            error!("Failed to get all servos: {}", e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Execute POSE command
pub async fn execute_pose(
    State(state): State<Arc<AppState>>,
    Json(req): Json<PoseRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.execute_pose(&req.angles) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to execute POSE: {}", e);
            Err(handle_serial_error(&state, &e))
        }
    }
}

/// Execute MOVE command
pub async fn execute_move(
    State(state): State<Arc<AppState>>,
    Json(req): Json<MoveRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    let serial = match state.get_serial() {
        Some(s) => s,
        None => {
            return Err((
                StatusCode::SERVICE_UNAVAILABLE,
                Json(ErrorResponse {
                    error: "Serial device not connected".to_string(),
                }),
            ));
        }
    };

    match serial.execute_move(req.duration_ms, &req.angles) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to execute MOVE: {}", e);
            Err(handle_serial_error(&state, &e))
        }
    }
}
