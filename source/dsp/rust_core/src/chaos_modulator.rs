//! Non-Linear Continuous Chaos Modulator based on Lorenz Attractor (Rules 7, 8, 25, 46, 47)
//! 4th-order Runge-Kutta (RK4) numerical integration for perpetual, correlated, non-repeating dynamics.

pub struct RustChaosModulator {
    sample_rate: f64,
    speed_hz: f32,
    x: f32,
    y: f32,
    z: f32,
    norm_x: f32,
    norm_y: f32,
    norm_z: f32,
}

impl RustChaosModulator {
    pub fn new() -> Self {
        let mut m = Self {
            sample_rate: 44100.0,
            speed_hz: 1.0,
            x: 0.1,
            y: 0.0,
            z: 0.0,
            norm_x: 0.0,
            norm_y: 0.0,
            norm_z: 0.0,
        };
        m.reset();
        m
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.x = 0.1;
        self.y = 0.0;
        self.z = 0.0;
        self.norm_x = 0.0;
        self.norm_y = 0.0;
        self.norm_z = 0.0;
    }

    #[inline]
    pub fn set_speed(&mut self, hz: f32) {
        self.speed_hz = hz.clamp(0.01, 30.0);
    }

    #[inline]
    pub fn process_sample(&mut self) {
        // Constantes canónicas del Atractor Caótico de Lorenz
        const SIGMA: f32 = 10.0;
        const RHO: f32 = 28.0;
        const BETA: f32 = 8.0 / 3.0;

        let sr = if self.sample_rate > 0.0 { self.sample_rate as f32 } else { 44100.0 };
        let dt = (self.speed_hz * 0.005) / sr;

        #[inline(always)]
        fn derivs(x: f32, y: f32, z: f32) -> (f32, f32, f32) {
            let dx = SIGMA * (y - x);
            let dy = x * (RHO - z) - y;
            let dz = x * y - BETA * z;
            (dx, dy, dz)
        }

        let (k1x, k1y, k1z) = derivs(self.x, self.y, self.z);
        let (k2x, k2y, k2z) = derivs(
            self.x + 0.5 * dt * k1x,
            self.y + 0.5 * dt * k1y,
            self.z + 0.5 * dt * k1z,
        );
        let (k3x, k3y, k3z) = derivs(
            self.x + 0.5 * dt * k2x,
            self.y + 0.5 * dt * k2y,
            self.z + 0.5 * dt * k2z,
        );
        let (k4x, k4y, k4z) = derivs(
            self.x + dt * k3x,
            self.y + dt * k3y,
            self.z + dt * k3z,
        );

        self.x += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
        self.y += (dt / 6.0) * (k1y + 2.0 * k2y + 2.0 * k3y + k4y);
        self.z += (dt / 6.0) * (k1z + 2.0 * k2z + 2.0 * k3z + k4z);

        // Protección contra NaNs / Infs y divergencia extrema
        if !self.x.is_finite()
            || !self.y.is_finite()
            || !self.z.is_finite()
            || self.x.abs() > 100.0
            || self.y.abs() > 100.0
            || self.z.abs() > 120.0
        {
            self.reset();
        }

        // Normalización bipolar a [-1.0, +1.0]
        self.norm_x = (self.x / 20.0).clamp(-1.0, 1.0);
        self.norm_y = (self.y / 30.0).clamp(-1.0, 1.0);
        self.norm_z = ((self.z - 25.0) / 25.0).clamp(-1.0, 1.0);
    }

    #[inline]
    pub fn get_normalized_x(&self) -> f32 {
        self.norm_x
    }

    #[inline]
    pub fn get_normalized_y(&self) -> f32 {
        self.norm_y
    }

    #[inline]
    pub fn get_normalized_z(&self) -> f32 {
        self.norm_z
    }
}
