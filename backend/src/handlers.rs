use axum::{
    extract::{Path, State},
    http::StatusCode,
    Json,
};
use std::sync::Arc;
use tracing::error;

use crate::models::*;
use crate::serial::SerialManager;

/// Shared application state
pub struct AppState {
    pub serial: Arc<SerialManager>,
}

/// Health check endpoint
pub async fn health_check(State(_state): State<Arc<AppState>>) -> Json<HealthResponse> {
    Json(HealthResponse {
        status: "ok".to_string(),
        serial: "connected".to_string(),
    })
}

/// Enter serial mode
pub async fn start_serial_mode(
    State(state): State<Arc<AppState>>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.start_serial_mode() {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "serial_mode".to_string(),
        })),
        Err(e) => {
            error!("Failed to start serial mode: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to start serial mode: {}", e),
                }),
            ))
        }
    }
}

/// Exit serial mode
pub async fn stop_serial_mode(
    State(state): State<Arc<AppState>>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.stop_serial_mode() {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "button_mode".to_string(),
        })),
        Err(e) => {
            error!("Failed to stop serial mode: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to stop serial mode: {}", e),
                }),
            ))
        }
    }
}

/// Set servo angle
pub async fn set_servo_angle(
    State(state): State<Arc<AppState>>,
    Path(id): Path<u8>,
    Json(req): Json<SetAngleRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.set_servo_angle(id, req.angle) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to set servo {} angle: {}", id, e);
            Err((
                StatusCode::BAD_REQUEST,
                Json(ErrorResponse {
                    error: format!("Failed to set servo angle: {}", e),
                }),
            ))
        }
    }
}

/// Set servo PWM pulse width
pub async fn set_servo_pwm(
    State(state): State<Arc<AppState>>,
    Path(id): Path<u8>,
    Json(req): Json<SetPwmRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.set_servo_pwm(id, req.pulse_us) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to set servo {} PWM: {}", id, e);
            Err((
                StatusCode::BAD_REQUEST,
                Json(ErrorResponse {
                    error: format!("Failed to set servo PWM: {}", e),
                }),
            ))
        }
    }
}

/// Get servo position
pub async fn get_servo_position(
    State(state): State<Arc<AppState>>,
    Path(id): Path<u8>,
) -> Result<Json<ServoPosition>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.get_servo_angle(id) {
        Ok(angle) => Ok(Json(ServoPosition {
            channel: id,
            angle,
        })),
        Err(e) => {
            error!("Failed to get servo {} position: {}", id, e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to get servo position: {}", e),
                }),
            ))
        }
    }
}

/// Get all servo positions
pub async fn get_all_servos(
    State(state): State<Arc<AppState>>,
) -> Result<Json<ServoPositions>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.get_all_servos() {
        Ok(servos) => {
            let positions = servos
                .into_iter()
                .map(|(channel, angle)| ServoPosition { channel, angle })
                .collect();
            Ok(Json(ServoPositions { servos: positions }))
        }
        Err(e) => {
            error!("Failed to get all servos: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to get servo positions: {}", e),
                }),
            ))
        }
    }
}

/// Execute POSE command
pub async fn execute_pose(
    State(state): State<Arc<AppState>>,
    Json(req): Json<PoseRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.execute_pose(&req.angles) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to execute POSE: {}", e);
            Err((
                StatusCode::BAD_REQUEST,
                Json(ErrorResponse {
                    error: format!("Failed to execute POSE: {}", e),
                }),
            ))
        }
    }
}

/// Execute MOVE command
pub async fn execute_move(
    State(state): State<Arc<AppState>>,
    Json(req): Json<MoveRequest>,
) -> Result<Json<SuccessResponse>, (StatusCode, Json<ErrorResponse>)> {
    match state.serial.execute_move(req.duration_ms, &req.angles) {
        Ok(_) => Ok(Json(SuccessResponse {
            status: "ok".to_string(),
        })),
        Err(e) => {
            error!("Failed to execute MOVE: {}", e);
            Err((
                StatusCode::BAD_REQUEST,
                Json(ErrorResponse {
                    error: format!("Failed to execute MOVE: {}", e),
                }),
            ))
        }
    }
}
