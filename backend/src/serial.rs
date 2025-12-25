use anyhow::{Context, Result};
use std::io::{Read, Write};
use std::sync::{Arc, Mutex};
use std::time::Duration;
use tokio_serial::SerialPort;
use tracing::{debug, error, info};

const NUM_SERVOS: u8 = 6;

/// Serial port manager for robot arm communication
pub struct SerialManager {
    port: Arc<Mutex<Box<dyn SerialPort>>>,
}

impl SerialManager {
    /// Open serial port and initialize connection
    pub fn new(port_name: &str, baud_rate: u32) -> Result<Self> {
        info!("Opening serial port {} at {} baud", port_name, baud_rate);

        let port = tokio_serial::new(port_name, baud_rate)
            .timeout(Duration::from_secs(12))
            .open()
            .context("Failed to open serial port")?;

        // Wait for port to stabilize after opening
        std::thread::sleep(Duration::from_millis(100));

        // Flush input buffer to discard startup message and any stale data
        port.clear(tokio_serial::ClearBuffer::Input)
            .context("Failed to clear input buffer")?;

        debug!("Input buffer cleared after opening port");

        // Additional flush: read and discard any remaining data
        std::thread::sleep(Duration::from_millis(500));
        port.clear(tokio_serial::ClearBuffer::Input)
            .context("Failed to clear input buffer (second flush)")?;

        debug!("Port initialization complete");

        Ok(Self {
            port: Arc::new(Mutex::new(port)),
        })
    }

    /// Send a command and read the response
    fn send_command(&self, cmd: &str) -> Result<String> {
        let mut port = self.port.lock().unwrap();

        debug!("Sending command: {:?}", cmd.trim());
        debug!("Sending bytes: {:?}", cmd.as_bytes());

        // Clear any stale data in the buffer before sending
        port.clear(tokio_serial::ClearBuffer::Input)
            .context("Failed to clear input buffer before sending")?;

        // Send command
        port.write_all(cmd.as_bytes())
            .context("Failed to write to serial port")?;
        port.flush()
            .context("Failed to flush serial port")?;

        // Give the AVR time to process and respond
        std::thread::sleep(std::time::Duration::from_millis(200));

        // Read response - use the port directly, not a clone
        let mut response = Vec::new();
        let mut buf = [0u8; 1];

        // Read until we get a newline
        loop {
            match port.read(&mut buf) {
                Ok(n) if n > 0 => {
                    response.push(buf[0]);
                    if buf[0] == b'\n' {
                        break;
                    }
                    // Safety: don't read forever
                    if response.len() > 256 {
                        break;
                    }
                }
                Ok(_) => break, // EOF or no data
                Err(e) if e.kind() == std::io::ErrorKind::TimedOut => break,
                Err(e) => return Err(e).context("Failed to read from serial port")?,
            }
        }

        let response = String::from_utf8_lossy(&response).to_string();
        debug!("Read {} bytes: {:?}", response.len(), response.as_bytes());
        debug!("Response string: {:?}", response);
        debug!("Response trimmed: {:?}", response.trim());

        Ok(response)
    }

    /// Convert channel number to hex character (0-9, A-F)
    fn channel_to_hex(channel: u8) -> char {
        if channel < 10 {
            (b'0' + channel) as char
        } else {
            (b'A' + (channel - 10)) as char
        }
    }

    /// Enter serial mode
    pub fn start_serial_mode(&self) -> Result<()> {
        info!("Entering serial mode");
        let response = self.send_command("START\n")?;

        if response.trim() == "OK" {
            Ok(())
        } else {
            anyhow::bail!("Failed to enter serial mode: {}", response);
        }
    }

    /// Exit serial mode
    pub fn stop_serial_mode(&self) -> Result<()> {
        info!("Exiting serial mode");
        let response = self.send_command("STOP\n")?;

        if response.trim() == "OK" {
            Ok(())
        } else {
            anyhow::bail!("Failed to exit serial mode: {}", response);
        }
    }

    /// Set servo angle (0-180 degrees)
    pub fn set_servo_angle(&self, channel: u8, angle: u8) -> Result<()> {
        if channel >= NUM_SERVOS {
            anyhow::bail!("Invalid servo channel: {}", channel);
        }
        if angle > 180 {
            anyhow::bail!("Invalid angle: {} (must be 0-180)", angle);
        }

        let hex_channel = Self::channel_to_hex(channel);
        let cmd = format!("S{}:{}\n", hex_channel, angle);
        let response = self.send_command(&cmd)?;

        if response.trim() == "OK" {
            Ok(())
        } else {
            anyhow::bail!("Failed to set servo angle: {}", response);
        }
    }

    /// Set servo PWM pulse width (0-20000 microseconds)
    pub fn set_servo_pwm(&self, channel: u8, pulse_us: u16) -> Result<()> {
        if channel >= NUM_SERVOS {
            anyhow::bail!("Invalid servo channel: {}", channel);
        }
        if pulse_us > 20000 {
            anyhow::bail!("Invalid pulse width: {} (must be 0-20000)", pulse_us);
        }

        let hex_channel = Self::channel_to_hex(channel);
        let cmd = format!("P{}:{}\n", hex_channel, pulse_us);
        let response = self.send_command(&cmd)?;

        if response.trim() == "OK" {
            Ok(())
        } else {
            anyhow::bail!("Failed to set servo PWM: {}", response);
        }
    }

    /// Execute POSE command (set multiple servos instantly)
    pub fn execute_pose(&self, angles: &[u8]) -> Result<()> {
        if angles.len() > NUM_SERVOS as usize {
            anyhow::bail!("Too many servos: {} (max {})", angles.len(), NUM_SERVOS);
        }

        for &angle in angles {
            if angle > 180 {
                anyhow::bail!("Invalid angle: {} (must be 0-180)", angle);
            }
        }

        let angles_str = angles.iter()
            .map(|a| a.to_string())
            .collect::<Vec<_>>()
            .join(",");

        let cmd = format!("POSE {}\n", angles_str);
        let response = self.send_command(&cmd)?;

        if response.trim() == "OK" {
            Ok(())
        } else {
            anyhow::bail!("Failed to execute POSE: {}", response);
        }
    }

    /// Execute MOVE command (smooth interpolated movement)
    pub fn execute_move(&self, duration_ms: u16, angles: &[u8]) -> Result<()> {
        if angles.len() > NUM_SERVOS as usize {
            anyhow::bail!("Too many servos: {} (max {})", angles.len(), NUM_SERVOS);
        }

        for &angle in angles {
            if angle > 180 {
                anyhow::bail!("Invalid angle: {} (must be 0-180)", angle);
            }
        }

        let angles_str = angles.iter()
            .map(|a| a.to_string())
            .collect::<Vec<_>>()
            .join(",");

        let cmd = format!("MOVE {} {}\n", duration_ms, angles_str);
        let response = self.send_command(&cmd)?;

        if response.trim() == "OK" {
            Ok(())
        } else {
            anyhow::bail!("Failed to execute MOVE: {}", response);
        }
    }

    /// Get servo angle
    pub fn get_servo_angle(&self, channel: u8) -> Result<u8> {
        if channel >= NUM_SERVOS {
            anyhow::bail!("Invalid servo channel: {}", channel);
        }

        let hex_channel = Self::channel_to_hex(channel);
        let cmd = format!("GET {}\n", hex_channel);
        let response = self.send_command(&cmd)?;

        // Parse response: "SERVO 0: 90 degrees"
        let parts: Vec<&str> = response.split_whitespace().collect();
        if parts.len() >= 3 {
            if let Ok(angle) = parts[2].parse::<u8>() {
                return Ok(angle);
            }
        }

        anyhow::bail!("Failed to parse servo angle from response: {}", response);
    }

    /// Get all servo angles
    pub fn get_all_servos(&self) -> Result<Vec<(u8, u8)>> {
        let mut servos = Vec::new();

        for channel in 0..NUM_SERVOS {
            match self.get_servo_angle(channel) {
                Ok(angle) => servos.push((channel, angle)),
                Err(e) => {
                    error!("Failed to get angle for servo {}: {}", channel, e);
                    // Continue with other servos
                }
            }
        }

        Ok(servos)
    }
}
