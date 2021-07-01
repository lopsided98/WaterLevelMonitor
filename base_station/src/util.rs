use std::time::{Duration, Instant};

pub struct Timeout {
    start: Instant,
    timeout: Duration,
}

impl Timeout {
    pub fn start(duration: Duration) -> Self {
        Self {
            start: Instant::now(),
            timeout: duration,
        }
    }

    pub fn get(&self) -> Duration {
        self.timeout
            .checked_sub(self.start.elapsed())
            .unwrap_or_else(|| Duration::from_millis(0))
    }
}
