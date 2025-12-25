use serde::{Deserialize, Serialize};

/// Request to set servo angle
#[derive(Debug, Deserialize)]
pub struct SetAngleRequest {
    pub angle: u8,
}

/// Request to set servo PWM pulse width
#[derive(Debug, Deserialize)]
pub struct SetPwmRequest {
    pub pulse_us: u16,
}

/// Request to execute POSE command
#[derive(Debug, Deserialize)]
pub struct PoseRequest {
    pub angles: Vec<u8>,
}

/// Request to execute MOVE command
#[derive(Debug, Deserialize)]
pub struct MoveRequest {
    pub duration_ms: u16,
    pub angles: Vec<u8>,
}

/// Response for servo position query
#[derive(Debug, Serialize)]
pub struct ServoPosition {
    pub channel: u8,
    pub angle: u8,
}

/// Response for all servos query
#[derive(Debug, Serialize)]
pub struct ServoPositions {
    pub servos: Vec<ServoPosition>,
}

/// Generic success response
#[derive(Debug, Serialize)]
pub struct SuccessResponse {
    pub status: String,
}

/// Generic error response
#[derive(Debug, Serialize)]
pub struct ErrorResponse {
    pub error: String,
}

/// Health check response
#[derive(Debug, Serialize)]
pub struct HealthResponse {
    pub status: String,
    pub serial: String,
}
