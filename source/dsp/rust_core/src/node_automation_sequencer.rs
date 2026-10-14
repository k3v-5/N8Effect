//! Node Automation Sequencer & Multi-Lane Modulator (Rules 7, 8, 9, 25, 37, 46, 47)
//! 4 independent rhythmic step lanes per node, glide smoothing, shapes, PPQ sync.

#[derive(Debug, Clone, Copy)]
pub struct RustAutomationLane {
    pub active: bool,
    pub target_param_id: u32,
    pub base_value: f32,
    pub has_captured_base: bool,
    pub num_steps: u32,
    pub steps: [f32; 32],
    pub rate: u8,
    pub glide: f32,
    pub amount: f32,
    pub current_output: f32,
    pub target_output: f32,
    pub last_ppq_step: i64,
    pub current_step: u32,
    pub live_modulated_val: f32,
}

impl Default for RustAutomationLane {
    fn default() -> Self {
        Self {
            active: false,
            target_param_id: 0,
            base_value: 0.0,
            has_captured_base: false,
            num_steps: 16,
            steps: [0.5; 32],
            rate: 4, // Sixteenth
            glide: 0.1,
            amount: 1.0,
            current_output: 0.5,
            target_output: 0.5,
            last_ppq_step: -1,
            current_step: 0,
            live_modulated_val: 0.0,
        }
    }
}

pub struct RustNodeAutomationBank {
    pub lanes: [RustAutomationLane; 4],
    pub selected_lane: usize,
}

impl RustNodeAutomationBank {
    pub fn new() -> Self {
        let mut bank = Self {
            lanes: [RustAutomationLane::default(); 4],
            selected_lane: 0,
        };
        for l in 0..4 {
            bank.apply_shape(l, 2); // SidechainPump
        }
        bank
    }

    pub fn prepare(&mut self, _sample_rate: f64) {
        for lane in &mut self.lanes {
            lane.current_output = lane.steps[0];
            lane.target_output = lane.steps[0];
            lane.last_ppq_step = -1;
            lane.current_step = 0;
        }
    }

    pub fn reset(&mut self) {
        for lane in &mut self.lanes {
            lane.current_output = lane.steps[0];
            lane.target_output = lane.steps[0];
            lane.last_ppq_step = -1;
            lane.current_step = 0;
            lane.live_modulated_val = lane.base_value;
        }
    }

    pub fn get_beats_per_step(div: u8) -> f64 {
        match div {
            0 => 4.0,           // Bar_1
            1 => 2.0,           // Half
            2 => 1.0,           // Quarter
            3 => 0.5,           // Eighth
            4 => 0.25,          // Sixteenth
            5 => 0.125,         // ThirtySecond
            6 => 2.0 / 3.0,     // Triplet_Quarter
            7 => 1.0 / 3.0,     // Triplet_Eighth
            8 => 0.75,          // Dotted_Eighth
            9 => 1.5,           // Dotted_Quarter
            _ => 0.25,
        }
    }

    pub fn apply_shape(&mut self, lane_idx: usize, shape: u8) {
        if lane_idx >= 4 {
            return;
        }
        let lane = &mut self.lanes[lane_idx];
        let n = lane.num_steps.clamp(1, 32);

        match shape {
            0 => {
                // RampUp
                for i in 0..n {
                    lane.steps[i as usize] = (i as f32) / ((n.max(2) - 1) as f32);
                }
            }
            1 => {
                // RampDown
                for i in 0..n {
                    lane.steps[i as usize] = 1.0 - (i as f32) / ((n.max(2) - 1) as f32);
                }
            }
            2 => {
                // SidechainPump
                let period = if n >= 8 { 4 } else { n };
                for i in 0..n {
                    let t = ((i % period) as f32) / (period as f32);
                    lane.steps[i as usize] = t * t;
                }
            }
            3 => {
                // Staccato
                for i in 0..n {
                    lane.steps[i as usize] = if i % 2 == 0 { 0.95 } else { 0.05 };
                }
            }
            4 => {
                // Triangle
                let half = n / 2;
                for i in 0..n {
                    if i <= half {
                        lane.steps[i as usize] = (i as f32) / (half.max(1) as f32);
                    } else {
                        lane.steps[i as usize] = 1.0 - ((i - half) as f32) / ((n - half).max(1) as f32);
                    }
                }
            }
            5 => {
                // Random
                let mut seed = 0x55AA1234u32.wrapping_add((lane_idx as u32) * 100);
                for i in 0..n {
                    seed = seed.wrapping_mul(1664525).wrapping_add(1013904223);
                    lane.steps[i as usize] = ((seed & 0xFFFF) as f32) / 65535.0;
                }
            }
            _ => {
                // Flat
                for i in 0..n {
                    lane.steps[i as usize] = 0.5;
                }
            }
        }
    }

    pub fn process_step(
        &mut self,
        lane_idx: usize,
        is_playing: bool,
        ppq_position: f64,
        base_value: f32,
        min_value: f32,
        max_value: f32,
    ) -> (f32, bool) {
        if lane_idx >= 4 {
            return (base_value, false);
        }
        let lane = &mut self.lanes[lane_idx];
        if !lane.active || lane.target_param_id == 0 {
            return (base_value, false);
        }

        if !lane.has_captured_base {
            lane.base_value = base_value;
            lane.has_captured_base = true;
        }

        if is_playing {
            let beats_per_step = Self::get_beats_per_step(lane.rate);
            if beats_per_step > 0.0 {
                let step_idx = (ppq_position / beats_per_step).floor() as i64;
                if step_idx != lane.last_ppq_step {
                    lane.last_ppq_step = step_idx;
                    let n = lane.num_steps.max(1) as i64;
                    let wrapped = ((step_idx % n) + n) % n;
                    lane.current_step = wrapped as u32;
                    lane.target_output = lane.steps[lane.current_step as usize];
                }
            }
        } else {
            let idx = (lane.current_step as usize).min((lane.num_steps.max(1) as usize) - 1);
            lane.target_output = lane.steps[idx];
        }

        let glide_coeff = (1.0 - lane.glide * 0.95).clamp(0.05, 1.0);
        lane.current_output += glide_coeff * (lane.target_output - lane.current_output);

        let range = max_value - min_value;
        let seq_value = min_value + lane.current_output.clamp(0.0, 1.0) * range;
        let modulated_val = ((1.0 - lane.amount) * lane.base_value + lane.amount * seq_value).clamp(min_value, max_value);

        lane.live_modulated_val = modulated_val;
        (modulated_val, true)
    }
}
