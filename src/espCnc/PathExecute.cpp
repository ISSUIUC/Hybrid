#include "PathExecute.h"

void PathExecutor::execute(GCodeCommand cmd) {
    int x_rate = motors[0].microsteps_per_sec();
    int y_rate = motors[1].microsteps_per_sec();
    int z_rate = motors[2].microsteps_per_sec();


    if(cmd.code == GCode::G0RapidTravel || cmd.code == GCode::G1LinearInterpolation) {
        int here_x = motors[0].microsteps_for_pos(X);
        int here_y = motors[1].microsteps_for_pos(Y);
        int here_z = motors[2].microsteps_for_pos(Z);

        float end_X = X;
        float end_Y = Y;
        float end_Z = Z;
        if(mode == CoordinateMode::Absolute) {
            if(cmd.coord.mask & (1 << 0)) end_X = cmd.coord.X;
            if(cmd.coord.mask & (1 << 1)) end_Y = cmd.coord.Y;
            if(cmd.coord.mask & (1 << 2)) end_Z = cmd.coord.Z;
        } else {
            if(cmd.coord.mask & (1 << 0)) end_X = X + cmd.coord.X;
            if(cmd.coord.mask & (1 << 1)) end_Y = Z + cmd.coord.Y;
            if(cmd.coord.mask & (1 << 2)) end_Z = Y + cmd.coord.Z;
        }

        int target_x = motors[0].microsteps_for_pos(end_X);
        int target_y = motors[1].microsteps_for_pos(end_Y);
        int target_z = motors[2].microsteps_for_pos(end_Z);
        

        linear_step(target_x - here_x, target_y - here_y, target_z - here_z, x_rate, y_rate, z_rate);

        X = end_X;
        Y = end_Y;
        Z = end_Z;
    }
    if(cmd.code == GCode::G3CircularInterpolationCounterClockwise || cmd.code == GCode::G2CircularInterpolationClockwise) {
        if(cmd.coord.mask & 0b11011 != 0b11011) {
            panic(12);
        }
        float center_X = X + cmd.coord.I;
        float center_Y = Y + cmd.coord.J;
        float radius = sqrtf(cmd.coord.I * cmd.coord.I + cmd.coord.J * cmd.coord.J);
        float theta0 = atan2f(-cmd.coord.J, -cmd.coord.I);
        float end_x = X;
        float end_y = Y;
        float end_z = Z;
        if(mode == CoordinateMode::Absolute) {
            end_x = cmd.coord.X;
            end_y = cmd.coord.Y;
            if(cmd.coord.mask & 0b00100) {
                end_z = cmd.coord.Z;
            }
        } else {
            end_x = X + cmd.coord.X;
            end_y = Y + cmd.coord.Y;
            if(cmd.coord.mask & 0b00100) {
                end_z = Z + cmd.coord.Z;
            }
        }
        float theta1 = atan2f(end_y - center_Y, end_x - center_X);
        if(cmd.code == GCode::G3CircularInterpolationCounterClockwise){
            if(theta1 < theta0) theta1 += 2 * PI; //enforce counterclockwise
        } else {
            if(theta1 > theta0) theta1 -= 2 * PI; //enforce clockwise
        }
        constexpr int steps = 128;
        float theta_step = (theta1 - theta0) / steps;
        float z_step = (end_z - Z) / steps;
        for(int i = 0; i < steps; i++) {
            float t_next = theta0 + (i+1) * theta_step;
            float next_X = center_X + radius * cosf(t_next);
            float next_Y = center_Y + radius * sinf(t_next);
            float next_Z = Z + z_step;
            int target_x = motors[0].microsteps_for_pos(next_X);
            int target_y = motors[1].microsteps_for_pos(next_Y);
            int target_z = motors[2].microsteps_for_pos(next_Z);
            int here_x = motors[0].microsteps_for_pos(X);
            int here_y = motors[1].microsteps_for_pos(Y);
            int here_z = motors[2].microsteps_for_pos(Z);
            linear_step(target_x - here_x, target_y - here_y, target_z - here_z, x_rate, y_rate, z_rate);
            X = next_X;
            Y = next_Y;
            Z = next_Z;
        }
    }
    if(cmd.code == GCode::G90AbsoluteMode) {
        mode = CoordinateMode::Absolute;
    }
    if(cmd.code == GCode::G91IncrementalMode) {
        mode = CoordinateMode::Absolute;
    }
}

// void PathExecutor::arc_step(int dx, int dy, int di, int dj) {
//     int x_rate = motors[0].microsteps_per_sec();
//     int y_rate = motors[1].microsteps_per_sec();
//     int z_rate = motors[2].microsteps_per_sec();


// }

void PathExecutor::linear_step(int dx, int dy, int dz, int xrate, int yrate, int zrate) {
    motors[0].set_dir(dx > 0);
    motors[1].set_dir(dy > 0);
    motors[2].set_dir(dz > 0);
    dx = abs(dx);
    dy = abs(dy);
    dz = abs(dz);
    int64_t longest_dist = dx;
    int64_t longest_rate = xrate;
    if(longest_dist*yrate < dy*longest_rate) {
        longest_dist = dy;
        longest_rate = yrate;
    }
    if(longest_dist*zrate < dz*longest_rate) {
        longest_dist = dz;
        longest_rate = zrate;
    }

    int move_time = ceil((double)longest_dist/(double)longest_rate*1000000);
    int x_pos = 0;
    int y_pos = 0;
    int z_pos = 0;

    uint64_t start = micros();
    while(x_pos != dx || y_pos != dy || z_pos != dz) {
        uint64_t dt = micros() - start;
        if(x_pos != dx && (int64_t)dt * (int64_t)dx > (int64_t)x_pos * (int64_t)move_time) {
            x_pos++;
            motors[0].step();
        }
        if(y_pos != dy && (int64_t)dt * (int64_t)dy > (int64_t)y_pos * (int64_t)move_time) {
            y_pos++;
            motors[1].step();
        }
        if(z_pos != dz && (int64_t)dt * (int64_t)dz > (int64_t)z_pos * (int64_t)move_time) {
            z_pos++;
            motors[2].step();
        }
    }
}